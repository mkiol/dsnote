#include "wl-clipboard.h"

#include <QProcess>
#include <QStandardPaths>
#include <QTextCodec>
#include <optional>

#include "logger.hpp"

bool wl_clipboard::set_clipboard(const QString& text) {
    static const auto wl_copy_path = QStandardPaths::findExecutable("wl-copy");
    if (wl_copy_path.isEmpty()) {
        LOGW("wl-copy not found");
        return false;
    }

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

std::optional<QString> wl_clipboard::get_clipboard() {
    static const auto wl_paste_path = QStandardPaths::findExecutable("wl-paste");
    if (wl_paste_path.isEmpty()) {
        LOGW("wl-paste not found");
        return std::nullopt;
    }

    QProcess wl_paste;
    wl_paste.start(wl_paste_path, QStringList());

    if (!wl_paste.waitForStarted()) {
        LOGW("wl-paste failed to start");
        return std::nullopt;
    }
    if (!wl_paste.waitForFinished()) {
        LOGW("wl-paste failed to finish");
        return std::nullopt;
    }

    if (wl_paste.exitCode() != 0) {
        LOGW("wl-paste failed to exit");
        return std::nullopt;
    }

    const auto output = wl_paste.readAllStandardOutput();
    return output;
}
