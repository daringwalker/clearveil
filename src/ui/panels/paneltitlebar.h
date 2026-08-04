#pragma once

#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QWidget>

class QAction;
class QDockWidget;
class QMouseEvent;
class QPaintEvent;
class QLabel;
class QHBoxLayout;
class QToolButton;

class PanelTitleBar final : public QWidget
{
    Q_OBJECT

public:
    PanelTitleBar(QDockWidget *dock, QAction *floatingAction,
                  QWidget *parent = nullptr);

    void setPresentation(bool locked, bool floating,
                         bool overlay = false);
    void addControl(QWidget *control);

signals:
    void overlayMoveRequested(const QPoint &topLeft);
    void overlayDockRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateToggleButton(bool floating);
    void startFloatingMove();

    QPointer<QDockWidget> m_dock;
    QPointer<QAction> m_floatingAction;
    QLabel *m_titleLabel = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QToolButton *m_toggleButton = nullptr;
    QPoint m_pressPosition;
    QPoint m_pressGlobalPosition;
    QRect m_pressDockGeometry;
    bool m_compact = false;
    bool m_locked = false;
    bool m_overlay = false;
};
