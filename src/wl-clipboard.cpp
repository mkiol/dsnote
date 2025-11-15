#include "wl-clipboard.h"

#include <QProcess>
#include <QStandardPaths>
#include <QTextCodec>

#include "logger.hpp"

const QString wl_copy_path = QStandardPaths::findExecutable("wl-copy");
const QString wl_paste_path = QStandardPaths::findExecutable("wl-paste");

bool wl_copy_to_clipboard(const QString& text) {
    LOGD("wl_copy_path: " << wl_copy_path.toStdString());

    QProcess wl_copy;
    wl_copy.start(wl_copy_path, QStringList() << text);

    if (!wl_copy.waitForStarted()) {
        LOGW("wl-copy failed to start");
        return false;
    }
    if (!wl_copy.waitForFinished()) {
        LOGW("wl-copy failed to finish");
        return false;
    }

    return wl_copy.exitCode() == 0;
}

bool wl_paste_clipboard(QString& outText) {
    LOGD("wl-paste_path: " << wl_paste_path.toStdString());

    QProcess wl_paste;
    wl_paste.start(wl_paste_path, QStringList());

    if (!wl_paste.waitForStarted()) {
        LOGW("wl-paste failed to start");
        return false;
    }
    if (!wl_paste.waitForFinished()) {
        LOGW("wl-paste failed to finish");
        return false;
    }

    outText = wl_paste.readAllStandardOutput();
    LOGD("wl-paste output: " << outText.toStdString());
    return wl_paste.exitCode() == 0;
}