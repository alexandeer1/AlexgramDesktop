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
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include "base/debug_log.h"

namespace Alex {

namespace {

constexpr auto kYtDlpUrlWin = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe";
constexpr auto kYtDlpUrlMac = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_macos";
constexpr auto kYtDlpUrlLinux = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_linux";

constexpr auto kFfmpegUrlWin = "https://github.com/yt-dlp/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip";
constexpr auto kFfmpegUrlMac = "https://evermeet.cx/ffmpeg/getrelease/ffmpeg/zip";
constexpr auto kFfmpegUrlLinux = "https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-amd64-static.tar.xz";

constexpr auto kFfmpegZipName = "ffmpeg_win.zip";
constexpr auto kFfmpegMacZipName = "ffmpeg_mac.zip";
constexpr auto kFfmpegLinuxTarName = "ffmpeg_linux.tar.xz";
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
#elif defined(Q_OS_MAC)
	return _binDir + u"/ffmpeg_mac"_q;
#else
	return _binDir + u"/ffmpeg_linux"_q;
#endif
}

QString VideoDownloaderManager::resolvedFfmpegPath() const {
	const auto local = ffmpegPath();
	if (QFile::exists(local)) {
		return local;
	}
	return QStandardPaths::findExecutable(u"ffmpeg"_q);
}

bool VideoDownloaderManager::areDependenciesReady() const {
	return isYtDlpReady() && isFfmpegReady();
}

bool VideoDownloaderManager::isYtDlpReady() const {
	return QFile::exists(ytDlpPath());
}

bool VideoDownloaderManager::isFfmpegReady() const {
	if (QFile::exists(ffmpegPath())) {
		return true;
	}
	return !QStandardPaths::findExecutable(u"ffmpeg"_q).isEmpty();
}

void VideoDownloaderManager::ensureYtDlp() {
	if (isYtDlpReady() || _downloading) {
		return;
	}
	_pending.clear();
#ifdef Q_OS_WIN
	const auto ytUrl = QString::fromUtf8(kYtDlpUrlWin);
#elif defined(Q_OS_MAC)
	const auto ytUrl = QString::fromUtf8(kYtDlpUrlMac);
#else
	const auto ytUrl = QString::fromUtf8(kYtDlpUrlLinux);
#endif
	_pending.append({ ytUrl, ytDlpPath(), DownloadStage::DownloadingYtDlp });
	downloadNextPending();
}

void VideoDownloaderManager::ensureFfmpeg() {
	if (isFfmpegReady() || _downloading) {
		return;
	}
	_pending.clear();
#ifdef Q_OS_WIN
	const auto ffmpegArchivePath = _binDir + u"/"_q + QString::fromUtf8(kFfmpegZipName);
	const auto ffmpegUrl = QString::fromUtf8(kFfmpegUrlWin);
#elif defined(Q_OS_MAC)
	const auto ffmpegArchivePath = _binDir + u"/"_q + QString::fromUtf8(kFfmpegMacZipName);
	const auto ffmpegUrl = QString::fromUtf8(kFfmpegUrlMac);
#else
	const auto ffmpegArchivePath = _binDir + u"/"_q + QString::fromUtf8(kFfmpegLinuxTarName);
	const auto ffmpegUrl = QString::fromUtf8(kFfmpegUrlLinux);
#endif
	_pending.append({
		ffmpegUrl,
		ffmpegArchivePath,
		DownloadStage::DownloadingFfmpeg,
	});
	downloadNextPending();
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

const auto needFfmpeg = !QFile::exists(ffmpegPath())
		&& QStandardPaths::findExecutable(u"ffmpeg"_q).isEmpty();
	if (needFfmpeg) {
#ifdef Q_OS_WIN
		const auto ffmpegArchivePath = _binDir + u"/"_q + QString::fromUtf8(kFfmpegZipName);
		const auto ffmpegUrl = QString::fromUtf8(kFfmpegUrlWin);
#elif defined(Q_OS_MAC)
		const auto ffmpegArchivePath = _binDir + u"/"_q + QString::fromUtf8(kFfmpegMacZipName);
		const auto ffmpegUrl = QString::fromUtf8(kFfmpegUrlMac);
#else
		const auto ffmpegArchivePath = _binDir + u"/"_q + QString::fromUtf8(kFfmpegLinuxTarName);
		const auto ffmpegUrl = QString::fromUtf8(kFfmpegUrlLinux);
#endif
		_pending.append({
			ffmpegUrl,
			ffmpegArchivePath,
			DownloadStage::DownloadingFfmpeg,
		});
	}

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
				if (isFfmpeg && savePath.endsWith(u".tar.xz"_q)) {
					LOG(("VideoDownloader Info: tar.xz download failed: %1. Trying zip fallback from Martin Riedl...").arg(reply->errorString()));
					reply->deleteLater();
					const auto zipUrl = u"https://ffmpeg.martin-riedl.de/redirect/latest/linux/amd64/release/ffmpeg.zip"_q;
					const auto zipSavePath = _binDir + u"/ffmpeg_linux_fallback.zip"_q;
					downloadFile(zipUrl, zipSavePath, DownloadStage::DownloadingFfmpeg);
					return;
				}
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

			if (isFfmpeg) {
				extractFfmpegFromArchive(savePath);
				return;
			}
			makePlatformExecutable(savePath);
			downloadNextPending();
		});
}

