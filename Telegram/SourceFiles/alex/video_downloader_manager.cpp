/*
This file is part of Alexgram Desktop,
the official desktop application for the Alexgram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "alex/video_downloader_manager.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Alex {

namespace {

constexpr auto kYtDlpUrlWin = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe";
constexpr auto kYtDlpUrlMac = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_macos";
constexpr auto kYtDlpUrlLinux = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_linux";

constexpr auto kFfmpegUrlWin = "https://github.com/yt-dlp/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip";

constexpr auto kFfmpegZipName = "ffmpeg_win.zip";
constexpr auto kFfmpegExtractDir = "ffmpeg_extracted";

} // namespace

VideoDownloaderManager::VideoDownloaderManager() {
	_binDir = buildBinDir();
	QDir().mkpath(_binDir);
}

VideoDownloaderManager::~VideoDownloaderManager() = default;

QString VideoDownloaderManager::buildBinDir() const {
	return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
		+ u"/alex_tools"_q;
}

QString VideoDownloaderManager::ytDlpPath() const {
#ifdef Q_OS_WIN
	return _binDir + u"/yt-dlp.exe"_q;
#elif defined(Q_OS_MAC)
	return _binDir + u"/yt-dlp_macos"_q;
#else
	return _binDir + u"/yt-dlp_linux"_q;
#endif
}

QString VideoDownloaderManager::ffmpegPath() const {
#ifdef Q_OS_WIN
	return _binDir + u"/ffmpeg.exe"_q;
#else
	return u"ffmpeg"_q;
#endif
}

bool VideoDownloaderManager::areDependenciesReady() const {
	if (!QFile::exists(ytDlpPath())) {
		return false;
	}
#ifdef Q_OS_WIN
	if (!QFile::exists(ffmpegPath())) {
		return false;
	}
#endif
	return true;
}

void VideoDownloaderManager::forceReinstall() {
	if (_downloading) {
		return;
	}
	QFile::remove(ytDlpPath());
	QFile::remove(ffmpegPath());
	
	ensureDependencies();
}

void VideoDownloaderManager::ensureDependencies() {
	if (areDependenciesReady()) {
		SetupProgress p;
		p.stage = DownloadStage::Finished;
		p.percent = 100;
		p.statusText = u"Ready"_q;
		_setupProgress.fire(std::move(p));
		return;
	}

	_pending.clear();
	_downloading = false;

	if (!QFile::exists(ytDlpPath())) {
#ifdef Q_OS_WIN
		const auto ytUrl = QString::fromUtf8(kYtDlpUrlWin);
#elif defined(Q_OS_MAC)
		const auto ytUrl = QString::fromUtf8(kYtDlpUrlMac);
#else
		const auto ytUrl = QString::fromUtf8(kYtDlpUrlLinux);
#endif
		_pending.append({ ytUrl, ytDlpPath(), DownloadStage::DownloadingYtDlp });
	}

#ifdef Q_OS_WIN
	if (!QFile::exists(ffmpegPath())) {
		const auto zipPath = _binDir + u"/"_q + QString::fromUtf8(kFfmpegZipName);
		_pending.append({
			QString::fromUtf8(kFfmpegUrlWin),
			zipPath,
			DownloadStage::DownloadingFfmpeg,
		});
	}
#endif

	downloadNextPending();
}

void VideoDownloaderManager::downloadNextPending() {
	if (_pending.isEmpty() || _downloading) {
		if (!_downloading) {
			finishSetup();
		}
		return;
	}

	_downloading = true;
	const auto item = _pending.first();
	_pending.removeFirst();
	downloadFile(item.url, item.savePath, item.stage);
}

void VideoDownloaderManager::downloadFile(
		const QString &url,
		const QString &savePath,
		DownloadStage stage) {
	const auto isFfmpeg = (stage == DownloadStage::DownloadingFfmpeg);
	const auto stageName = isFfmpeg
		? u"Downloading ffmpeg..."_q
		: u"Downloading yt-dlp..."_q;

	{
		SetupProgress p;
		p.stage = stage;
		p.percent = 0;
		p.statusText = stageName;
		_setupProgress.fire(std::move(p));
	}

	auto request = QNetworkRequest(QUrl(url));
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);

	const auto reply = _network.get(request);

	QObject::connect(reply, &QNetworkReply::downloadProgress,
		[this, stage, stageName](qint64 received, qint64 total) {
			if (total > 0) {
				const auto pct = int((received * 100) / total);
				SetupProgress p;
				p.stage = stage;
				p.percent = pct;
				p.statusText = stageName;
				_setupProgress.fire(std::move(p));
			}
		});

	QObject::connect(reply, &QNetworkReply::finished,
		[this, reply, savePath, stage, isFfmpeg]() {
			_downloading = false;
			if (reply->error() != QNetworkReply::NoError) {
				SetupProgress p;
				p.stage = DownloadStage::Error;
				p.percent = 0;
				p.statusText = u"Network error: "_q + reply->errorString();
				_setupProgress.fire(std::move(p));
				reply->deleteLater();
				return;
			}

			const auto data = reply->readAll();
			reply->deleteLater();

			QFile file(savePath);
			if (!file.open(QIODevice::WriteOnly)) {
				SetupProgress p;
				p.stage = DownloadStage::Error;
				p.percent = 0;
				p.statusText = u"Cannot write to: "_q + savePath;
				_setupProgress.fire(std::move(p));
				return;
			}
			file.write(data);
			file.close();

#ifdef Q_OS_WIN
			if (isFfmpeg) {
				extractFfmpegFromZip(savePath);
				return;
			}
#endif
			makePlatformExecutable(savePath);
			downloadNextPending();
		});
}

#ifdef Q_OS_WIN
void VideoDownloaderManager::extractFfmpegFromZip(const QString &zipPath) {
	{
		SetupProgress p;
		p.stage = DownloadStage::DownloadingFfmpeg;
		p.percent = 95;
		p.statusText = u"Extracting ffmpeg..."_q;
		_setupProgress.fire(std::move(p));
	}

	const auto extractDir = _binDir + u"/"_q + QString::fromUtf8(kFfmpegExtractDir);
	QDir().mkpath(extractDir);

	const auto process = new QProcess();
	const auto script = u"Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force; "
		"Get-ChildItem -Recurse '%2' -Filter 'ffmpeg.exe' | "
		"Select-Object -First 1 | Copy-Item -Destination '%3'"_q
		.arg(zipPath)
		.arg(extractDir)
		.arg(ffmpegPath());

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process, zipPath, extractDir](int code, QProcess::ExitStatus) {
			process->deleteLater();

			QFile::remove(zipPath);
			QDir(extractDir).removeRecursively();

			if (code == 0 && QFile::exists(ffmpegPath())) {
				downloadNextPending();
			} else {
				SetupProgress p;
				p.stage = DownloadStage::Error;
				p.percent = 0;
				p.statusText = u"Failed to extract ffmpeg.exe from zip."_q;
				_setupProgress.fire(std::move(p));
			}
		});

	process->start(
		u"powershell"_q,
		QStringList{ u"-NoProfile"_q, u"-Command"_q, script });
}
#else
void VideoDownloaderManager::extractFfmpegFromZip(const QString &) {
}
#endif

void VideoDownloaderManager::makePlatformExecutable(const QString &path) {
#ifndef Q_OS_WIN
	QFile::setPermissions(path,
		QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
		QFile::ReadGroup | QFile::ExeGroup |
		QFile::ReadOther | QFile::ExeOther);
#endif
}

void VideoDownloaderManager::finishSetup() {
	SetupProgress p;
	p.stage = DownloadStage::Finished;
	p.percent = 100;
	p.statusText = u"All tools ready!"_q;
	_setupProgress.fire(std::move(p));
}

rpl::producer<VideoDownloaderManager::SetupProgress>
VideoDownloaderManager::setupProgress() const {
	return _setupProgress.events();
}

} // namespace Alex
