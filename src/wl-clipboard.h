#ifndef WL_CLIPBOARD_H
#define WL_CLIPBOARD_H

#include <QString>
#include <optional>

namespace wl_clipboard {
    bool setClipboard(const QString& text);
    std::optional<QString> getClipboard();
}

#endif