void VideoDownloaderManager::extractFfmpegFromArchive(const QString &archivePath) {
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

#ifdef Q_OS_WIN
	const auto script = u"Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force; "
		"Get-ChildItem -Recurse '%2' -Filter 'ffmpeg.exe' | "
		"Select-Object -First 1 | Copy-Item -Destination '%3'"_q
		.arg(archivePath)
		.arg(extractDir)
		.arg(ffmpegPath());
	process->start(
		u"powershell"_q,
		QStringList{ u"-NoProfile"_q, u"-Command"_q, script });
#elif defined(Q_OS_MAC)
	const auto script = u"unzip -o '%1' -d '%2' && find '%2' -type f -name 'ffmpeg' -exec mv {} '%3' \\; && chmod +x '%3'"_q
		.arg(archivePath)
		.arg(extractDir)
		.arg(ffmpegPath());
	process->start(u"bash"_q, QStringList{ u"-c"_q, script });
#else
	const auto isZip = archivePath.endsWith(u".zip"_q);
	const auto script = isZip
		? u"(unzip -o '%1' -d '%2' || python3 -c \"import zipfile; zipfile.ZipFile('%1').extractall('%2')\" || python -c \"import zipfile; zipfile.ZipFile('%1').extractall('%2')\") && find '%2' -type f -name 'ffmpeg' -exec mv {} '%3' \\; && chmod +x '%3'"_q
			.arg(archivePath)
			.arg(extractDir)
			.arg(ffmpegPath())
		: u"(tar -xf '%1' -C '%2' || python3 -c \"import tarfile; tarfile.open('%1').extractall('%2')\" || python -c \"import tarfile; tarfile.open('%1').extractall('%2')\") && find '%2' -type f -name 'ffmpeg' -exec mv {} '%3' \\; && chmod +x '%3'"_q
			.arg(archivePath)
			.arg(extractDir)
			.arg(ffmpegPath());
	process->start(u"sh"_q, QStringList{ u"-c"_q, script });
#endif

	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process, archivePath, extractDir](int code, QProcess::ExitStatus) {
			const auto stdOut = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
			const auto stdErr = QString::fromUtf8(process->readAllStandardError()).trimmed();
			process->deleteLater();

			QFile::remove(archivePath);
			QDir(extractDir).removeRecursively();

			if (code == 0 && QFile::exists(ffmpegPath())) {
				downloadNextPending();
			} else {
				LOG(("VideoDownloader Error: ffmpeg extraction failed. Code: %1, Stdout: %2, Stderr: %3")
					.arg(code)
					.arg(stdOut)
					.arg(stdErr));

				const auto isTarXz = archivePath.endsWith(u".tar.xz"_q);
				if (isTarXz) {
					LOG(("VideoDownloader Info: tar.xz extraction failed. Trying zip fallback from Martin Riedl..."));
					const auto zipUrl = u"https://ffmpeg.martin-riedl.de/redirect/latest/linux/amd64/release/ffmpeg.zip"_q;
					const auto zipSavePath = _binDir + u"/ffmpeg_linux_fallback.zip"_q;
					downloadFile(zipUrl, zipSavePath, DownloadStage::DownloadingFfmpeg);
					return;
				}

				SetupProgress p;
				p.stage = DownloadStage::Error;
				p.percent = 0;
				p.statusText = u"Failed to extract ffmpeg from archive."_q;
				_setupProgress.fire(std::move(p));
			}
		});
}

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

