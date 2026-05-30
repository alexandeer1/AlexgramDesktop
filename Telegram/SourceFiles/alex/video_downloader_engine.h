/*
This file is part of Alexgram Desktop,
the official desktop application for the Alexgram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QtCore/QMap>
#include <QtGui/QPixmap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <rpl/event_stream.h>
#include <rpl/producer.h>
#include "base/weak_ptr.h"

namespace Alex {

struct VideoFormat {
	QString id;
	QString resolution;
	QString ext;
	QString note;
	qint64 fileSizeApprox = 0;
};

struct AudioTrack {
	QString id;
	QString language;
};

struct Subtitle {
	QString language;
	QString name;
};

struct VideoInfo {
	QString title;
	QString duration;
	QString uploaderName;
	QString thumbnailUrl;
	QString webpageUrl;
	QString rawFormatsText;
	QVector<VideoFormat> formats;
	QVector<AudioTrack> audioTracks;
	QVector<Subtitle> subtitles;
};

struct PlaylistEntry {
	int index = 0;
	QString id;
	QString title;
	QString duration;
	QString thumbnailUrl;
	QString webpageUrl;
	bool isAvailable = true;
};

struct PlaylistInfo {
	QString playlistTitle;
	QString playlistId;
	QString uploaderName;
	int totalCount = 0;
	QVector<PlaylistEntry> entries;
};

struct PlaylistItemProgress {
	int entryIndex = 0;
	int percent = 0;
	QString statusText;
	QString speed;
	QString eta;
	bool done = false;
	bool error = false;
	QString errorText;
};

class VideoDownloaderEngine final : public QObject, public base::has_weak_ptr {
public:
	enum class State {
		Idle,
		FetchingInfo,
		FetchingPlaylist,
		Downloading,
		Error,
		Done,
	};

	struct DownloadProgress {
		State state = State::Idle;
		int percent = 0;
		QString statusText;
		QString eta;
		QString speed;
	};

	explicit VideoDownloaderEngine(
		const QString &ytDlpPath,
		const QString &ffmpegPath);
	~VideoDownloaderEngine();

	void fetchInfo(const QString &url, const QString &browserCookies, const QString &cookiesFile = QString());
	void fetchPlaylistInfo(const QString &url, const QString &browserCookies, const QString &cookiesFile = QString());
	void updateYtDlp();
	void startDownload(
		const QString &url,
		const QString &videoFormatId,
		const QString &audioFormatId,
		const QString &subtitleLang,
		const QString &downloadDir,
		const QString &browserCookies,
		const QString &cookiesFile = QString());
	void startPlaylistDownload(
		const QVector<PlaylistEntry> &entries,
		const QString &videoFormatId,
		const QString &audioFormatId,
		const QString &subtitleLang,
		const QString &downloadDir,
		const QString &browserCookies,
		const QString &cookiesFile = QString(),
		int maxConcurrency = 3);
	void cancel();
	void cancelPlaylistDownload();
	[[nodiscard]] bool isDownloading() const;
	[[nodiscard]] bool isPlaylistDownloading() const;

	[[nodiscard]] rpl::producer<VideoInfo> infoReady() const;
	[[nodiscard]] rpl::producer<PlaylistInfo> playlistReady() const;
	[[nodiscard]] rpl::producer<QPixmap> thumbnailReady() const;
	[[nodiscard]] rpl::producer<QString> fetchError() const;
	[[nodiscard]] rpl::producer<DownloadProgress> downloadProgress() const;
	[[nodiscard]] rpl::producer<PlaylistItemProgress> playlistItemProgress() const;
	[[nodiscard]] rpl::producer<int> playlistDownloadDone() const;

private:
	void parseFetchOutput(const QByteArray &json);
	void parseFlatPlaylistOutput(const QByteArray &jsonLines);
	void parseDownloadLine(const QString &line, QProcess *process);
	void fetchThumbnail(const QString &url);
	void processNextPlaylistItem();

	QString _ytDlpPath;
	QString _ffmpegPath;

	QString _lastFetchUrl;
	QString _lastFetchBrowserCookies;
	QString _lastFetchCookiesFile;

	QProcess *_process = nullptr;
	bool _isDownloading = false;

	struct PlaylistDownloadState {
		QVector<PlaylistEntry> items;
		int nextIndexToStart = 0;
		int activeCount = 0;
		int completedCount = 0;
		int maxConcurrency = 3;
		QMap<int, QProcess*> activeProcesses;
		QString videoFormatId;
		QString audioFormatId;
		QString subtitleLang;
		QString downloadDir;
		QString browserCookies;
		QString cookiesFile;
		bool cancelled = false;
	};
	PlaylistDownloadState _playlistState;
	bool _isPlaylistDownloading = false;

	QNetworkAccessManager _network;

	rpl::event_stream<VideoInfo> _infoReady;
	rpl::event_stream<PlaylistInfo> _playlistReady;
	rpl::event_stream<QPixmap> _thumbnailReady;
	rpl::event_stream<QString> _fetchError;
	rpl::event_stream<DownloadProgress> _downloadProgress;
	rpl::event_stream<PlaylistItemProgress> _playlistItemProgress;
	rpl::event_stream<int> _playlistDownloadDone;
};

} // namespace Alex
