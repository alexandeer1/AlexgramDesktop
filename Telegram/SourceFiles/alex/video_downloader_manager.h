/*
This file is part of Alexgram Desktop,
the official desktop application for the Alexgram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <rpl/event_stream.h>
#include <rpl/producer.h>
#include "base/weak_ptr.h"

namespace Alex {

class VideoDownloaderManager final : public QObject, public base::has_weak_ptr {
public:
	enum class DownloadStage {
		Idle,
		DownloadingYtDlp,
		DownloadingFfmpeg,
		Finished,
		Error,
	};

	struct SetupProgress {
		DownloadStage stage = DownloadStage::Idle;
		int percent = 0;
		QString statusText;
	};

	enum class VersionState {
		Checking,
		UpToDate,
		UpdateAvailable,
		Error,
	};

	struct YtDlpVersionInfo {
		VersionState state = VersionState::Checking;
		QString installed;
		QString latest;
		QString errorDetails;
	};

	VideoDownloaderManager();
	~VideoDownloaderManager();

	[[nodiscard]] bool areDependenciesReady() const;
	[[nodiscard]] bool isYtDlpReady() const;
	[[nodiscard]] bool isFfmpegReady() const;
	void ensureDependencies();
	void ensureYtDlp();
	void ensureFfmpeg();

	[[nodiscard]] QString ytDlpPath() const;
	[[nodiscard]] QString ffmpegPath() const;
	[[nodiscard]] QString resolvedFfmpegPath() const;

	void forceReinstall();
	void checkYtDlpVersion();

	[[nodiscard]] rpl::producer<SetupProgress> setupProgress() const;
	[[nodiscard]] rpl::producer<YtDlpVersionInfo> ytDlpVersion() const;

private:
	void downloadNextPending();
	void downloadFile(const QString &url, const QString &savePath, DownloadStage stage);
	void extractFfmpegFromArchive(const QString &archivePath);
	void makePlatformExecutable(const QString &path);
	void finishSetup();
	QString buildBinDir() const;

	QNetworkAccessManager _network;
	QString _binDir;

	struct PendingDownload {
		QString url;
		QString savePath;
		DownloadStage stage;
	};
	QVector<PendingDownload> _pending;
	bool _downloading = false;

	rpl::event_stream<SetupProgress> _setupProgress;
	rpl::event_stream<YtDlpVersionInfo> _versionStream;
	bool _checkingVersion = false;
};

} // namespace Alex
