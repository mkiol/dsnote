#ifndef WL_CLIPBOARD_H
#define WL_CLIPBOARD_H

#include <QString>
#include <optional>

namespace wl_clipboard {
    bool set_clipboard(const QString& text);
    std::optional<QString> get_clipboard();
}

#endif
