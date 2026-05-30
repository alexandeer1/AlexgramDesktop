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
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QCheckBox>
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


class PlaylistBrowserDialog : public QDialog {
	Q_OBJECT
public:
	struct RowWidget {
		QWidget *widget = nullptr;
		QCheckBox *checkbox = nullptr;
		QLabel *thumbnail = nullptr;
		QLabel *indexBadge = nullptr;
		QLabel *titleLabel = nullptr;
		QLabel *durationLabel = nullptr;
		QLabel *statusLabel = nullptr;
		QProgressBar *progressBar = nullptr;
		bool thumbnailStarted = false;
	};

	explicit PlaylistBrowserDialog(
		VideoDownloaderEngine *engine,
		const PlaylistInfo &playlist,
		const QString &defaultDownloadDir,
		QWidget *parent = nullptr);

	[[nodiscard]] QVector<PlaylistEntry> selectedEntries() const;
	[[nodiscard]] QString videoFormatId() const;
	[[nodiscard]] QString audioFormatId() const;
	[[nodiscard]] QString subtitleLang() const;
	[[nodiscard]] QString downloadDir() const;
	[[nodiscard]] int maxConcurrency() const;

	void setItemStatus(int entryIndex, int percent, bool done, bool error, const QString &statusText);
	void setDownloadingMode(bool downloading);
	void setRowVisible(int index, bool visible);

protected:
	void showEvent(QShowEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	void setupUi();
	void applyStyle();
	void populateList();
	void updateSelectionCount();
	void fetchThumbnailForRow(int rowIdx, const QString &url);
	void filterRows(const QString &query);
	void loadVisibleThumbnails();

	VideoDownloaderEngine *_engine = nullptr;
	PlaylistInfo _playlist;
	QString _downloadDir;

	QLabel *_playlistTitleLabel = nullptr;
	QLabel *_uploaderLabel = nullptr;
	QLabel *_countLabel = nullptr;

	QLineEdit *_searchBox = nullptr;
	QPushButton *_selectAllBtn = nullptr;
	QPushButton *_selectNoneBtn = nullptr;
	QPushButton *_invertBtn = nullptr;

	QWidget *_listContainer = nullptr;
	QScrollArea *_scrollArea = nullptr;
	QVector<RowWidget> _rows;

	QComboBox *_qualityCombo = nullptr;
	QComboBox *_subtitleCombo = nullptr;
	QComboBox *_concurrencyCombo = nullptr;
	QLabel *_folderLabel = nullptr;
	QPushButton *_folderButton = nullptr;

	QLabel *_selectionCountLabel = nullptr;
	QPushButton *_downloadBtn = nullptr;
	QPushButton *_cancelBtn = nullptr;

	QNetworkAccessManager *_thumbnailNetwork = nullptr;
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
	void onOpenPlaylistClicked();

	void ensureEngine();
	void setUiEnabled(bool enabled);
	void showPlaylistButton(const PlaylistInfo &info);
	void hidePlaylistButton();

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
	QPushButton *_playlistButton = nullptr;

	QLabel *_thumbnailLabel = nullptr;
	QLabel *_statusLabel = nullptr;
	QProgressBar *_progressBar = nullptr;

	QString _currentUrl;
	QString _downloadDir;
	QString _customCookiesPath;
	QString _rawFormatsText;
	QStringList _selectedAudioFormatIds;
	VideoInfo _lastInfo;
	PlaylistInfo _lastPlaylistInfo;
	bool _isPlaylist = false;
	int _lastQualityIndex = 0;
	bool _formatsDialogOpen = false;

	rpl::lifetime _lifetime;
};

} // namespace Alex
