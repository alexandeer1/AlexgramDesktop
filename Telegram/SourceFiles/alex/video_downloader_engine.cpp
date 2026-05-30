/*
This file is part of Alexgram Desktop,
the official desktop application for the Alexgram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "alex/video_downloader_engine.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QFileInfo>
#include "base/debug_log.h"

namespace Alex {

namespace {

VideoFormat makeAudioOnlyFormat() {
	VideoFormat fmt;
	fmt.id = u"bestaudio"_q;
	fmt.ext = u"m4a"_q;
	fmt.resolution = u"Audio Only"_q;
	fmt.note = u"Best available audio"_q;
	return fmt;
}

VideoFormat makeBestFormat() {
	VideoFormat fmt;
	fmt.id = u"bestvideo+bestaudio/best"_q;
	fmt.ext = u"mp4"_q;
	fmt.resolution = u"yt-dlp Default (Best Quality)"_q;
	fmt.note = u"Highest available quality, default yt-dlp behavior"_q;
	return fmt;
}

} // namespace

VideoDownloaderEngine::VideoDownloaderEngine(
	const QString &ytDlpPath,
	const QString &ffmpegPath)
: _ytDlpPath(ytDlpPath)
, _ffmpegPath(ffmpegPath) {
}

VideoDownloaderEngine::~VideoDownloaderEngine() {
	cancel();
}

void VideoDownloaderEngine::cancel() {
	if (_process && _process->state() != QProcess::NotRunning) {
		_process->kill();
		_process->waitForFinished(2000);
	}
	delete _process;
	_process = nullptr;
	_isDownloading = false;
}

bool VideoDownloaderEngine::isDownloading() const {
	return _isDownloading;
}

void VideoDownloaderEngine::fetchInfo(const QString &url, const QString &browserCookies, const QString &cookiesFile) {
	cancel();

	const auto process = new QProcess(this);
	_process = process;

	const auto outputBuffer = new QByteArray();

	auto args = QStringList{
		u"--dump-json"_q,
		u"--no-playlist"_q,
		u"--no-warnings"_q,
	};
	if (!cookiesFile.isEmpty()) {
		args.append(u"--cookies"_q);
		args.append(cookiesFile);
	} else if (!browserCookies.isEmpty() && browserCookies != u"custom"_q) {
		args.append(u"--cookies-from-browser"_q);
		args.append(browserCookies);
	}
	args.append(url);

	QObject::connect(process, &QProcess::readyReadStandardOutput,
		[process, outputBuffer]() {
			*outputBuffer += process->readAllStandardOutput();
		});

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process, outputBuffer](int code, QProcess::ExitStatus) {
			*outputBuffer += process->readAllStandardOutput();
			if (code == 0 && !outputBuffer->isEmpty()) {
				parseFetchOutput(*outputBuffer);
			} else {
				auto err = QString::fromUtf8(process->readAllStandardError()).trimmed();
				if (err.isEmpty()) {
					err = u"Failed to fetch video information."_q;
				}
				_fetchError.fire(std::move(err));
			}
			delete outputBuffer;
			process->deleteLater();
			if (_process == process) {
				_process = nullptr;
			}
		});

	process->start(_ytDlpPath, args);
}

void VideoDownloaderEngine::updateYtDlp() {
	cancel();

	const auto process = new QProcess(this);
	_process = process;

	{
		DownloadProgress p;
		p.state = State::Downloading;
		p.percent = 0;
		p.statusText = u"Updating yt-dlp..."_q;
		_downloadProgress.fire(std::move(p));
	}

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process](int code, QProcess::ExitStatus) {
			DownloadProgress p;
			if (code == 0) {
				p.state = State::Done;
				p.percent = 100;
				p.statusText = u"yt-dlp updated successfully!"_q;
			} else {
				auto err = QString::fromUtf8(process->readAllStandardError()).trimmed();
				p.state = State::Error;
				p.percent = 0;
				p.statusText = err.isEmpty() ? u"Update failed."_q : std::move(err);
			}
			_downloadProgress.fire(std::move(p));
			process->deleteLater();
			if (_process == process) {
				_process = nullptr;
			}
		});

	process->start(_ytDlpPath, QStringList{ u"-U"_q });
}

static QString FormatSize(qint64 bytes) {
	if (bytes <= 0) return u""_q;
	if (bytes < 1024) return QString::number(bytes) + u" B"_q;
	if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + u" KB"_q;
	if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + u" MB"_q;
	return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + u" GB"_q;
}

void VideoDownloaderEngine::parseFetchOutput(const QByteArray &json) {
	QJsonParseError error;
	auto doc = QJsonDocument::fromJson(json, &error);
	if (doc.isNull() || !doc.isObject()) {
		auto msg = u"Received invalid JSON from yt-dlp."_q;
		_fetchError.fire(std::move(msg));
		return;
	}

	const auto root = doc.object();
	VideoInfo info;
	info.title = root[u"title"_q].toString(u"Unknown Title"_q);
	info.thumbnailUrl = root[u"thumbnail"_q].toString();
	info.webpageUrl = root[u"webpage_url"_q].toString();
	info.uploaderName = root[u"uploader"_q].toString();

	const auto durationSecs = root[u"duration"_q].toInt(0);
	if (durationSecs > 0) {
		const auto h = durationSecs / 3600;
		const auto m = (durationSecs % 3600) / 60;
		const auto s = durationSecs % 60;
		if (h > 0) {
			info.duration = u"%1:%2:%3"_q
				.arg(h)
				.arg(m, 2, 10, QLatin1Char('0'))
				.arg(s, 2, 10, QLatin1Char('0'));
		} else {
			info.duration = u"%1:%2"_q
				.arg(m)
				.arg(s, 2, 10, QLatin1Char('0'));
		}
	}

	info.formats.append(makeBestFormat());

	const auto formatsArr = root[u"formats"_q].toArray();
	
	QString rawText = u"ID       EXT    RESOLUTION  FPS   VCODEC       ACODEC       SIZE         NOTE\n"_q;
	rawText += u"--------------------------------------------------------------------------------------------------\n"_q;
	for (const auto &fv : formatsArr) {
		const auto fobj = fv.toObject();
		auto id = fobj[u"format_id"_q].toString().leftJustified(8, ' ');
		auto ext = fobj[u"ext"_q].toString().leftJustified(6, ' ');
		auto res = fobj[u"resolution"_q].toString().leftJustified(11, ' ');
		auto fps = QString::number(fobj[u"fps"_q].toInt(0)).leftJustified(5, ' ');
		auto vc = fobj[u"vcodec"_q].toString().leftJustified(12, ' ');
		auto ac = fobj[u"acodec"_q].toString().leftJustified(12, ' ');
		
		qint64 b = fobj[u"filesize"_q].toVariant().toLongLong();
		if (b == 0) b = fobj[u"filesize_approx"_q].toVariant().toLongLong();
		auto sizeStr = FormatSize(b);
		if (sizeStr.isEmpty()) sizeStr = u"N/A"_q;
		sizeStr = sizeStr.leftJustified(12, ' ');

		auto note = fobj[u"format_note"_q].toString();
		rawText += u"%1 %2 %3 %4 %5 %6 %7 %8\n"_q.arg(id, ext, res, fps, vc, ac, sizeStr, note);
	}
	info.rawFormatsText = rawText;

	QMap<QString, VideoFormat> heightMap;
	
	qint64 bestAudioSize = 0;
	for (const auto &fv : formatsArr) {
		const auto fobj = fv.toObject();
		if (fobj[u"vcodec"_q].toString() == u"none"_q && fobj[u"acodec"_q].toString() != u"none"_q) {
			qint64 asize = fobj[u"filesize"_q].toVariant().toLongLong();
			if (asize == 0) asize = fobj[u"filesize_approx"_q].toVariant().toLongLong();
			if (asize > bestAudioSize) {
				bestAudioSize = asize;
			}
		}
	}

	for (const auto &fv : formatsArr) {
		const auto fobj = fv.toObject();
		const auto vcodec = fobj[u"vcodec"_q].toString();
		if (vcodec == u"none"_q || vcodec.isEmpty()) {
			continue;
		}
		const auto height = fobj[u"height"_q].toInt(0);
		if (height <= 0) {
			continue;
		}
		const auto key = QString::number(height);

		VideoFormat fmt;
		fmt.id = u"bestvideo[height<=%1]+bestaudio/best[height<=%1]"_q.arg(height);
		fmt.ext = u"mp4"_q;
		fmt.resolution = u"%1p"_q.arg(height);
		fmt.note = fobj[u"format_note"_q].toString();
		
		qint64 vsize = fobj[u"filesize"_q].toVariant().toLongLong();
		if (vsize == 0) vsize = fobj[u"filesize_approx"_q].toVariant().toLongLong();
		fmt.fileSizeApprox = vsize > 0 ? (vsize + bestAudioSize) : 0;
		if (fmt.fileSizeApprox > 0) {
			fmt.resolution += u" ("_q + FormatSize(fmt.fileSizeApprox) + u")"_q;
		}
		
		heightMap[key] = fmt;
	}

	const auto keys = heightMap.keys();
	for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
		info.formats.append(heightMap[*it]);
	}

	info.formats.append(makeAudioOnlyFormat());

	// Parse subtitles
	const auto parseSubs = [&](const QJsonObject &subsObj, const QString &suffix) {
		for (auto it = subsObj.begin(); it != subsObj.end(); ++it) {
			const auto lang = it.key();
			Subtitle sub;
			sub.language = lang;
			sub.name = lang + suffix; // e.g., "en", "en (auto)"
			
			// Avoid duplicates
			bool exists = false;
			for (const auto &s : info.subtitles) {
				if (s.language == lang) {
					exists = true;
					break;
				}
			}
			if (!exists) {
				info.subtitles.append(sub);
			}
		}
	};
	parseSubs(root[u"subtitles"_q].toObject(), u""_q);
	parseSubs(root[u"automatic_captions"_q].toObject(), u" (auto)"_q);

	// Parse audio tracks (dubs)
	QMap<QString, AudioTrack> audioTrackMap;
	for (const auto &fv : formatsArr) {
		const auto fobj = fv.toObject();
		const auto vcodec = fobj[u"vcodec"_q].toString();
		const auto acodec = fobj[u"acodec"_q].toString();
		if (vcodec == u"none"_q && acodec != u"none"_q && !acodec.isEmpty()) {
			auto lang = fobj[u"language"_q].toString();
			const auto id = fobj[u"format_id"_q].toString();
			const auto name = fobj[u"format_note"_q].toString();
			
			if (lang.isEmpty() || lang == u"null"_q) {
				// Fallback to extracting from format_id (e.g. "251-es")
				const auto dashIdx = id.lastIndexOf('-');
				if (dashIdx != -1) {
					lang = id.mid(dashIdx + 1);
				} else if (!name.isEmpty()) {
					lang = name;
				} else {
					lang = u"unknown"_q;
				}
			}
			
			AudioTrack track;
			track.id = id;
			track.language = lang;
			if (!name.isEmpty() && track.language != name) {
				track.language += u" ("_q + name + u")"_q;
			}
			audioTrackMap[lang] = track;
		}
	}
	for (const auto &track : audioTrackMap) {
		info.audioTracks.append(track);
	}

	if (!info.thumbnailUrl.isEmpty()) {
		fetchThumbnail(info.thumbnailUrl);
	}

	_infoReady.fire(std::move(info));
}

void VideoDownloaderEngine::fetchThumbnail(const QString &url) {
	auto request = QNetworkRequest(QUrl(url));
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);

	const auto reply = _network.get(request);

	QObject::connect(reply, &QNetworkReply::finished, [this, reply] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto data = reply->readAll();
			QPixmap pm;
			if (pm.loadFromData(data)) {
				_thumbnailReady.fire(std::move(pm));
			}
		}
		reply->deleteLater();
	});
}

void VideoDownloaderEngine::startDownload(
		const QString &url,
		const QString &videoFormatId,
		const QString &audioFormatId,
		const QString &subtitleLang,
		const QString &outputDir,
		const QString &browserCookies,
		const QString &cookiesFile) {
	cancel();

	const auto process = new QProcess(this);
	_process = process;

	{
		DownloadProgress p;
		p.state = State::Downloading;
		p.percent = 0;
		p.statusText = u"Starting download..."_q;
		_isDownloading = true;
		_downloadProgress.fire(std::move(p));
	}

	QString finalFormatId = videoFormatId;
	bool multiAudio = false;

	if (videoFormatId != u"bestaudio"_q) {
		if (!audioFormatId.isEmpty()) {
			if (finalFormatId.contains(u"+bestaudio"_q)) {
				finalFormatId = finalFormatId.left(finalFormatId.indexOf(u"+bestaudio"_q));
			} else if (finalFormatId.contains(u"bestvideo"_q) && finalFormatId.contains(u"/"_q)) {
				finalFormatId = u"bestvideo"_q;
			}

			auto audioParts = audioFormatId.split(u","_q);
			if (audioParts.size() > 1) {
				multiAudio = true;
			}
			
			for (const auto &part : audioParts) {
				if (!part.isEmpty() && !finalFormatId.contains(part)) {
					finalFormatId += u"+"_q + part;
				}
			}
		}
		
		if (!finalFormatId.contains(u"/"_q) && !finalFormatId.contains(u"+"_q)) {
			finalFormatId += u"+bestaudio/best"_q;
		} else if (!finalFormatId.contains(u"/"_q)) {
			finalFormatId += u"/best"_q;
		}
	}

	auto args = QStringList{
		u"-f"_q, finalFormatId,
	};
	if (videoFormatId == u"bestaudio"_q) {
		args.append(u"-x"_q);
		args.append(u"--audio-format"_q);
		args.append(u"m4a"_q);
	}
	if (QFileInfo(_ffmpegPath).isAbsolute()) {
		args.append(u"--ffmpeg-location"_q);
		args.append(_ffmpegPath);
	}
	if (multiAudio) {
		args.append(u"--audio-multistreams"_q);
	}
	
	if (!cookiesFile.isEmpty()) {
		args.append(u"--cookies"_q);
		args.append(cookiesFile);
	} else if (!browserCookies.isEmpty() && browserCookies != u"custom"_q) {
		args.append(u"--cookies-from-browser"_q);
		args.append(browserCookies);
	}
	
	if (!subtitleLang.isEmpty()) {
		args.append(u"--write-subs"_q);
		args.append(u"--write-auto-subs"_q);
		args.append(u"--sub-langs"_q);
		args.append(subtitleLang.split(u" (auto)"_q).first());
		args.append(u"--embed-subs"_q);
		// Embedded subs work best in mkv
		args.append(u"--merge-output-format"_q);
		args.append(u"mkv"_q);
	}
	
	args.append(u"--newline"_q);
	args.append(u"--progress"_q);
	args.append(u"--embed-thumbnail"_q);
	args.append(u"-o"_q);
	args.append(outputDir + u"/%(title)s.%(ext)s"_q);
	args.append(url);


	QObject::connect(process, &QProcess::readyReadStandardOutput,
		[this, process]() {
			while (process->canReadLine()) {
				const auto line = QString::fromUtf8(process->readLine()).trimmed();
				parseDownloadLine(line);
			}
		});

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process](int code, QProcess::ExitStatus) {
			DownloadProgress p;
			if (code == 0) {
				p.state = State::Done;
				p.percent = 100;
				p.statusText = u"Download complete!"_q;
			} else {
				auto err = QString::fromUtf8(process->readAllStandardError()).trimmed();
				p.state = State::Error;
				p.percent = 0;
				p.statusText = err.isEmpty() ? u"Download failed."_q : std::move(err);
			}
			_downloadProgress.fire(std::move(p));
			_isDownloading = false;
			process->deleteLater();
			if (_process == process) {
				_process = nullptr;
			}
		});

	LOG(("VideoDownloader Info: yt-dlp arguments: %1").arg(args.join(u" "_q)));
	process->start(_ytDlpPath, args);
}

void VideoDownloaderEngine::parseDownloadLine(const QString &line) {
	static const QRegularExpression re(
		u"\\[download\\]\\s+(\\d+\\.?\\d*)%.*?(?:at\\s+(\\S+/s))?.*?(?:ETA\\s+(\\S+))?"_q);
	const auto match = re.match(line);
	if (match.hasMatch()) {
		DownloadProgress p;
		p.state = State::Downloading;
		p.percent = int(match.captured(1).toDouble());
		p.speed = match.captured(2);
		p.eta = match.captured(3);
		p.statusText = u"Downloading... %1%"_q.arg(p.percent);
		_downloadProgress.fire(std::move(p));
	}
}

rpl::producer<VideoInfo> VideoDownloaderEngine::infoReady() const {
	return _infoReady.events();
}

rpl::producer<QPixmap> VideoDownloaderEngine::thumbnailReady() const {
	return _thumbnailReady.events();
}

rpl::producer<QString> VideoDownloaderEngine::fetchError() const {
	return _fetchError.events();
}

rpl::producer<VideoDownloaderEngine::DownloadProgress>
VideoDownloaderEngine::downloadProgress() const {
	return _downloadProgress.events();
}

} // namespace Alex
