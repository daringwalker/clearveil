#pragma once

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QList>

class QEvent;
class QMainWindow;
class QWidget;

class WindowDragController final : public QObject
{
    Q_OBJECT

public:
    explicit WindowDragController(
        QMainWindow *window, QObject *parent = nullptr);

    void addDragSurface(QWidget *surface);

signals:
    void systemMoveRequested();
    void windowStateToggleRequested(bool maximized);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void watchWidgetTree(QWidget *widget);
    [[nodiscard]] QWidget *dragSurfaceFor(QWidget *widget) const;
    [[nodiscard]] bool isDraggablePosition(
        QWidget *surface, const QPoint &position) const;

    QPointer<QMainWindow> m_window;
    QList<QPointer<QWidget>> m_dragSurfaces;
};
