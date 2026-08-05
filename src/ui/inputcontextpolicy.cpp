#include "inputcontextpolicy.h"

#include <QKeyEvent>
#include <QKeySequence>

namespace {
bool isArrowKey(int key)
{
    return key == Qt::Key_Left || key == Qt::Key_Right
        || key == Qt::Key_Up || key == Qt::Key_Down;
}
}

bool InputContextPolicy::claimsShortcut(
    Context context, const QKeyEvent &event,
    bool hasSelection)
{
    switch (context) {
    case Context::ColorPickerPinned:
        return isArrowKey(event.key())
            || event.key() == Qt::Key_Escape;
    case Context::OcrTextSelection:
        return event.matches(QKeySequence::SelectAll)
            || (hasSelection && event.key() == Qt::Key_Escape);
    case Context::FolderBrowser:
        if (event.modifiers() == Qt::AltModifier
            && (event.key() == Qt::Key_Left
                || event.key() == Qt::Key_Right)) {
            return true;
        }
        return event.key() == Qt::Key_Escape
            || event.key() == Qt::Key_Backspace
            || event.key() == Qt::Key_F2
            || event.key() == Qt::Key_Delete;
    }
    return false;
}
