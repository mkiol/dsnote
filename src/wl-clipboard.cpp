#include "wl-clipboard.h"

#include <QProcess>
#include <QStandardPaths>
#include <QTextCodec>

#include "logger.hpp"

// #ifdef WITH_FLATPAK
// const QString wl_copy_path = QString("/app/bin/wl-copy");
// const QString wl_paste_path = QString("/app/bin/wl-paste");
// #else
const QString wl_copy_path = QStandardPaths::findExecutable("wl-copy");
const QString wl_paste_path = QStandardPaths::findExecutable("wl-paste");
// #endif

bool wl_copy_to_clipboard(const QString& text) {
#ifdef WITH_FLATPAK
    LOGE("Using flatpak wl-copy");
#else
    LOGE("Using system wl-copy");
#endif
    LOGE("wl_copy_path: " << wl_copy_path.toStdString());

    QProcess wl_copy;
    wl_copy.start(wl_copy_path, QStringList() << text);

    if (!wl_copy.waitForStarted()) return false;
    if (!wl_copy.waitForFinished()) return false;

    LOGE("wl-copy exit code: " << wl_copy.exitCode());
    LOGE("wl-copy error: " << wl_copy.readAllStandardError().toStdString());
    LOGE("wl-copy output: " << wl_copy.readAllStandardOutput().toStdString());

    return wl_copy.exitCode() == 0;
}

bool wl_paste_clipboard(QString& outText) {
#ifdef WITH_FLATPAK
    LOGE("Using flatpak wl-paste");
#else
    LOGE("Using system wl-paste");
#endif

    LOGE("wl-paste_path: " << wl_paste_path.toStdString());

    QProcess wl_paste;
    wl_paste.start(wl_paste_path, QStringList());

    if (!wl_paste.waitForStarted()) return false;
    if (!wl_paste.waitForFinished()) return false;

    // Copy text from standard output to outText as UTF-16
    outText = wl_paste.readAllStandardOutput();
    LOGE("wl-paste output: " << outText.toStdString());
    // auto buffer = wl_paste.readAllStandardOutput();
    // outText = QTextCodec::codecForMib(1015)->toUnicode(buffer);
    return wl_paste.exitCode() == 0;
}