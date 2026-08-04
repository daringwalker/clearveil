#include "canvasappearancecontroller.h"

#include "imagecanvas.h"

#include <QAction>
#include <QSignalBlocker>

CanvasAppearanceController::CanvasAppearanceController(
    ImageCanvas *canvas, QAction *checkerboardAction,
    QObject *parent)
    : QObject(parent)
    , m_canvas(canvas)
    , m_checkerboardAction(checkerboardAction)
{
    Q_ASSERT(m_canvas);
    Q_ASSERT(m_checkerboardAction);
    m_checkerboardAction->setCheckable(true);
    connect(m_checkerboardAction, &QAction::toggled, this,
            &CanvasAppearanceController::setTransparencyCheckerboardVisible);
    setTransparencyCheckerboardVisible(
        m_checkerboardAction->isChecked());
}

bool CanvasAppearanceController::transparencyCheckerboardVisible() const
{
    return m_canvas
        && m_canvas->canvasAppearance().transparencyCheckerboardVisible;
}

void CanvasAppearanceController::setTransparencyCheckerboardVisible(
    bool visible)
{
    if (!m_canvas || !m_checkerboardAction)
        return;

    const bool changed =
        transparencyCheckerboardVisible() != visible;
    m_canvas->setTransparencyCheckerboardVisible(visible);
    if (m_checkerboardAction->isChecked() != visible) {
        const QSignalBlocker blocker(m_checkerboardAction);
        m_checkerboardAction->setChecked(visible);
    }
    if (changed)
        emit transparencyCheckerboardVisibleChanged(visible);
}
