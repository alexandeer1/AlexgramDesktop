#include "alex/alex_crash_report.h"
#include "core/application.h"
#include "core/crash_report_window.h"
#include "core/sandbox.h"
#include "logs.h"
#include <QtCore/QDateTime>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QHttpMultiPart>

namespace Alex {
namespace {

// Premium styles for the crash window
const char kCrashWindowStyle[] = R"(
    Alex--CrashWindow {
        background-color: #f4f4f7;
    }
    QLabel {
        color: #2c3e50;
    }
    QRadioButton {
        color: #34495e;
        padding: 5px;
    QWidget {
        background-color: #17212b;
        color: #f5f5f5;
        font-family: "Segoe UI", "Roboto", sans-serif;
        font-size: 14px;
    }
    QLabel#header {
        font-size: 18px;
        font-weight: bold;
        color: #ffffff;
    }
    QLabel#desc {
        color: #708499;
        font-size: 13px;
    }
    QPushButton {
        background-color: #2b97e2;
        color: white;
        border: none;
        padding: 8px 20px;
        font-weight: bold;
        border-radius: 6px;
    }
    QPushButton:hover {
        background-color: #35a0e9;
    }
    QPushButton:disabled {
        background-color: #242f3d;
        color: #49596a;
    }
    QPushButton#network {
        background-color: transparent;
        color: #2b97e2;
        font-size: 12px;
        text-transform: uppercase;
    }
    QPushButton#continue {
        background-color: transparent;
        color: #2b97e2;
    }
    QPlainTextEdit {
        border: 1px solid #242f3d;
        background-color: #0e1621;
        color: #abb3bb;
        font-family: "Consolas", "Monaco", monospace;
        font-size: 12px;
        border-radius: 4px;
        padding: 5px;
    }
    QRadioButton {
        spacing: 10px;
        color: #eff3f7;
    }
    QRadioButton::indicator {
        width: 18px;
        height: 18px;
    }
)";

} // namespace

QString LogSanitizer::Scrub(const QString &text) {
  auto result = text;

  static const QRegularExpression phoneRegex(u"\\+\\d{10,15}"_q);
  result.replace(phoneRegex, u"[PHONE_SCRUBBED]"_q);

  static const QRegularExpression tokenRegex(u"\\d+:[a-zA-Z0-9_-]{35}"_q);
  result.replace(tokenRegex, u"[TOKEN_SCRUBBED]"_q);

  QString home = QDir::homePath();
  if (!home.isEmpty()) {
    result.replace(home, u"~"_q);
  }

  return result;
}

QString LogExtractor::GetFull() {
  auto result = Logs::full();
  QFile oldLog(cWorkingDir() + u"log.txt"_q);
  if (oldLog.open(QIODevice::ReadOnly)) {
    auto oldContent = QString::fromUtf8(oldLog.readAll());
    if (!oldContent.isEmpty()) {
      result = oldContent + u"\n[NEW SESSION STARTED]\n"_q + result;
    }
  }
  return result;
}

QString LogExtractor::GetRecent(int seconds) {
  const auto full = GetFull();
  const auto lines = full.split('\n', Qt::SkipEmptyParts);
  if (lines.isEmpty())
    return QString();

  static const QRegularExpression tsRegex(
      u"\\[(\\d{4}\\.\\d{2}\\.\\d{2} \\d{2}:\\d{2}:\\d{2})\\]"_q);

  QDateTime lastTime;
  for (int i = lines.size() - 1; i >= 0; --i) {
    auto match = tsRegex.match(lines[i]);
    if (match.hasMatch()) {
      lastTime =
          QDateTime::fromString(match.captured(1), u"yyyy.MM.dd hh:mm:ss"_q);
      if (lastTime.isValid())
        break;
    }
  }

  if (!lastTime.isValid()) {
    int count = qMin(50, (int)lines.size());
    return lines.mid(lines.size() - count).join('\n');
  }

  QStringList recent;
  for (int i = lines.size() - 1; i >= 0; --i) {
    auto match = tsRegex.match(lines[i]);
    if (match.hasMatch()) {
      auto currentTime =
          QDateTime::fromString(match.captured(1), u"yyyy.MM.dd hh:mm:ss"_q);
      if (currentTime.isValid() && currentTime.secsTo(lastTime) > seconds) {
        break;
      }
    }
    recent.prepend(lines[i]);
  }

  return recent.join('\n');
}

CrashWindow::CrashWindow(const QByteArray &crashdump, Fn<void()> launch)
    : PreLaunchWindow(u"Alexgram Crash Reporter"_q), _dumpraw(crashdump),
      _header(this), _description(this),
      _radioRecent(new QRadioButton(u"Last 10 Seconds"_q, this)),
      _radioFull(new QRadioButton(u"Full Log (Recommended)"_q, this)), _preview(this),
      _networkSettings(this), _send(this), _continue(this),
      _launch(std::move(launch)) {

  setStyleSheet(kCrashWindowStyle);

  _header.setObjectName(u"header"_q);
  _header.setText(u"Ouch! Alexgram crashed last time."_q);
  _header.setAlignment(Qt::AlignCenter);

  _description.setObjectName(u"desc"_q);
  _description.setText(
      u"Please help us fix it by sending a sanitized log. Sensitive data like phone numbers are automatically removed."_q);
  _description.setWordWrap(true);
  _description.setAlignment(Qt::AlignCenter);

  _radioFull->setChecked(true);
  _preview.setPlaceholderText(u"Loading log preview..."_q);
  _preview.setReadOnly(true);

  _networkSettings.setObjectName(u"network"_q);
  _networkSettings.setText(u"Network Settings"_q);
  _networkSettings.setCursor(Qt::PointingHandCursor);

  _send.setText(u"SEND REPORT"_q);
  _send.setObjectName(u"send"_q);
  _send.setCursor(Qt::PointingHandCursor);

  _continue.setText(u"CONTINUE TO APP"_q);
  _continue.setObjectName(u"continue"_q);
  _continue.setCursor(Qt::PointingHandCursor);

  connect(_radioRecent, &QRadioButton::toggled, [=] { updatePreview(); });
  connect(_radioFull, &QRadioButton::toggled, [=] { updatePreview(); });
  connect(&_networkSettings, &QPushButton::clicked, [=] { networkSettings(); });
  connect(&_send, &QPushButton::clicked, [=] { sendReport(); });
  connect(&_continue, &QPushButton::clicked, [=] { close(); });

  _fullLog = LogExtractor::GetFull();
  _recentLog = LogExtractor::GetRecent(10);

  updatePreview();
  updateControls();

  setMinimumSize(_size * 25, _size * 30);
  resize(_size * 30, _size * 35);
  show();
}

