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
#include <memory>

#include "alex/video_downloader_engine.h"

namespace Alex {

class VideoDownloaderManager;

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

	std::unique_ptr<VideoDownloaderManager> _manager;
	std::unique_ptr<VideoDownloaderEngine> _engine;

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
