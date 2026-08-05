#include "selectablelabel.h"

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QKeySequence>

SelectableLabel::SelectableLabel(QWidget *parent)
    : QLabel(parent)
{
    initializeTextInteraction();
}

SelectableLabel::SelectableLabel(const QString &text,
                                 QWidget *parent)
    : QLabel(text, parent)
{
    initializeTextInteraction();
}

void SelectableLabel::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::SelectAll)) {
        setSelection(0, text().size());
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Copy)) {
        const QString copyText = hasSelectedText()
            ? selectedText() : text();
        if (!copyText.isEmpty())
            QApplication::clipboard()->setText(copyText);
        event->accept();
        return;
    }
    QLabel::keyPressEvent(event);
}

void SelectableLabel::initializeTextInteraction()
{
    setTextInteractionFlags(Qt::TextSelectableByMouse
                            | Qt::TextSelectableByKeyboard);
    setFocusPolicy(Qt::ClickFocus);
    setCursor(Qt::IBeamCursor);
}
