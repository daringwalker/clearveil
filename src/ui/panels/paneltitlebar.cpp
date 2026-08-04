#include "paneltitlebar.h"

#include "clearveilicon.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QWindow>

PanelTitleBar::PanelTitleBar(
    QDockWidget *dock, QAction *floatingAction, QWidget *parent)
    : QWidget(parent), m_dock(dock), m_floatingAction(floatingAction)
{
    setAccessibleName(tr("Panel drag handle"));
    setCursor(Qt::SizeAllCursor);
    auto *layout = new QHBoxLayout(this);
    m_layout = layout;
    layout->setContentsMargins(8, 3, 4, 3);
    layout->setSpacing(6);
    m_titleLabel = new QLabel(dock ? dock->windowTitle() : QString(), this);
    m_titleLabel->setObjectName(QStringLiteral("floatingPanelTitle"));
    m_titleLabel->setSizePolicy(
        QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    layout->addWidget(m_titleLabel);
    layout->addStretch();
    m_toggleButton = new QToolButton(this);
    m_toggleButton->setAutoRaise(true);
    m_toggleButton->setFixedSize(24, 24);
    m_toggleButton->setIconSize(QSize(17, 17));
    m_toggleButton->setAccessibleName(tr("Change panel form"));
    layout->addWidget(m_toggleButton);
    connect(m_toggleButton, &QToolButton::clicked,
            this, [this] {
        if (m_overlay) {
            emit overlayDockRequested();
            return;
        }
        if (m_floatingAction)
            m_floatingAction->setChecked(
                !m_floatingAction->isChecked());
    });
    if (m_floatingAction) {
        connect(m_floatingAction, &QAction::toggled,
                this, &PanelTitleBar::updateToggleButton);
        updateToggleButton(m_floatingAction->isChecked());
    }
    if (dock) {
        connect(dock, &QDockWidget::windowTitleChanged,
                m_titleLabel, &QLabel::setText);
    }
    setFixedHeight(30);
}

void PanelTitleBar::addControl(QWidget *control)
{
    if (!control || !m_layout || control->parentWidget() != this)
        return;
    const int toggleIndex = m_layout->indexOf(m_toggleButton);
    m_layout->insertWidget(
        toggleIndex >= 0 ? toggleIndex : m_layout->count(), control);
}

void PanelTitleBar::setPresentation(
    bool locked, bool floating, bool overlay)
{
    m_locked = locked;
    m_overlay = overlay;
    m_compact = locked && !floating && !overlay;
    setFixedHeight(m_compact ? 4 : 30);
    m_titleLabel->setVisible((floating || overlay) && !m_compact);
    m_toggleButton->setVisible(!m_compact);
    setCursor(m_compact ? Qt::ArrowCursor : Qt::SizeAllCursor);
    if (m_dock) {
        m_dock->setProperty("clearveilFloating", floating);
        m_dock->setProperty("clearveilOverlay", overlay);
        const bool elevatedPanel = floating || overlay;
        m_dock->setAttribute(Qt::WA_StyledBackground,
                             elevatedPanel);
        m_dock->setAutoFillBackground(elevatedPanel);
        if (QWidget *content = m_dock->widget()) {
            content->setAttribute(Qt::WA_StyledBackground,
                                  elevatedPanel);
            content->setAutoFillBackground(elevatedPanel);
            content->update();
        }
        if (m_dock->style()) {
            m_dock->style()->unpolish(m_dock);
            m_dock->style()->polish(m_dock);
        }
        m_dock->update();
    }
    updateToggleButton(floating || overlay);
    update();
}

void PanelTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_overlay) {
        if (event->button() == Qt::LeftButton) {
            emit overlayDockRequested();
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    if (!m_compact && event->button() == Qt::LeftButton
        && m_floatingAction) {
        m_floatingAction->setChecked(!m_floatingAction->isChecked());
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void PanelTitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_overlay) {
        if (!(event->buttons() & Qt::LeftButton) || !m_dock) {
            QWidget::mouseMoveEvent(event);
            return;
        }
        if ((event->position().toPoint() - m_pressPosition)
                .manhattanLength()
            < QApplication::startDragDistance()) {
            return;
        }
        const QPoint delta = event->globalPosition().toPoint()
            - m_pressGlobalPosition;
        emit overlayMoveRequested(
            m_pressDockGeometry.topLeft() + delta);
        event->accept();
        return;
    }
    if (m_compact || !(event->buttons() & Qt::LeftButton)
        || !m_dock || !m_floatingAction) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    if ((event->position().toPoint() - m_pressPosition).manhattanLength()
        < QApplication::startDragDistance()) {
        return;
    }
    if (!m_dock->isFloating()) {
        m_floatingAction->setChecked(true);
        QTimer::singleShot(0, this, &PanelTitleBar::startFloatingMove);
    } else {
        startFloatingMove();
    }
    event->accept();
}

void PanelTitleBar::mousePressEvent(QMouseEvent *event)
{
    if (!m_compact && event->button() == Qt::LeftButton) {
        m_pressPosition = event->position().toPoint();
        m_pressGlobalPosition = event->globalPosition().toPoint();
        if (m_dock)
            m_pressDockGeometry = m_dock->geometry();
        if (m_dock && m_dock->isFloating())
            startFloatingMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void PanelTitleBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Window));
    painter.setPen(palette().color(QPalette::Mid));
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());
    if (m_compact)
        return;

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().color(QPalette::Mid));
    const int centerX = m_toggleButton && m_toggleButton->isVisible()
        ? m_toggleButton->geometry().left() - 12
        : width() / 2;
    const int centerY = height() / 2;
    for (int row = -1; row <= 1; ++row) {
        for (int column = -1; column <= 1; ++column) {
            painter.drawEllipse(
                QPointF(centerX + column * 5,
                        centerY + row * 5), 1.25, 1.25);
        }
    }
}

void PanelTitleBar::updateToggleButton(bool floating)
{
    if (!m_toggleButton)
        return;
    m_toggleButton->setIcon(ClearveilIcon::fromName(
        floating ? QStringLiteral("panel_dock")
                 : QStringLiteral("panel_float")));
    const QString text = floating
        ? tr("Dock panel") : tr("Float panel");
    m_toggleButton->setToolTip(text);
    m_toggleButton->setAccessibleName(text);
}

void PanelTitleBar::startFloatingMove()
{
    if (!m_dock || !m_dock->isFloating()
        || !m_dock->windowHandle()) {
        return;
    }
    m_dock->windowHandle()->startSystemMove();
}