void CrashWindow::updatePreview() {
  QString log = _radioRecent->isChecked() ? _recentLog : _fullLog;
  _preview.setPlainText(LogSanitizer::Scrub(log));
}

void CrashWindow::updateControls() {
}

void CrashWindow::networkSettings() {
  auto host = QString(), user = QString(), password = QString();
  auto port = quint32(0);
  const auto proxy = Core::Sandbox::Instance().sandboxProxy();
  if (proxy.type == MTP::ProxyData::Type::Socks5 ||
      proxy.type == MTP::ProxyData::Type::Http) {
    host = proxy.host;
    port = proxy.port;
    user = proxy.user;
    password = proxy.password;
  }
  auto window = new NetworkSettingsWindow(this, host, port, user, password);
  window->saveRequests() |
      rpl::on_next(
          [=](MTP::ProxyData &&proxy) { _proxyChanges.fire(std::move(proxy)); },
          _lifetime);
  window->activate();
}

void CrashWindow::sendReport() {
  if (_sent)
    return;

  _send.setEnabled(false);
  _send.setText(u"SENDING..."_q);

  const auto scriptUrl =
      u"https://script.google.com/macros/s/AKfycbwBT5PiqFnwx7lZGeV7hLmTkrfcq9oc-h6KGYWdvRlnBuvk5sY_gM6Ajt59QRZgH6Fr/exec"_q;

  auto multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  // 1. Log content
  QHttpPart logPart;
  logPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                    QVariant(u"form-data; name=\"log\""_q));
  logPart.setBody(_preview.toPlainText().toUtf8());
  multipart->append(logPart);

  // 2. System Info (Device, OS, Version)
  QString info = u"App Version: %1\nDevice: %2\nOS: %3\nKernel: %4"_q
      .arg(QString::number(AppVersion))
      .arg(QSysInfo::machineHostName())
      .arg(QSysInfo::prettyProductName())
      .arg(QSysInfo::kernelType() + u" "_q + QSysInfo::kernelVersion());

  QHttpPart infoPart;
  infoPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(u"form-data; name=\"version\""_q)); // Reuse version field for full info
  infoPart.setBody(info.toUtf8());
  multipart->append(infoPart);

  _reply = _manager.post(QNetworkRequest(QUrl(scriptUrl)), multipart);
  multipart->setParent(_reply);

  connect(_reply, &QNetworkReply::finished, [=] { onSendFinished(); });
}

void CrashWindow::onSendFinished() {
  if (_reply->error() == QNetworkReply::NoError) {
    _sent = true;
    _send.setText(u"SENT! THANK YOU"_q);
    _send.setStyleSheet(u"background-color: #27ae60; color: white;"_q);
    _header.setText(u"Report Sent Successfully!"_q);
  } else {
    _send.setEnabled(true);
    _send.setText(u"RETRY SENDING"_q);
    _header.setText(u"Failed to send report."_q);
    _header.setStyleSheet(u"color: #ff4444;"_q);
  }
  _reply->deleteLater();
  _reply = nullptr;
}

void CrashWindow::resizeEvent(QResizeEvent *e) {
  int w = width(), h = height();
  int pad = _size;

  _header.setGeometry(pad, pad * 1.5, w - 2 * pad, pad * 2);
  _description.setGeometry(pad * 2, pad * 4, w - 4 * pad, pad * 4);

  _radioRecent->setGeometry(pad * 2, pad * 8.5, w - 4 * pad, pad * 2);
  _radioFull->setGeometry(pad * 2, pad * 10.5, w - 4 * pad, pad * 2);

  // Dynamic preview area (fills most of the screen)
  _preview.setGeometry(pad * 2, pad * 13, w - 4 * pad, h - pad * 22);

  _networkSettings.setGeometry(pad, h - pad * 8, w - 2 * pad, pad * 2);

  int btnW = (w - 5 * pad) / 2;
  _send.setGeometry(pad * 2, h - pad * 5, btnW, pad * 3);
  _continue.setGeometry(w - btnW - pad * 2, h - pad * 5, btnW, pad * 3);
}

void CrashWindow::onSendError(QNetworkReply::NetworkError e) {
  _sent = false;
  _send.setEnabled(true);
  _send.setText(u"RETRY SENDING"_q);

  _header.setText(u"Report failed to send."_q);
  _header.setStyleSheet(u"color: #ff4444; font-weight: bold;"_q);
}

void CrashWindow::closeEvent(QCloseEvent *e) {
  if (_launch)
    _launch();
  deleteLater();
}

} // namespace Alex
