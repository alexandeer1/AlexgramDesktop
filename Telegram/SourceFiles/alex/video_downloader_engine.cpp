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
#include <QtCore/QTimer>
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

QString FormatSize(qint64 bytes) {
	if (bytes <= 0) return u""_q;
	if (bytes < 1024) return QString::number(bytes) + u" B"_q;
	if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + u" KB"_q;
	if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + u" MB"_q;
	return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + u" GB"_q;
}

QString FormatDurationSecs(int secs) {
	if (secs <= 0) return QString();
	const auto h = secs / 3600;
	const auto m = (secs % 3600) / 60;
	const auto s = secs % 60;
	if (h > 0) {
		return u"%1:%2:%3"_q
			.arg(h)
			.arg(m, 2, 10, QLatin1Char('0'))
			.arg(s, 2, 10, QLatin1Char('0'));
	}
	return u"%1:%2"_q
		.arg(m)
		.arg(s, 2, 10, QLatin1Char('0'));
}

QStringList buildCookieArgs(const QString &browserCookies, const QString &cookiesFile) {
	QStringList args;
	if (!cookiesFile.isEmpty()) {
		args << u"--cookies"_q << cookiesFile;
	} else if (!browserCookies.isEmpty() && browserCookies != u"custom"_q) {
		args << u"--cookies-from-browser"_q << browserCookies;
	}
	return args;
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

void VideoDownloaderEngine::cancelPlaylistDownload() {
	_playlistState.cancelled = true;
	_isPlaylistDownloading = false;

	for (auto it = _playlistState.activeProcesses.begin(); it != _playlistState.activeProcesses.end(); ++it) {
		auto process = it.value();
		if (process) {
			if (process->state() != QProcess::NotRunning) {
				process->kill();
				process->waitForFinished(1000);
			}
			process->deleteLater();
		}
	}
	_playlistState.activeProcesses.clear();
	_playlistState.activeCount = 0;

	cancel();
}

bool VideoDownloaderEngine::isDownloading() const {
	return _isDownloading;
}

bool VideoDownloaderEngine::isPlaylistDownloading() const {
	return _isPlaylistDownloading;
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
	args << buildCookieArgs(browserCookies, cookiesFile);
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

void VideoDownloaderEngine::fetchPlaylistInfo(const QString &url, const QString &browserCookies, const QString &cookiesFile) {
	_lastFetchUrl = url;
	_lastFetchBrowserCookies = browserCookies;
	_lastFetchCookiesFile = cookiesFile;

	cancel();

	const auto process = new QProcess(this);
	_process = process;

	const auto outputBuffer = new QByteArray();

	auto args = QStringList{
		u"--flat-playlist"_q,
		u"--dump-json"_q,
		u"--no-warnings"_q,
	};
	args << buildCookieArgs(browserCookies, cookiesFile);
	args.append(url);

	QObject::connect(process, &QProcess::readyReadStandardOutput,
		[process, outputBuffer]() {
			*outputBuffer += process->readAllStandardOutput();
		});

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process, outputBuffer, url](int code, QProcess::ExitStatus) {
			*outputBuffer += process->readAllStandardOutput();
			if (code == 0 && !outputBuffer->isEmpty()) {
				parseFlatPlaylistOutput(*outputBuffer);
			} else {
				auto err = QString::fromUtf8(process->readAllStandardError()).trimmed();
				if (err.isEmpty()) {
					err = u"Failed to fetch playlist information."_q;
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
	if (info.thumbnailUrl.isEmpty()) {
		const auto thumbs = root[u"thumbnails"_q].toArray();
		if (!thumbs.isEmpty()) {
			info.thumbnailUrl = thumbs.first().toObject()[u"url"_q].toString();
		}
	}
	info.webpageUrl = root[u"webpage_url"_q].toString();
	info.uploaderName = root[u"uploader"_q].toString();

	const auto durationSecs = root[u"duration"_q].toInt(0);
	info.duration = FormatDurationSecs(durationSecs);

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

	const auto parseSubs = [&](const QJsonObject &subsObj, const QString &suffix) {
		for (auto it = subsObj.begin(); it != subsObj.end(); ++it) {
			const auto lang = it.key();
			Subtitle sub;
			sub.language = lang;
			sub.name = lang + suffix;

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

void VideoDownloaderEngine::parseFlatPlaylistOutput(const QByteArray &jsonLines) {
	PlaylistInfo playlist;
	bool headerParsed = false;

	const auto lines = jsonLines.split('\n');
	for (const auto &rawLine : lines) {
		const auto line = rawLine.trimmed();
		if (line.isEmpty()) {
			continue;
		}

		QJsonParseError err;
		auto doc = QJsonDocument::fromJson(line, &err);
		if (doc.isNull() || !doc.isObject()) {
			continue;
		}

		const auto obj = doc.object();
		const auto entryType = obj[u"_type"_q].toString();

		if (!headerParsed) {
			playlist.playlistTitle = obj[u"playlist_title"_q].toString();
			if (playlist.playlistTitle.isEmpty()) {
				playlist.playlistTitle = obj[u"playlist"_q].toString();
			}
			playlist.playlistId = obj[u"playlist_id"_q].toString();
			playlist.uploaderName = obj[u"playlist_uploader"_q].toString();
			if (playlist.uploaderName.isEmpty()) {
				playlist.uploaderName = obj[u"uploader"_q].toString();
			}
			headerParsed = true;
		}

		PlaylistEntry entry;
		entry.index = obj[u"playlist_index"_q].toInt(playlist.entries.size() + 1);
		entry.id = obj[u"id"_q].toString();
		entry.title = obj[u"title"_q].toString();
		if (entry.title.isEmpty()) {
			entry.title = u"[Unavailable]"_q;
			entry.isAvailable = false;
		}
		entry.thumbnailUrl = obj[u"thumbnail"_q].toString();
		if (entry.thumbnailUrl.isEmpty()) {
			const auto thumbs = obj[u"thumbnails"_q].toArray();
			if (!thumbs.isEmpty()) {
				entry.thumbnailUrl = thumbs.first().toObject()[u"url"_q].toString();
			}
		}
		entry.webpageUrl = obj[u"url"_q].toString();
		if (entry.webpageUrl.isEmpty()) {
			entry.webpageUrl = obj[u"webpage_url"_q].toString();
		}
		const auto durationSecs = obj[u"duration"_q].toInt(0);
		entry.duration = FormatDurationSecs(durationSecs);

		if (entry.id.isEmpty() && !entry.webpageUrl.isEmpty()) {
			entry.id = entry.webpageUrl;
		}

		if (!entry.id.isEmpty()) {
			playlist.entries.append(entry);
		}
	}

	playlist.totalCount = playlist.entries.size();

	if (playlist.totalCount == 0) {
		_fetchError.fire(u"No entries found in playlist."_q);
		return;
	}

	if (playlist.playlistId.isEmpty()) {
		QTimer::singleShot(0, this, [this] {
			fetchInfo(_lastFetchUrl, _lastFetchBrowserCookies, _lastFetchCookiesFile);
		});
		return;
	}

	_playlistReady.fire(std::move(playlist));
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
	process->setProperty("alreadyDownloaded", false);

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

	args << buildCookieArgs(browserCookies, cookiesFile);

	if (!subtitleLang.isEmpty()) {
		args.append(u"--write-subs"_q);
		args.append(u"--write-auto-subs"_q);
		args.append(u"--sub-langs"_q);
		args.append(subtitleLang.split(u" (auto)"_q).first());
		args.append(u"--embed-subs"_q);
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
				parseDownloadLine(line, process);
			}
		});

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process](int code, QProcess::ExitStatus) {
			DownloadProgress p;
			if (code == 0) {
				p.state = State::Done;
				p.percent = 100;
				const bool alreadyDownloaded = process->property("alreadyDownloaded").toBool();
				p.statusText = alreadyDownloaded ? u"Already Downloaded"_q : u"Download complete!"_q;
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

void VideoDownloaderEngine::startPlaylistDownload(
		const QVector<PlaylistEntry> &entries,
		const QString &videoFormatId,
		const QString &audioFormatId,
		const QString &subtitleLang,
		const QString &downloadDir,
		const QString &browserCookies,
		const QString &cookiesFile,
		int maxConcurrency) {
	cancel();

	_playlistState.items = entries;
	_playlistState.nextIndexToStart = 0;
	_playlistState.activeCount = 0;
	_playlistState.completedCount = 0;
	_playlistState.maxConcurrency = maxConcurrency;
	_playlistState.activeProcesses.clear();
	_playlistState.videoFormatId = videoFormatId;
	_playlistState.audioFormatId = audioFormatId;
	_playlistState.subtitleLang = subtitleLang;
	_playlistState.downloadDir = downloadDir;
	_playlistState.browserCookies = browserCookies;
	_playlistState.cookiesFile = cookiesFile;
	_playlistState.cancelled = false;
	_isPlaylistDownloading = true;

	const int startCount = std::min(maxConcurrency, entries.size());
	for (int i = 0; i < startCount; ++i) {
		processNextPlaylistItem();
	}
}

void VideoDownloaderEngine::processNextPlaylistItem() {
	if (_playlistState.cancelled) {
		return;
	}

	if (_playlistState.nextIndexToStart >= _playlistState.items.size()) {
		if (_playlistState.activeCount == 0) {
			_isPlaylistDownloading = false;
			_playlistDownloadDone.fire(int(_playlistState.completedCount));
		}
		return;
	}

	const auto itemIndex = _playlistState.nextIndexToStart++;
	_playlistState.activeCount++;

	const auto &entry = _playlistState.items[itemIndex];

	{
		PlaylistItemProgress prog;
		prog.entryIndex = itemIndex;
		prog.percent = 0;
		prog.statusText = u"Starting..."_q;
		_playlistItemProgress.fire(std::move(prog));
	}

	const auto process = new QProcess(this);
	_playlistState.activeProcesses[itemIndex] = process;
	process->setProperty("alreadyDownloaded", false);

	auto &st = _playlistState;

	QString finalFormatId = st.videoFormatId;
	bool multiAudio = false;

	if (st.videoFormatId != u"bestaudio"_q) {
		if (!st.audioFormatId.isEmpty()) {
			if (finalFormatId.contains(u"+bestaudio"_q)) {
				finalFormatId = finalFormatId.left(finalFormatId.indexOf(u"+bestaudio"_q));
			} else if (finalFormatId.contains(u"bestvideo"_q) && finalFormatId.contains(u"/"_q)) {
				finalFormatId = u"bestvideo"_q;
			}
			auto audioParts = st.audioFormatId.split(u","_q);
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

	auto args = QStringList{ u"-f"_q, finalFormatId };

	if (st.videoFormatId == u"bestaudio"_q) {
		args << u"-x"_q << u"--audio-format"_q << u"m4a"_q;
	}
	if (QFileInfo(_ffmpegPath).isAbsolute()) {
		args << u"--ffmpeg-location"_q << _ffmpegPath;
	}
	if (multiAudio) {
		args.append(u"--audio-multistreams"_q);
	}

	args << buildCookieArgs(st.browserCookies, st.cookiesFile);

	if (!st.subtitleLang.isEmpty()) {
		args << u"--write-subs"_q << u"--write-auto-subs"_q
		     << u"--sub-langs"_q << st.subtitleLang.split(u" (auto)"_q).first()
		     << u"--embed-subs"_q << u"--merge-output-format"_q << u"mkv"_q;
	}

	args << u"--newline"_q << u"--progress"_q << u"--embed-thumbnail"_q
	     << u"-o"_q << (st.downloadDir + u"/%(title)s.%(ext)s"_q)
	     << u"--no-playlist"_q
	     << entry.webpageUrl;

	QObject::connect(process, &QProcess::readyReadStandardOutput,
		[this, process, itemIndex]() {
			while (process->canReadLine()) {
				const auto line = QString::fromUtf8(process->readLine()).trimmed();
				if (line.contains(u"has already been downloaded"_q, Qt::CaseInsensitive)) {
					process->setProperty("alreadyDownloaded", true);
					PlaylistItemProgress prog;
					prog.entryIndex = itemIndex;
					prog.percent = 100;
					prog.done = true;
					prog.statusText = u"Already Downloaded"_q;
					_playlistItemProgress.fire(std::move(prog));
					continue;
				}
				static const QRegularExpression re(
					u"\\[download\\]\\s+(\\d+\\.?\\d*)%.*?(?:at\\s+(\\S+/s))?.*?(?:ETA\\s+(\\S+))?"_q);
				const auto match = re.match(line);
				if (match.hasMatch()) {
					PlaylistItemProgress prog;
					prog.entryIndex = itemIndex;
					prog.percent = int(match.captured(1).toDouble());
					prog.speed = match.captured(2);
					prog.eta = match.captured(3);
					prog.statusText = u"Downloading... %1%"_q.arg(prog.percent);
					_playlistItemProgress.fire(std::move(prog));
				}
			}
		});

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process, itemIndex](int code, QProcess::ExitStatus) {
			_playlistState.activeCount--;
			if (code == 0) {
				_playlistState.completedCount++;
			}
			_playlistState.activeProcesses.remove(itemIndex);
			process->deleteLater();

			PlaylistItemProgress prog;
			prog.entryIndex = itemIndex;
			if (code == 0) {
				prog.percent = 100;
				prog.done = true;
				const bool alreadyDownloaded = process->property("alreadyDownloaded").toBool();
				prog.statusText = alreadyDownloaded ? u"Already Downloaded"_q : u"Done"_q;
			} else {
				prog.error = true;
				prog.errorText = QString::fromUtf8(process->readAllStandardError()).trimmed();
				if (prog.errorText.isEmpty()) {
					prog.errorText = u"Download failed"_q;
				}
				prog.statusText = u"Error"_q;
			}
			_playlistItemProgress.fire(std::move(prog));

			if (!_playlistState.cancelled) {
				processNextPlaylistItem();
			} else {
				if (_playlistState.activeCount == 0) {
					_isPlaylistDownloading = false;
				}
			}
		});

	LOG(("VideoDownloader Playlist [%1/%2]: %3")
		.arg(itemIndex + 1)
		.arg(_playlistState.items.size())
		.arg(entry.webpageUrl));
	process->start(_ytDlpPath, args);
}

void VideoDownloaderEngine::parseDownloadLine(const QString &line, QProcess *process) {
	if (line.contains(u"has already been downloaded"_q, Qt::CaseInsensitive)) {
		process->setProperty("alreadyDownloaded", true);
		DownloadProgress p;
		p.state = State::Done;
		p.percent = 100;
		p.statusText = u"Already Downloaded"_q;
		_downloadProgress.fire(std::move(p));
		return;
	}
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

rpl::producer<PlaylistInfo> VideoDownloaderEngine::playlistReady() const {
	return _playlistReady.events();
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

rpl::producer<PlaylistItemProgress> VideoDownloaderEngine::playlistItemProgress() const {
	return _playlistItemProgress.events();
}

rpl::producer<int> VideoDownloaderEngine::playlistDownloadDone() const {
	return _playlistDownloadDone.events();
}

} // namespace Alex
