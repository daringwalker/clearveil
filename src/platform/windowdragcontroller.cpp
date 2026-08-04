#include "windowdragcontroller.h"

#include <QAction>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
#include <QChildEvent>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QSpinBox>
#include <QToolBar>
#include <QWindow>

WindowDragController::WindowDragController(
    QMainWindow *window, QObject *parent)
    : QObject(parent ? parent : window), m_window(window)
{
}

void WindowDragController::addDragSurface(QWidget *surface)
{
    if (!surface || m_dragSurfaces.contains(surface))
        return;
    m_dragSurfaces.append(surface);
    watchWidgetTree(surface);
}

bool WindowDragController::eventFilter(
    QObject *watched, QEvent *event)
{
    auto *watchedWidget = qobject_cast<QWidget *>(watched);
    if (event->type() == QEvent::ChildAdded && watchedWidget) {
        auto *childEvent = static_cast<QChildEvent *>(event);
        if (auto *child = qobject_cast<QWidget *>(childEvent->child()))
            watchWidgetTree(child);
        return QObject::eventFilter(watched, event);
    }

    if (!m_window
        || (event->type() != QEvent::MouseButtonPress
            && event->type() != QEvent::MouseButtonDblClick)) {
        return QObject::eventFilter(watched, event);
    }

    QWidget *surface = dragSurfaceFor(watchedWidget);
    auto *mouseEvent = static_cast<QMouseEvent *>(event);
    const QPoint surfacePosition = surface
        ? surface->mapFromGlobal(mouseEvent->globalPosition().toPoint())
        : QPoint();
    if (!surface || mouseEvent->button() != Qt::LeftButton
        || !isDraggablePosition(surface, surfacePosition)) {
        return QObject::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        if (m_window->isFullScreen())
            return QObject::eventFilter(watched, event);

        const bool maximize = !m_window->isMaximized();
        emit windowStateToggleRequested(maximize);
        if (maximize)
            m_window->showMaximized();
        else
            m_window->showNormal();
        mouseEvent->accept();
        return true;
    }

    emit systemMoveRequested();
    if (QWindow *windowHandle = m_window->windowHandle())
        windowHandle->startSystemMove();
    mouseEvent->accept();
    return true;
}

void WindowDragController::watchWidgetTree(QWidget *widget)
{
    if (!widget)
        return;

    widget->installEventFilter(this);
    const auto children = widget->findChildren<QWidget *>(
        QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        if (child->window() == widget->window())
            watchWidgetTree(child);
    }
}

QWidget *WindowDragController::dragSurfaceFor(QWidget *widget) const
{
    QWidget *eventWindow = widget ? widget->window() : nullptr;
    for (QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        for (const QPointer<QWidget> &surface : m_dragSurfaces) {
            if (surface == candidate
                && surface->window() == eventWindow) {
                return surface;
            }
        }
    }
    return nullptr;
}

bool WindowDragController::isDraggablePosition(
    QWidget *surface, const QPoint &position) const
{
    if (auto *menuBar = qobject_cast<QMenuBar *>(surface))
        return menuBar->actionAt(position) == nullptr;

    auto *toolBar = qobject_cast<QToolBar *>(surface);
    if (!toolBar)
        return false;

    QWidget *child = toolBar->childAt(position);
    while (child && child != toolBar) {
        if (qobject_cast<QAbstractButton *>(child)
            || qobject_cast<QAbstractSlider *>(child)
            || qobject_cast<QComboBox *>(child)
            || qobject_cast<QLineEdit *>(child)
            || qobject_cast<QSpinBox *>(child)) {
            return false;
        }
        child = child->parentWidget();
    }

    QAction *action = toolBar->actionAt(position);
    if (!action || action->isSeparator())
        return true;

    QWidget *actionWidget = toolBar->widgetForAction(action);
    return actionWidget
        && !qobject_cast<QAbstractButton *>(actionWidget)
        && !qobject_cast<QAbstractSlider *>(actionWidget)
        && !qobject_cast<QComboBox *>(actionWidget)
        && !qobject_cast<QLineEdit *>(actionWidget)
        && !qobject_cast<QSpinBox *>(actionWidget);
}