void VideoDownloaderManager::checkYtDlpVersion() {
	if (_checkingVersion || !isYtDlpReady()) {
		return;
	}
	_checkingVersion = true;

	{
		YtDlpVersionInfo info;
		info.state = VersionState::Checking;
		_versionStream.fire(std::move(info));
	}

	const auto process = new QProcess();
	QObject::connect(process,
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		[this, process](int code, QProcess::ExitStatus) {
			const auto installed = QString::fromUtf8(
				process->readAllStandardOutput()).trimmed();
			const auto stdErr = QString::fromUtf8(
				process->readAllStandardError()).trimmed();
			process->deleteLater();

			if (code != 0 || installed.isEmpty()) {
				_checkingVersion = false;
				YtDlpVersionInfo err;
				err.state = VersionState::Error;
				err.errorDetails = u"Process failed. Code: %1, Error: %2"_q.arg(code).arg(stdErr);
				LOG(("VideoDownloader Error: checkYtDlpVersion process failed: %1").arg(err.errorDetails));
				_versionStream.fire(std::move(err));
				return;
			}

			auto request = QNetworkRequest(
				QUrl(u"https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest"_q));
			request.setRawHeader("User-Agent", "AlexgramDesktop/1.0");
			request.setAttribute(
				QNetworkRequest::RedirectPolicyAttribute,
				QNetworkRequest::NoLessSafeRedirectPolicy);

			const auto reply = _network.get(request);
			QObject::connect(reply, &QNetworkReply::finished,
				[this, reply, installed]() {
					_checkingVersion = false;
					if (reply->error() != QNetworkReply::NoError) {
						YtDlpVersionInfo err;
						err.state = VersionState::Error;
						err.installed = installed;
						err.errorDetails = u"Network failed: %1"_q.arg(reply->errorString());
						LOG(("VideoDownloader Error: checkYtDlpVersion network failed: %1").arg(err.errorDetails));
						reply->deleteLater();
						_versionStream.fire(std::move(err));
						return;
					}
					const auto data = reply->readAll();
					reply->deleteLater();

					const auto doc = QJsonDocument::fromJson(data);
					const auto latest = doc.object()
						[u"tag_name"_q].toString().trimmed();

					if (latest.isEmpty()) {
						YtDlpVersionInfo err;
						err.state = VersionState::Error;
						err.installed = installed;
						err.errorDetails = u"Invalid JSON or rate limit exceeded. Response: %1"_q.arg(QString::fromUtf8(data).left(200));
						LOG(("VideoDownloader Error: checkYtDlpVersion JSON empty: %1").arg(err.errorDetails));
						_versionStream.fire(std::move(err));
						return;
					}

					YtDlpVersionInfo info;
					info.installed = installed;
					info.latest = latest;
					info.state = (installed == latest)
						? VersionState::UpToDate
						: VersionState::UpdateAvailable;
					LOG(("VideoDownloader Info: checkYtDlpVersion success. Installed: %1, Latest: %2").arg(installed).arg(latest));
					_versionStream.fire(std::move(info));
				});
		});
	process->start(ytDlpPath(), QStringList{ u"--version"_q });
}

rpl::producer<VideoDownloaderManager::YtDlpVersionInfo>
VideoDownloaderManager::ytDlpVersion() const {
	return _versionStream.events();
}

} // namespace Alex
