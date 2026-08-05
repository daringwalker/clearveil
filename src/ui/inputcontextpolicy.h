#pragma once

class QKeyEvent;

class InputContextPolicy final
{
public:
    enum class Context {
        ColorPickerPinned,
        OcrTextSelection,
        FolderBrowser
    };

    [[nodiscard]] static bool claimsShortcut(
        Context context, const QKeyEvent &event,
        bool hasSelection = false);
};
