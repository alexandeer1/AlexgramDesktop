#pragma once

#include "core/crash_report_window.h"
#include <QtWidgets/QRadioButton>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <rpl/event_stream.h>
#include <rpl/lifetime.h>

namespace Alex {

class LogSanitizer {
public:
    static QString Scrub(const QString &text);
};

class LogExtractor {
public:
    static QString GetFull();
    static QString GetRecent(int seconds = 10);
};

class CrashWindow : public PreLaunchWindow {
public:
    CrashWindow(const QByteArray &crashdump, Fn<void()> launch);

    rpl::producer<MTP::ProxyData> proxyChanges() const {
        return _proxyChanges.events();
    }
    
    rpl::lifetime &lifetime() {
        return _lifetime;
    }

protected:
    void resizeEvent(QResizeEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private:
    void updateControls();
    void updatePreview();
    void sendReport();
    void networkSettings();
    void onSendError(QNetworkReply::NetworkError e);
    void onSendFinished();

    rpl::event_stream<MTP::ProxyData> _proxyChanges;
    rpl::lifetime _lifetime;

    QByteArray _dumpraw;
    QString _fullLog;
    QString _recentLog;

    PreLaunchLabel _header;
    PreLaunchLabel _description;
    
    QRadioButton *_radioRecent = nullptr;
    QRadioButton *_radioFull = nullptr;
    
    PreLaunchLog _preview;
    
    PreLaunchButton _networkSettings;
    PreLaunchButton _send;
    PreLaunchButton _continue;
    
    QNetworkAccessManager _manager;
    QNetworkReply *_reply = nullptr;
    
    Fn<void()> _launch;
    bool _sent = false;
};

} // namespace Alex
