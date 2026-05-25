/*
This file is part of Alexgram Desktop,
the official desktop application for the Alexgram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "alex/video_downloader_window.h"

#include "alex/video_downloader_manager.h"
#include "alex/video_downloader_engine.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QDialogButtonBox>
#include <QtCore/QStandardPaths>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace Alex {

namespace {
	VideoDownloaderWindow *GlobalWindow = nullptr;
}

VideoDownloaderWindow::VideoDownloaderWindow(QWidget *parent)
: QWidget(parent)
, _manager(std::make_unique<VideoDownloaderManager>()) {
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
	setWindowTitle(u"YT & Media Downloader"_q);
	resize(850, 500);

	_downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

	setupUi();
	setupConnections();
	applyStyle();
}

VideoDownloaderWindow::~VideoDownloaderWindow() {
	if (GlobalWindow == this) {
		GlobalWindow = nullptr;
	}
}

void VideoDownloaderWindow::Show() {
	if (!GlobalWindow) {
		GlobalWindow = new VideoDownloaderWindow();
	}
	GlobalWindow->show();
	GlobalWindow->raise();
	GlobalWindow->activateWindow();
}

void VideoDownloaderWindow::setupUi() {
	auto mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(15, 15, 15, 15);
	mainLayout->setSpacing(15);

	// LEFT PANE (Controls)
	auto leftPane = new QWidget(this);
	leftPane->setObjectName("LeftPane");
	auto leftLayout = new QVBoxLayout(leftPane);
	leftLayout->setContentsMargins(20, 20, 20, 20);
	leftLayout->setSpacing(20);

	auto titleLayout = new QHBoxLayout();
	auto titleAppLabel = new QLabel(u"Modern YT & Media Downloader"_q);
	titleAppLabel->setObjectName("AppTitle");
	
	_updateButton = new QPushButton(u"\U0001F504"_q); // Update/Refresh icon
	_updateButton->setObjectName("InfoBtn"); // Share same style as InfoBtn
	_updateButton->setFixedSize(28, 28);
	_updateButton->setCursor(Qt::PointingHandCursor);

	auto infoButton = new QPushButton(u"\U00002139"_q); // Info icon
	infoButton->setObjectName("InfoBtn");
	infoButton->setFixedSize(28, 28);
	infoButton->setCursor(Qt::PointingHandCursor);
	
	titleLayout->addWidget(titleAppLabel);
	titleLayout->addStretch(1);
	titleLayout->addWidget(_updateButton);
	titleLayout->addWidget(infoButton);
	leftLayout->addLayout(titleLayout);

	QObject::connect(infoButton, &QPushButton::clicked, this, [this] {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle(u"Legal Disclaimer"_q);
		msgBox.setText(u"This tool uses open-source software (yt-dlp and FFmpeg).\n\n"
					   "By using this feature, you agree that you are solely responsible "
					   "for the content you download. You must only download non-copyrighted "
					   "material or content you have explicit permission to use.\n\n"
					   "The developers of Alexgram take absolutely zero responsibility for "
					   "any copyright infringement or misuse of this tool by the end user."_q);
		msgBox.setStyleSheet(u"QMessageBox { background-color: #1a222c; } "
							 "QLabel { color: #fff; font-size: 13px; } "
							 "QPushButton { background-color: #2ed573; color: white; padding: 6px 15px; border-radius: 4px; font-weight: bold; border: none; } "
							 "QPushButton:hover { background-color: #3be080; }"_q);
		msgBox.exec();
	});

	auto inputRow = new QHBoxLayout();
	_urlInput = new QLineEdit();
	_urlInput->setPlaceholderText(u"Paste video URL here..."_q);
	_urlInput->setMinimumHeight(40);
	
	_fetchButton = new QPushButton(u"Fetch Qualities"_q);
	_fetchButton->setMinimumHeight(40);
	_fetchButton->setCursor(Qt::PointingHandCursor);
	_fetchButton->setObjectName("FetchBtn");

	inputRow->addWidget(_urlInput, 1);
	inputRow->addWidget(_fetchButton);
	leftLayout->addLayout(inputRow);

	auto infoRow = new QHBoxLayout();
	_titleLabel = new QLabel(u"Title: Waiting for URL..."_q);
	_titleLabel->setObjectName("InfoTitle");
	_titleLabel->setWordWrap(true);
	_metaLabel = new QLabel(u""_q);
	_metaLabel->setObjectName("InfoMeta");
	_metaLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	infoRow->addWidget(_titleLabel, 1);
	infoRow->addWidget(_metaLabel);
	leftLayout->addLayout(infoRow);

	_qualityCombo = new QComboBox();
	_qualityCombo->setMinimumHeight(40);
	leftLayout->addWidget(_qualityCombo);

	auto optionsRow = new QHBoxLayout();
	_audioButton = new QPushButton(u"Default Audio"_q);
	_audioButton->setMinimumHeight(40);
	_audioButton->setCursor(Qt::PointingHandCursor);
	
	_subtitleCombo = new QComboBox();
	_subtitleCombo->setMinimumHeight(40);
	_subtitleCombo->addItem(u"No Subtitles"_q, u""_q);
	
	_browserCombo = new QComboBox();
	_browserCombo->setMinimumHeight(40);
	_browserCombo->addItem(u"No Cookies"_q, u""_q);
	_browserCombo->addItem(u"Edge"_q, u"edge"_q);
	_browserCombo->addItem(u"Chrome"_q, u"chrome"_q);
	_browserCombo->addItem(u"Firefox"_q, u"firefox"_q);
	_browserCombo->addItem(u"Opera"_q, u"opera"_q);
	_browserCombo->addItem(u"Vivaldi"_q, u"vivaldi"_q);
	_browserCombo->addItem(u"Brave"_q, u"brave"_q);
	_browserCombo->addItem(u"Import Cookies File..."_q, u"custom"_q);
	
	optionsRow->addWidget(_audioButton, 1);
	optionsRow->addWidget(_subtitleCombo, 1);
	optionsRow->addWidget(_browserCombo, 1);
	leftLayout->addLayout(optionsRow);

	auto folderRow = new QHBoxLayout();
	auto folderTitle = new QLabel(u"Save To:"_q);
	folderTitle->setFixedWidth(60);
	_folderLabel = new QLabel(_downloadDir);
	_folderLabel->setObjectName("FolderLabel");
	_folderButton = new QPushButton(u"\U0001F4C2"_q); // Folder icon
	_folderButton->setFixedSize(40, 40);
	_folderButton->setCursor(Qt::PointingHandCursor);
	_folderButton->setObjectName("FolderBtn");

	folderRow->addWidget(folderTitle);
	folderRow->addWidget(_folderLabel, 1);
	folderRow->addWidget(_folderButton);
	leftLayout->addLayout(folderRow);

	leftLayout->addStretch(1);

	_downloadButton = new QPushButton(u"Download"_q);
	_downloadButton->setMinimumHeight(50);
	_downloadButton->setCursor(Qt::PointingHandCursor);
	_downloadButton->setObjectName("DownloadBtn");
	leftLayout->addWidget(_downloadButton);


	// RIGHT PANE (Media & Status)
	auto rightPane = new QWidget(this);
	rightPane->setObjectName("RightPane");
	rightPane->setFixedWidth(350);
	auto rightLayout = new QVBoxLayout(rightPane);
	rightLayout->setContentsMargins(15, 15, 15, 15);
	rightLayout->setSpacing(15);

	_thumbnailLabel = new QLabel(u"No Video Loaded"_q);
	_thumbnailLabel->setObjectName("Thumbnail");
	_thumbnailLabel->setAlignment(Qt::AlignCenter);
	_thumbnailLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	rightLayout->addWidget(_thumbnailLabel, 1);

	auto statusArea = new QWidget();
	statusArea->setObjectName("StatusArea");
	auto statusLayout = new QVBoxLayout(statusArea);
	_statusLabel = new QLabel(u"Ready."_q);
	_statusLabel->setWordWrap(true);
	_progressBar = new QProgressBar();
	_progressBar->setRange(0, 100);
	_progressBar->setValue(0);
	_progressBar->setTextVisible(false);
	_progressBar->setFixedHeight(10);
	statusLayout->addWidget(_statusLabel);
	statusLayout->addWidget(_progressBar);
	rightLayout->addWidget(statusArea);

	mainLayout->addWidget(leftPane, 1);
	mainLayout->addWidget(rightPane);
}

void VideoDownloaderWindow::applyStyle() {
	setStyleSheet(uR"(
		QWidget {
			background-color: #121820;
			color: #e0e5ea;
			font-family: 'Segoe UI', Arial, sans-serif;
			font-size: 13px;
		}
		#LeftPane, #RightPane {
			background-color: #1a222c;
			border-radius: 12px;
			border: 1px solid #2a3542;
		}
		#AppTitle {
			font-size: 18px;
			font-weight: bold;
			color: #ffffff;
		}
		QLineEdit {
			background-color: #121820;
			border: 1px solid #364454;
			border-radius: 8px;
			padding: 0 12px;
			color: #fff;
		}
		QLineEdit:focus {
			border: 1px solid #4a90e2;
		}
		#FetchBtn {
			background-color: #ff574d;
			color: white;
			font-weight: bold;
			border-radius: 8px;
			padding: 0 15px;
		}
		#FetchBtn:hover { background-color: #ff6b62; }
		#FetchBtn:pressed { background-color: #e04a41; }
		
		#DownloadBtn {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff4b2b, stop:1 #ff416c);
			color: white;
			font-weight: bold;
			font-size: 16px;
			border-radius: 10px;
		}
		#DownloadBtn:hover {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff5c40, stop:1 #ff567c);
		}
		
		#CancelBtn {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #d63031, stop:1 #ff7675);
			color: white;
			font-weight: bold;
			font-size: 16px;
			border-radius: 10px;
		}
		#CancelBtn:hover {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff7675, stop:1 #ff8788);
		}
		
		#FolderBtn {
			background-color: #2ed573;
			border-radius: 8px;
			font-size: 16px;
		}
		#FolderBtn:hover { background-color: #3be080; }
		
		QComboBox {
			background-color: #121820;
			border: 1px solid #364454;
			border-radius: 8px;
			padding: 0 12px;
		}
		QComboBox::drop-down { border: none; }
		
		#InfoTitle {
			color: #2ed573;
			font-weight: bold;
			font-size: 15px;
		}
		#InfoMeta {
			color: #9ba5b1;
		}
		#FolderLabel {
			color: #9ba5b1;
			background-color: #121820;
			padding: 8px;
			border-radius: 6px;
		}
		
		#Thumbnail {
			background-color: #121820;
			border-radius: 10px;
			color: #556270;
		}
		#StatusArea {
			background-color: #121820;
			border-radius: 8px;
		}
		QProgressBar {
			background-color: #2a3542;
			border-radius: 5px;
		}
		QProgressBar::chunk {
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2ed573, stop:1 #1e90ff);
			border-radius: 5px;
		}
	)"_q);
}

void VideoDownloaderWindow::setupConnections() {
	connect(_fetchButton, &QPushButton::clicked, this, &VideoDownloaderWindow::onFetchClicked);
	QObject::connect(_downloadButton, &QPushButton::clicked, [=] {
		if (_engine && _engine->isDownloading()) {
			_engine->cancel();
			_downloadButton->setText(u"Download"_q);
			_downloadButton->setObjectName(u"DownloadBtn"_q);
			_downloadButton->style()->unpolish(_downloadButton);
			_downloadButton->style()->polish(_downloadButton);
			_statusLabel->setText(u"Download cancelled."_q);
			_progressBar->setValue(0);
		} else {
			onDownloadClicked();
		}
	});
	QObject::connect(_audioButton, &QPushButton::clicked,
		[=] { onAudioButtonClicked(); });

	connect(_folderButton, &QPushButton::clicked, this, &VideoDownloaderWindow::onFolderClicked);
	
	connect(_qualityCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
		if (index < 0) return;
		if (_qualityCombo->itemData(index).toString() == u"custom_format"_q) {
			QDialog dialog(this);
			dialog.setWindowTitle(u"yt-dlp Video Formats"_q);
			dialog.setMinimumSize(800, 500);
			
			auto layout = new QVBoxLayout(&dialog);
			
			auto listWidget = new QListWidget();
			listWidget->setFont(QFont(u"Consolas"_q));
			for (const auto &line : _rawFormatsText.split('\n')) {
				listWidget->addItem(line);
			}
			layout->addWidget(listWidget);
			
			auto inputLayout = new QHBoxLayout();
			auto formatLabel = new QLabel(u"Or enter format ID manually (e.g. 137+140):"_q);
			auto formatInput = new QLineEdit();
			formatInput->setPlaceholderText(u"Leave empty to use selected row above..."_q);
			inputLayout->addWidget(formatLabel);
			inputLayout->addWidget(formatInput, 1);
			layout->addLayout(inputLayout);

			auto bestAudioCheck = new QCheckBox(u"Best Audio (merge bestaudio with selected format)"_q);
			bestAudioCheck->setStyleSheet(u"QCheckBox { color: #2ed573; font-weight: bold; } QCheckBox::indicator { width: 16px; height: 16px; }"_q);
			layout->addWidget(bestAudioCheck);
			
			auto buttonLayout = new QHBoxLayout();
			auto okBtn = new QPushButton(u"OK"_q);
			auto cancelBtn = new QPushButton(u"Cancel"_q);
			okBtn->setFixedHeight(34);
			cancelBtn->setFixedHeight(34);
			okBtn->setCursor(Qt::PointingHandCursor);
			cancelBtn->setCursor(Qt::PointingHandCursor);
			buttonLayout->addStretch();
			buttonLayout->addWidget(okBtn);
			buttonLayout->addWidget(cancelBtn);
			layout->addLayout(buttonLayout);
			
			connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
			connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
			connect(listWidget, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);
			
			dialog.setStyleSheet(u"QDialog { background-color: #1a222c; color: #fff; font-family: 'Segoe UI', sans-serif; } "
								 "QListWidget { background-color: #121820; color: #e0e5ea; font-family: Consolas, monospace; font-size: 13px; border: 1px solid #2a3542; } "
								 "QListWidget::item { padding: 2px; } "
								 "QListWidget::item:selected { background-color: #2ed573; color: #121820; } "
								 "QLineEdit { background-color: #121820; color: #fff; border: 1px solid #364454; border-radius: 4px; padding: 4px 8px; } "
								 "QLineEdit:focus { border: 1px solid #4a90e2; } "
								 "QPushButton { background-color: #2ed573; color: #121820; padding: 6px 22px; border-radius: 6px; font-weight: bold; border: none; } "
								 "QPushButton:hover { background-color: #3be080; } "
								 "QLabel { color: #fff; }"_q);
			
			if (dialog.exec() == QDialog::Accepted) {
				auto customFormat = formatInput->text().trimmed();
				if (customFormat.isEmpty()) {
					if (auto item = listWidget->currentItem()) {
						auto text = item->text().trimmed();
						auto parts = text.split(QChar(' '), Qt::SkipEmptyParts);
						if (!parts.isEmpty()) {
							customFormat = parts.first();
						}
					}
				}
				if (customFormat.isEmpty() || customFormat == u"ID"_q || customFormat.startsWith(u"-"_q)) {
					_qualityCombo->setCurrentIndex(0);
					return;
				}

				if (bestAudioCheck->isChecked()) {
					customFormat = customFormat + u"+bestaudio"_q;
				}
				
				const auto label = u"Custom: %1"_q.arg(customFormat);
				const auto existingIdx = _qualityCombo->findData(customFormat);
				if (existingIdx >= 0) {
					_qualityCombo->setCurrentIndex(existingIdx);
				} else {
					_qualityCombo->insertItem(0, label, customFormat);
					_qualityCombo->setCurrentIndex(0);
				}
			} else {
				_qualityCombo->setCurrentIndex(0);
			}
		}
	});


	connect(_browserCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
		if (_browserCombo->itemData(index).toString() == u"custom"_q) {
			const auto path = QFileDialog::getOpenFileName(
				this,
				u"Select Cookies File"_q,
				QString(),
				u"Text Files (*.txt);;All Files (*)"_q);
			if (!path.isEmpty()) {
				_customCookiesPath = path;
				_browserCombo->setItemText(index, u"File: "_q + QFileInfo(path).fileName());
			} else {
				_browserCombo->setCurrentIndex(0);
				_browserCombo->setItemText(index, u"Import Cookies File..."_q);
			}
		} else {
			const auto customIdx = _browserCombo->findData(u"custom"_q);
			if (customIdx >= 0) {
				_browserCombo->setItemText(customIdx, u"Import Cookies File..."_q);
			}
			_customCookiesPath.clear();
		}
	});

	connect(_updateButton, &QPushButton::clicked, this, [this] {
		QMenu menu(this);
		menu.setStyleSheet(u"QMenu { background-color: #1a222c; border: 1px solid #2a3542; color: #fff; } "
						   "QMenu::item { padding: 8px 20px; } "
						   "QMenu::item:selected { background-color: #2ed573; color: white; }"_q);
		
		auto updateAct = menu.addAction(u"Update yt-dlp"_q);
		auto reinstallAct = menu.addAction(u"Force Reinstall Dependencies"_q);
		
		connect(updateAct, &QAction::triggered, this, [this] {
			ensureEngine();
			_engine->updateYtDlp();
		});
		
		connect(reinstallAct, &QAction::triggered, this, [this] {
			_statusLabel->setText(u"Reinstalling dependencies..."_q);
			_progressBar->setValue(0);
			_manager->setupProgress() | rpl::on_next(
				[=](const VideoDownloaderManager::SetupProgress &prog) {
					_statusLabel->setText(prog.statusText);
					_progressBar->setValue(prog.percent);
				},
				lifetime());
			_manager->forceReinstall();
		});
		
		menu.exec(_updateButton->mapToGlobal(QPoint(0, _updateButton->height())));
	});
}

void VideoDownloaderWindow::ensureEngine() {
	if (_engine) {
		return;
	}
	_engine = std::make_unique<VideoDownloaderEngine>(
		_manager->ytDlpPath(),
		_manager->ffmpegPath());

	// Since rpl fires on Telegram's event loop, and this widget lives in it, it works.
	_engine->infoReady() | rpl::on_next([=](VideoInfo info) {
		_rawFormatsText = info.rawFormatsText;
		_titleLabel->setText(info.title);
		_metaLabel->setText(info.uploaderName + u" • "_q + info.duration);
		
		_qualityCombo->clear();
		for (const auto &fmt : info.formats) {
			_qualityCombo->addItem(fmt.resolution, fmt.id);
		}
		_qualityCombo->addItem(u"Show yt-dlp formats (Custom)..."_q, u"custom_format"_q);
		
		_lastInfo = info;
		_selectedAudioFormatIds.clear();
		_audioButton->setText(u"Default Audio"_q);
		
		_subtitleCombo->clear();
		_subtitleCombo->addItem(u"No Subtitles"_q, u""_q);
		for (const auto &sub : info.subtitles) {
			_subtitleCombo->addItem(sub.name, sub.language);
		}
		
		_statusLabel->setText(u"Ready to download."_q);
		_progressBar->setValue(0);
	}, lifetime());
	
	_engine->thumbnailReady() | rpl::on_next([=](QPixmap pm) {
		auto scaled = pm.scaled(_thumbnailLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

		QPixmap rounded(scaled.size());
		rounded.fill(Qt::transparent);
		QPainter p(&rounded);
		p.setRenderHint(QPainter::Antialiasing);
		QPainterPath path;
		path.addRoundedRect(rounded.rect(), 12, 12);
		p.setClipPath(path);
		p.drawPixmap(0, 0, scaled);
		p.end();

		_thumbnailLabel->setPixmap(rounded);
	}, lifetime());

	_engine->fetchError() | rpl::on_next([=](const QString &err) {
		_statusLabel->setText(u"Error: "_q + err);
		_progressBar->setValue(0);
	}, lifetime());
	
	_engine->downloadProgress() | rpl::on_next([=](const VideoDownloaderEngine::DownloadProgress &prog) {
		auto text = prog.eta.isEmpty()
			? prog.statusText
			: (prog.statusText + u" \u2022 ETA "_q + prog.eta);
		if (prog.state == VideoDownloaderEngine::State::Downloading) {
			_downloadButton->setText(u"\u274C Cancel"_q);
			_downloadButton->setObjectName(u"CancelBtn"_q);
			_downloadButton->style()->unpolish(_downloadButton);
			_downloadButton->style()->polish(_downloadButton);
		} else if (prog.state == VideoDownloaderEngine::State::Done) {
			text = u"Saved successfully!"_q;
			_downloadButton->setText(u"Download"_q);
			_downloadButton->setObjectName(u"DownloadBtn"_q);
			_downloadButton->style()->unpolish(_downloadButton);
			_downloadButton->style()->polish(_downloadButton);
		} else if (prog.state == VideoDownloaderEngine::State::Error) {
			text = u"Error: "_q + prog.statusText;
			_downloadButton->setText(u"Download"_q);
			_downloadButton->setObjectName(u"DownloadBtn"_q);
			_downloadButton->style()->unpolish(_downloadButton);
			_downloadButton->style()->polish(_downloadButton);
		}
		
		_statusLabel->setText(text);
		_progressBar->setValue(prog.percent);
	}, lifetime());
}

void VideoDownloaderWindow::onFetchClicked() {
	_currentUrl = _urlInput->text().trimmed();
	if (_currentUrl.isEmpty()) {
		return;
	}

	_statusLabel->setText(u"Checking dependencies..."_q);
	_progressBar->setValue(0);
	_selectedAudioFormatIds.clear();
	_audioButton->setText(u"Default Audio"_q);

	if (!_manager->areDependenciesReady()) {
		_manager->setupProgress() | rpl::on_next(
			[=](const VideoDownloaderManager::SetupProgress &prog) {
				_statusLabel->setText(prog.statusText);
				_progressBar->setValue(prog.percent);
				
				if (prog.stage == VideoDownloaderManager::DownloadStage::Finished) {
					ensureEngine();
					_statusLabel->setText(u"Fetching video info..."_q);
					_progressBar->setValue(0);
					_engine->fetchInfo(_currentUrl, _browserCombo->currentData().toString(), _customCookiesPath);
				}
			},
			lifetime());
		_manager->ensureDependencies();
	} else {
		ensureEngine();
		_statusLabel->setText(u"Fetching video info..."_q);
		_engine->fetchInfo(_currentUrl, _browserCombo->currentData().toString(), _customCookiesPath);
	}
}

void VideoDownloaderWindow::onFolderClicked() {
	const auto path = QFileDialog::getExistingDirectory(
		this,
		u"Choose download folder"_q,
		_downloadDir);
	if (!path.isEmpty()) {
		_downloadDir = path;
		_folderLabel->setText(_downloadDir);
	}
}

void VideoDownloaderWindow::onDownloadClicked() {
	if (!_engine || _qualityCombo->count() == 0) {
		return;
	}
	const auto formatId = _qualityCombo->currentData().toString();
	
	if (formatId == u"custom_format"_q) {
		return;
	}

	const auto audioId = _selectedAudioFormatIds.join(u","_q);
	const auto subLang = _subtitleCombo->currentData().toString();
	const auto browser = _browserCombo->currentData().toString();
	
	_statusLabel->setText(u"Starting download..."_q);
	_progressBar->setValue(0);	
	_engine->startDownload(
		_currentUrl, formatId, audioId, subLang, _downloadDir, browser, _customCookiesPath);
}

void VideoDownloaderWindow::onAudioButtonClicked() {
	if (_lastInfo.audioTracks.isEmpty()) {
		QMessageBox::information(this, u"No Audio Tracks"_q, u"No multiple audio tracks found for this video."_q);
		return;
	}

	QDialog dialog(this);
	dialog.setWindowTitle(u"Select Audio Tracks"_q);
	dialog.setMinimumWidth(300);
	dialog.setMinimumHeight(400);

	auto layout = new QVBoxLayout(&dialog);

	auto scrollArea = new QScrollArea(&dialog);
	scrollArea->setWidgetResizable(true);
	
	auto contentWidget = new QWidget();
	auto contentLayout = new QVBoxLayout(contentWidget);

	QList<QCheckBox*> checkBoxes;
	for (const auto &trk : _lastInfo.audioTracks) {
		auto cb = new QCheckBox(trk.language, contentWidget);
		cb->setProperty("audioId", trk.id);
		if (_selectedAudioFormatIds.contains(trk.id)) {
			cb->setChecked(true);
		}
		contentLayout->addWidget(cb);
		checkBoxes.append(cb);
	}
	contentLayout->addStretch(1);
	scrollArea->setWidget(contentWidget);
	layout->addWidget(scrollArea);

	auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	layout->addWidget(buttonBox);

	if (dialog.exec() == QDialog::Accepted) {
		_selectedAudioFormatIds.clear();
		for (auto cb : checkBoxes) {
			if (cb->isChecked()) {
				_selectedAudioFormatIds.append(cb->property("audioId").toString());
			}
		}

		if (_selectedAudioFormatIds.isEmpty()) {
			_audioButton->setText(u"Default Audio"_q);
		} else if (_selectedAudioFormatIds.size() == 1) {
			_audioButton->setText(u"1 Track Selected"_q);
		} else {
			_audioButton->setText(u"%1 Tracks Selected"_q.arg(_selectedAudioFormatIds.size()));
		}
	}
}

} // namespace Alex
