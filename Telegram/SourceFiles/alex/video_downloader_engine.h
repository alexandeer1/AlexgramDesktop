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

class VideoDownloaderEngine final : public QObject, public base::has_weak_ptr {
public:
	enum class State {
		Idle,
		FetchingInfo,
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
	void updateYtDlp();
	void startDownload(
		const QString &url,
		const QString &videoFormatId,
		const QString &audioFormatId,
		const QString &subtitleLang,
		const QString &downloadDir,
		const QString &browserCookies,
		const QString &cookiesFile = QString());
	void cancel();
	[[nodiscard]] bool isDownloading() const;

	[[nodiscard]] rpl::producer<VideoInfo> infoReady() const;
	[[nodiscard]] rpl::producer<QPixmap> thumbnailReady() const;
	[[nodiscard]] rpl::producer<QString> fetchError() const;
	[[nodiscard]] rpl::producer<DownloadProgress> downloadProgress() const;

private:
	void parseFetchOutput(const QByteArray &json);
	void parseDownloadLine(const QString &line);
	void fetchThumbnail(const QString &url);

	QString _ytDlpPath;
	QString _ffmpegPath;

	QProcess *_process = nullptr;
	bool _isDownloading = false;
	QNetworkAccessManager _network;

	rpl::event_stream<VideoInfo> _infoReady;
	rpl::event_stream<QPixmap> _thumbnailReady;
	rpl::event_stream<QString> _fetchError;
	rpl::event_stream<DownloadProgress> _downloadProgress;
};

} // namespace Alex
