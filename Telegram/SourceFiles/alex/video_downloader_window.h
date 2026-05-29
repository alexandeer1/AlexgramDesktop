/*
This file is part of Alexgram Desktop,
the official desktop application for the Alexgram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QDialog>
#include <QtCore/QStringList>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QDialogButtonBox>
#include <rpl/lifetime.h>
#include <rpl/producer.h>
#include <rpl/event_stream.h>
#include <memory>

#include "alex/video_downloader_engine.h"

namespace Alex {

class VideoDownloaderManager;

class DepsStatusWidget : public QWidget {
public:
	explicit DepsStatusWidget(VideoDownloaderManager *manager, QWidget *parent = nullptr);

	[[nodiscard]] rpl::producer<> allReady() const;

private:
	struct ToolRow {
		QLabel *iconLabel = nullptr;
		QLabel *nameLabel = nullptr;
		QLabel *statusLabel = nullptr;
		QLabel *versionLabel = nullptr;
		QPushButton *updateBtn = nullptr;
		QProgressBar *progressBar = nullptr;
		QPushButton *installBtn = nullptr;
	};

	void setupUi();
	void applyStyle();
	void checkAndRefresh();
	void setToolReady(ToolRow &row, const QString &name);
	void setToolMissing(ToolRow &row, const QString &name);
	void setToolDownloading(ToolRow &row, const QString &name, int percent);
	void setYtDlpVersionChecking();
	void setYtDlpVersionUpToDate(const QString &version);
	void setYtDlpVersionUpdateAvailable(const QString &installed, const QString &latest);
	void setYtDlpVersionError(const QString &installed, const QString &errorDetails);

	VideoDownloaderManager *_manager = nullptr;
	ToolRow _ytDlpRow;
	ToolRow _ffmpegRow;
	QLabel *_allGoodLabel = nullptr;
	QPushButton *_dismissBtn = nullptr;
	rpl::event_stream<> _allReadyStream;
	rpl::lifetime _lifetime;
};


class VideoDownloaderSetupWindow : public QWidget {
public:
	VideoDownloaderSetupWindow(QWidget *parent = nullptr);
	~VideoDownloaderSetupWindow() override;

	static void Show();

private:
	void setupUi();
	void startSetup();
	void applyStyle();

	std::unique_ptr<VideoDownloaderManager> _manager;
	QLabel *_titleLabel = nullptr;
	QLabel *_descLabel = nullptr;
	QProgressBar *_progressBar = nullptr;
	QLabel *_statusLabel = nullptr;
	QPushButton *_retryButton = nullptr;
	QPushButton *_cancelButton = nullptr;

	rpl::lifetime _lifetime;
};

class VideoDownloaderWindow : public QWidget {
public:
	VideoDownloaderWindow(QWidget *parent = nullptr);
	~VideoDownloaderWindow() override;

	static void Show();
	rpl::lifetime &lifetime() { return _lifetime; }

private:
	void setupUi();
	void setupConnections();
	void applyStyle();

	void onFetchClicked();
	void onDownloadClicked();
	void onFolderClicked();
	void onAudioButtonClicked();

	void ensureEngine();
	void setUiEnabled(bool enabled);

	std::unique_ptr<VideoDownloaderManager> _manager;
	std::unique_ptr<VideoDownloaderEngine> _engine;
	DepsStatusWidget *_depsWidget = nullptr;

	QLineEdit *_urlInput = nullptr;
	QPushButton *_fetchButton = nullptr;
	QPushButton *_updateButton = nullptr;
	QLabel *_titleLabel = nullptr;
	QLabel *_metaLabel = nullptr;
	QComboBox *_qualityCombo = nullptr;
	QPushButton *_audioButton = nullptr;
	QComboBox *_subtitleCombo = nullptr;
	QComboBox *_browserCombo = nullptr;
	QLabel *_folderLabel = nullptr;
	QPushButton *_folderButton = nullptr;
	QPushButton *_downloadButton = nullptr;
	
	QLabel *_thumbnailLabel = nullptr;
	QLabel *_statusLabel = nullptr;
	QProgressBar *_progressBar = nullptr;

	QString _currentUrl;
	QString _downloadDir;
	QString _customCookiesPath;
	QString _rawFormatsText;
	QStringList _selectedAudioFormatIds;
	VideoInfo _lastInfo;
	
	rpl::lifetime _lifetime;
};

} // namespace Alex
