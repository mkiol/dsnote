#ifndef WL_CLIPBOARD_H
#define WL_CLIPBOARD_H

#include <QString>

/**
 * @file wl-clipboard.h
 * @brief Wayland clipboard operations wrapper
 *
 * This header provides functions for interacting with the Wayland clipboard
 * using the wl-clipboard utilities (wl-copy and wl-paste). It supports both
 * system installations and Flatpak environments.
 */

/**
 * @brief Copies text to the Wayland clipboard
 *
 * This function uses the wl-copy utility to copy the provided text to the
 * Wayland clipboard. It automatically detects whether running in a Flatpak
 * environment and uses the appropriate path for the wl-copy executable.
 *
 * @param text The QString containing the text to copy to the clipboard
 * @return bool Returns true if the text was successfully copied, false
 * otherwise
 *
 * @note The function logs debug information including the executable path,
 *       exit code, and any error output
 * @warning Requires wl-copy to be installed (system-wide or in Flatpak
 * environment)
 *
 * Example usage:
 * @code
 * QString myText = "Hello, World!";
 * if (wl_copy_to_clipboard(myText)) {
 *     qDebug() << "Text copied successfully";
 * } else {
 *     qDebug() << "Failed to copy text";
 * }
 * @endcode
 */
bool wl_copy_to_clipboard(const QString& text);

/**
 * @brief Retrieves text from the Wayland clipboard
 *
 * This function uses the wl-paste utility to retrieve the current contents
 * of the Wayland clipboard. It automatically detects whether running in a
 * Flatpak environment and uses the appropriate path for the wl-paste
 * executable.
 *
 * @param outText Reference to a QString that will be populated with the
 * clipboard content
 * @return bool Returns true if text was successfully retrieved, false otherwise
 *
 * @note The function logs debug information including the executable path
 *       and the retrieved text content
 * @warning Requires wl-paste to be installed (system-wide or in Flatpak
 * environment)
 * @warning The output parameter is only modified if the function returns true
 *
 * Example usage:
 * @code
 * QString clipboardText;
 * if (wl_paste_clipboard(clipboardText)) {
 *     qDebug() << "Clipboard content:" << clipboardText;
 * } else {
 *     qDebug() << "Failed to retrieve clipboard content";
 * }
 * @endcode
 */
bool wl_paste_clipboard(QString& outText);

#endif