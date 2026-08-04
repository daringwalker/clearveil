#include "panellayoutcontroller.h"

#include "paneltitlebar.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDockWidget>
#include <QEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSettings>
#include <QSignalBlocker>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <utility>

namespace {
constexpr auto floatingGeometryProperty =
    "clearveilLastFloatingGeometry";
constexpr auto placementTransitionProperty =
    "clearveilPanelPlacementTransition";
constexpr int screenMargin = 12;
constexpr int overlayMargin = 0;
constexpr int overlaySnapDistance = 18;
constexpr QSize minimumOverlaySize(180, 140);

class OverlayResizeHandle final : public QWidget
{
public:
    explicit OverlayResizeHandle(
        QWidget *parent,
        std::function<void(const QSize &)> resizeCallback)
        : QWidget(parent), m_resizeCallback(std::move(resizeCallback))
    {
        setObjectName(QStringLiteral("overlayPanelResizeHandle"));
        setAccessibleName(QCoreApplication::translate(
            "PanelLayoutController", "Resize panel"));
        setFixedSize(20, 20);
        setCursor(Qt::SizeFDiagCursor);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(event);
            return;
        }
        m_pressGlobal = event->globalPosition().toPoint();
        m_pressSize = parentWidget()
            ? parentWidget()->size() : QSize();
        event->accept();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!(event->buttons() & Qt::LeftButton)
            || !m_resizeCallback) {
            QWidget::mouseMoveEvent(event);
            return;
        }
        const QPoint delta = event->globalPosition().toPoint()
            - m_pressGlobal;
        m_resizeCallback(m_pressSize + QSize(delta.x(), delta.y()));
        event->accept();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(palette().color(QPalette::Mid), 1.5));
        for (int offset = 4; offset <= 12; offset += 4) {
            painter.drawLine(width() - offset, height() - 2,
                             width() - 2, height() - offset);
        }
    }

private:
    std::function<void(const QSize &)> m_resizeCallback;
    QPoint m_pressGlobal;
    QSize m_pressSize;
};

class PanelBorderOverlay final : public QWidget
{
public:
    explicit PanelBorderOverlay(QWidget *parent)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("floatingPanelBorder"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QColor outline = palette().color(QPalette::Mid);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(outline, 1));
        painter.drawRoundedRect(
            rect().adjusted(0, 0, -1, -1), 4, 4);
    }
};
}

PanelLayoutController::PanelLayoutController(
    QMainWindow *window)
    : QObject(window), m_window(window),
      m_overlayRoot(window ? window->centralWidget() : nullptr)
{
    if (m_overlayRoot)
        m_overlayRoot->installEventFilter(this);
}

void PanelLayoutController::addPanel(
    QDockWidget *dock, QAction *floatingAction,
    const QString &settingsId,
    DefaultFloatingPosition defaultPosition,
    Qt::DockWidgetArea homeArea,
    bool closableWhenUnlocked)
{
    if (!dock || !floatingAction || settingsId.isEmpty())
        return;
    for (const PanelEntry &entry : std::as_const(m_panels)) {
        if (entry.dock == dock || entry.settingsId == settingsId)
            return;
    }

    auto *titleBar = new PanelTitleBar(dock, floatingAction, dock);
    titleBar->setObjectName(
        QStringLiteral("panelDragHandle_") + settingsId);
    dock->setTitleBarWidget(titleBar);
    PanelEntry entry;
    entry.dock = dock;
    entry.floatingAction = floatingAction;
    entry.titleBar = titleBar;
    entry.settingsId = settingsId;
    entry.defaultPosition = defaultPosition;
    entry.homeArea = homeArea;
    entry.dockMinimumSize = dock->minimumSize();
    entry.dockMaximumSize = dock->maximumSize();
    entry.closableWhenUnlocked = closableWhenUnlocked;
    m_panels.append(entry);
    PanelEntry *storedEntry = &m_panels.last();
    auto *resizeHandle = new OverlayResizeHandle(
        dock, [this, dock](const QSize &size) {
        if (PanelEntry *current = entryFor(dock))
            resizeOverlay(*current, size);
    });
    resizeHandle->hide();
    storedEntry->resizeHandle = resizeHandle;
    auto *borderOverlay = new PanelBorderOverlay(dock);
    borderOverlay->hide();
    storedEntry->borderOverlay = borderOverlay;
    dock->installEventFilter(this);
    {
        const QSignalBlocker blocker(floatingAction);
        floatingAction->setChecked(dock->isFloating());
    }
    connect(floatingAction, &QAction::toggled,
            this, [this, dock](bool floating) {
        if (PanelEntry *entry = entryFor(dock)) {
            if (floating && entry->overlay)
                leaveOverlay(*entry, entry->homeArea);
            setFloating(*entry, floating);
        }
    });
    connect(titleBar, &PanelTitleBar::overlayMoveRequested,
            this, [this, dock](const QPoint &topLeft) {
        if (PanelEntry *entry = entryFor(dock))
            moveOverlay(*entry, topLeft);
    });
    connect(titleBar, &PanelTitleBar::overlayDockRequested,
            this, [this, dock] {
        if (PanelEntry *entry = entryFor(dock))
            leaveOverlay(*entry, entry->homeArea);
    });
    QPointer<QAction> guardedAction(floatingAction);
    QPointer<PanelTitleBar> guardedTitleBar(titleBar);
    connect(dock, &QDockWidget::topLevelChanged,
            this, [this, guardedAction,
                   guardedTitleBar](bool floating) {
        if (!guardedAction)
            return;
        const QSignalBlocker blocker(guardedAction);
        guardedAction->setChecked(floating);
        if (guardedTitleBar) {
            const PanelEntry *entry = entryFor(guardedTitleBar->parent());
            guardedTitleBar->setPresentation(
                m_locked, floating, entry && entry->overlay);
        }
        if (PanelEntry *entry = entryFor(guardedTitleBar
                                             ? guardedTitleBar->parent()
                                             : nullptr)) {
            updatePanelBorder(*entry);
        }
        if (!m_restoring)
            emit layoutStateChanged();
    });
    connect(dock, &QDockWidget::dockLocationChanged,
            this, [this, dock](Qt::DockWidgetArea area) {
        if (PanelEntry *entry = entryFor(dock);
            entry && !entry->overlay
            && area != Qt::NoDockWidgetArea) {
            entry->homeArea = area;
        }
    });
    titleBar->setPresentation(m_locked, dock->isFloating(), false);
}

void PanelLayoutController::setLocked(bool locked)
{
    m_locked = locked;
    for (PanelEntry &entry : m_panels) {
        QDockWidget *dock = entry.dock;
        if (!dock)
            continue;
        if (entry.titleBar) {
            entry.titleBar->setPresentation(
                locked, dock->isFloating(), entry.overlay);
        }
        if (entry.resizeHandle)
            entry.resizeHandle->setVisible(entry.overlay);
        updatePanelBorder(entry);
        if (entry.overlay) {
            dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
            updateResizeHandle(entry);
            continue;
        }
        if (locked) {
            dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
            continue;
        }
        QDockWidget::DockWidgetFeatures features =
            QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable;
        if (entry.closableWhenUnlocked)
            features |= QDockWidget::DockWidgetClosable;
        dock->setFeatures(features);
    }
}

void PanelLayoutController::setPlacement(
    QDockWidget *dock, const QString &placement)
{
    PanelEntry *entry = entryFor(dock);
    if (!entry || !dock || !m_window)
        return;

    if (placement == QStringLiteral("overlay")) {
        enterOverlay(*entry);
        return;
    }
    if (placement == QStringLiteral("floating")) {
        if (entry->overlay)
            leaveOverlay(*entry, entry->homeArea);
        setFloating(*entry, true);
        return;
    }

    Qt::DockWidgetArea area = Qt::BottomDockWidgetArea;
    if (placement == QStringLiteral("top"))
        area = Qt::TopDockWidgetArea;
    else if (placement == QStringLiteral("left"))
        area = Qt::LeftDockWidgetArea;
    else if (placement == QStringLiteral("right"))
        area = Qt::RightDockWidgetArea;
    entry->homeArea = area;
    if (entry->overlay) {
        leaveOverlay(*entry, area);
    } else {
        if (dock->isFloating())
            dock->setFloating(false);
        m_window->addDockWidget(area, dock);
    }
    emit placementChanged(dock, placementName(dock));
}

QString PanelLayoutController::placementName(
    QDockWidget *dock) const
{
    const PanelEntry *entry = entryFor(dock);
    if (!entry || !dock || !m_window)
        return {};
    if (entry->overlay)
        return QStringLiteral("overlay");
    if (dock->isFloating())
        return QStringLiteral("floating");
    switch (m_window->dockWidgetArea(dock)) {
    case Qt::TopDockWidgetArea:
        return QStringLiteral("top");
    case Qt::LeftDockWidgetArea:
        return QStringLiteral("left");
    case Qt::RightDockWidgetArea:
        return QStringLiteral("right");
    case Qt::BottomDockWidgetArea:
    default:
        return QStringLiteral("bottom");
    }
}

bool PanelLayoutController::isOverlay(QDockWidget *dock) const
{
    const PanelEntry *entry = entryFor(dock);
    return entry && entry->overlay;
}

void PanelLayoutController::restore(QSettings &settings)
{
    m_restoring = true;
    for (PanelEntry &entry : m_panels) {
        if (!entry.dock)
            continue;
        const QString prefix = QStringLiteral("layout/docks/")
            + entry.settingsId + QLatin1Char('/');
        QString storedMode = settings.value(
            prefix + QStringLiteral("mode")).toString();
        // A color picker is a transient tool palette, not a document panel.
        // Older releases docked it into a complete side column, leaving a
        // large empty strip below the compact content. Migrate that one old
        // default once; an explicit placement chosen after the migration is
        // preserved normally.
        if (entry.settingsId == QStringLiteral("colorPicker")) {
            const QString compactPlacementRevisionKey = prefix
                + QStringLiteral("compactPlacementRevision");
            if (settings.value(compactPlacementRevisionKey, 0).toInt()
                < 1) {
                if (storedMode.isEmpty()
                    || storedMode == QStringLiteral("docked")) {
                    storedMode = QStringLiteral("overlay");
                }
                settings.setValue(compactPlacementRevisionKey, 1);
            }
        }
        const int storedHomeArea = settings.value(
            prefix + QStringLiteral("homeArea"),
            static_cast<int>(entry.homeArea)).toInt();
        if (storedHomeArea == Qt::TopDockWidgetArea
            || storedHomeArea == Qt::BottomDockWidgetArea
            || storedHomeArea == Qt::LeftDockWidgetArea
            || storedHomeArea == Qt::RightDockWidgetArea) {
            entry.homeArea = static_cast<Qt::DockWidgetArea>(
                storedHomeArea);
        }
        const bool floating = settings.value(
            prefix + QStringLiteral("floating"),
            entry.dock->isFloating()).toBool();
        const QByteArray geometry = settings.value(
            prefix + QStringLiteral("geometry")).toByteArray();
        entry.overlayGeometry = settings.value(
            prefix + QStringLiteral("overlayGeometry")).toRect();
        const QString sizeRevisionKey = prefix
            + QStringLiteral("overlaySizeRevision");
        if (entry.settingsId == QStringLiteral("colorPicker")
            && settings.value(sizeRevisionKey, 0).toInt() < 3) {
            if (entry.overlayGeometry.isValid()) {
                entry.overlayGeometry.setSize(
                    entry.overlayGeometry.size().boundedTo(
                        QSize(280, 430))
                        .expandedTo(minimumOverlaySize));
            }
            settings.setValue(sizeRevisionKey, 3);
        }
        const QString anchorKey = prefix
            + QStringLiteral("overlayAnchors");
        entry.anchorStateRestored = settings.contains(anchorKey);
        if (entry.anchorStateRestored) {
            restoreOverlayAnchors(
                entry, settings.value(anchorKey).toString());
        }
        entry.peerId = settings.value(
            prefix + QStringLiteral("overlayPeerId")).toString();
        entry.peerAttachment = peerAttachmentFromName(
            settings.value(prefix
                           + QStringLiteral("overlayPeerAttachment"))
                .toString());
        entry.peerOffset = settings.value(
            prefix + QStringLiteral("overlayPeerOffset"), 0).toInt();
        if (entry.peerAttachment == PeerAttachment::None)
            entry.peerId.clear();
        if (storedMode == QStringLiteral("overlay")) {
            // QMainWindow's dock layout is not fully activated until the
            // first show. Removing and reparenting a dock while that layout
            // is still being restored can leave Qt's internal dock layout
            // with stale items and crash during QWidget::show() on Wayland.
            // Enter overlay mode on the first event-loop turn instead.
            QPointer<QDockWidget> guardedDock = entry.dock;
            const QString settingsId = entry.settingsId;
            QTimer::singleShot(0, this,
                               [this, guardedDock, settingsId] {
                PanelEntry *current = entryFor(guardedDock);
                if (!current || current->settingsId != settingsId)
                    return;
                enterOverlay(*current);
            });
        } else {
            restorePanel(entry,
                         storedMode == QStringLiteral("floating")
                             || (storedMode.isEmpty() && floating),
                         geometry);
        }
    }
    m_restoring = false;
}

void PanelLayoutController::save(QSettings &settings) const
{
    for (const PanelEntry &entry : m_panels) {
        QDockWidget *dock = entry.dock;
        if (!dock)
            continue;
        const QString prefix = QStringLiteral("layout/docks/")
            + entry.settingsId + QLatin1Char('/');
        settings.setValue(prefix + QStringLiteral("floating"),
                          dock->isFloating());
        settings.setValue(prefix + QStringLiteral("mode"),
                          entry.overlay
                              ? QStringLiteral("overlay")
                              : dock->isFloating()
                                  ? QStringLiteral("floating")
                                  : QStringLiteral("docked"));
        settings.setValue(prefix + QStringLiteral("homeArea"),
                          static_cast<int>(entry.homeArea));
        settings.setValue(prefix + QStringLiteral("overlayGeometry"),
                          entry.overlay ? dock->geometry()
                                        : entry.overlayGeometry);
        settings.setValue(prefix + QStringLiteral("overlayAnchors"),
                          overlayAnchorName(entry));
        settings.setValue(prefix + QStringLiteral("overlayPeerId"),
                          entry.peerId);
        settings.setValue(
            prefix + QStringLiteral("overlayPeerAttachment"),
            peerAttachmentName(entry.peerAttachment));
        settings.setValue(prefix + QStringLiteral("overlayPeerOffset"),
                          entry.peerOffset);
        QByteArray geometry = dock->property(
            floatingGeometryProperty).toByteArray();
        if (dock->isFloating())
            geometry = dock->saveGeometry();
        settings.setValue(prefix + QStringLiteral("geometry"),
                          geometry);
    }
}

void PanelLayoutController::resetFloatingPanels()
{
    if (!m_window)
        return;
    for (PanelEntry &entry : m_panels) {
        if (!entry.dock)
            continue;
        entry.dock->setProperty(floatingGeometryProperty,
                                QByteArray());
        entry.overlayGeometry = {};
        entry.anchorLeft = false;
        entry.anchorRight = false;
        entry.anchorTop = false;
        entry.anchorBottom = false;
        entry.anchorStateRestored = false;
        clearPeerAttachment(entry);
        if (entry.overlay)
            leaveOverlay(entry, entry.homeArea);
        entry.dock->setFloating(false);
        m_window->addDockWidget(entry.homeArea, entry.dock);
    }
}

bool PanelLayoutController::eventFilter(
    QObject *watched, QEvent *event)
{
    if (watched == m_overlayRoot
        && event->type() == QEvent::Resize) {
        scheduleOverlayReflow();
    }
    PanelEntry *entry = entryFor(watched);
    if (entry && entry->overlay && entry->dock
        && event->type() == QEvent::Resize) {
        entry->overlayGeometry = entry->dock->geometry();
        updateResizeHandle(*entry);
        const QRect bounded = boundedOverlayGeometry(
            entry->dock->geometry());
        if (bounded != entry->dock->geometry()) {
            scheduleOverlayReflow();
        }
        scheduleOverlayReflow();
    }
    if (entry && entry->dock
        && (event->type() == QEvent::Resize
            || event->type() == QEvent::PaletteChange
            || event->type() == QEvent::StyleChange)) {
        updatePanelBorder(*entry);
    }
    if (entry && entry->dock && entry->dock->isFloating()
        && !m_restoring
        && (event->type() == QEvent::Move
            || event->type() == QEvent::Resize)) {
        QPointer<QDockWidget> dock = entry->dock;
        QTimer::singleShot(0, this, [this, dock] {
            if (!dock || !dock->isFloating())
                return;
            dock->setProperty(floatingGeometryProperty,
                              dock->saveGeometry());
            if (!m_restoring)
                emit layoutStateChanged();
        });
    }
    return QObject::eventFilter(watched, event);
}

PanelLayoutController::PanelEntry *
PanelLayoutController::entryFor(QObject *object)
{
    for (PanelEntry &entry : m_panels) {
        if (entry.dock == object)
            return &entry;
    }
    return nullptr;
}

const PanelLayoutController::PanelEntry *
PanelLayoutController::entryFor(const QObject *object) const
{
    for (const PanelEntry &entry : m_panels) {
        if (entry.dock == object)
            return &entry;
    }
    return nullptr;
}

PanelLayoutController::PanelEntry *
PanelLayoutController::entryForSettingsId(const QString &settingsId)
{
    for (PanelEntry &entry : m_panels) {
        if (entry.settingsId == settingsId)
            return &entry;
    }
    return nullptr;
}

const PanelLayoutController::PanelEntry *
PanelLayoutController::entryForSettingsId(
    const QString &settingsId) const
{
    for (const PanelEntry &entry : m_panels) {
        if (entry.settingsId == settingsId)
            return &entry;
    }
    return nullptr;
}

void PanelLayoutController::enterOverlay(PanelEntry &entry)
{
    QDockWidget *dock = entry.dock;
    if (!dock || !m_window || !m_overlayRoot || entry.overlay)
        return;
    const bool wasVisible = dock->isVisible();
    dock->setProperty(placementTransitionProperty, true);
    if (dock->isFloating())
        dock->setFloating(false);
    m_window->removeDockWidget(dock);
    dock->hide();
    dock->setParent(m_overlayRoot);
    dock->setMinimumSize(minimumOverlaySize);
    dock->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    entry.overlay = true;
    {
        const QSignalBlocker blocker(entry.floatingAction);
        if (entry.floatingAction)
            entry.floatingAction->setChecked(false);
    }
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    if (entry.titleBar)
        entry.titleBar->setPresentation(m_locked, false, true);
    const QRect geometry = entry.overlayGeometry.isValid()
        ? entry.overlayGeometry : defaultOverlayGeometry(entry);
    dock->setGeometry(
        !entry.peerId.isEmpty()
            ? boundedOverlayGeometry(geometry)
            : entry.anchorStateRestored
                ? anchoredOverlayGeometry(entry, geometry)
                : snapOverlayGeometry(entry, geometry));
    entry.anchorStateRestored = true;
    entry.overlayGeometry = dock->geometry();
    if (entry.resizeHandle)
        entry.resizeHandle->show();
    updatePanelBorder(entry);
    updateResizeHandle(entry);
    if (wasVisible) {
        dock->show();
        dock->raise();
        if (entry.resizeHandle)
            entry.resizeHandle->raise();
    }
    dock->setProperty(placementTransitionProperty, false);
    emit placementChanged(dock, QStringLiteral("overlay"));
    if (!m_restoring)
        emit layoutStateChanged();
    scheduleOverlayReflow();
}

void PanelLayoutController::leaveOverlay(
    PanelEntry &entry, Qt::DockWidgetArea area)
{
    QDockWidget *dock = entry.dock;
    if (!dock || !m_window || !entry.overlay)
        return;
    const bool wasVisible = dock->isVisible();
    dock->setProperty(placementTransitionProperty, true);
    entry.overlayGeometry = dock->geometry();
    dock->hide();
    if (entry.resizeHandle)
        entry.resizeHandle->hide();
    entry.overlay = false;
    clearPeerAttachment(entry);
    for (PanelEntry &candidate : m_panels) {
        if (candidate.peerId == entry.settingsId)
            clearPeerAttachment(candidate);
    }
    dock->setParent(m_window);
    dock->setMinimumSize(entry.dockMinimumSize);
    dock->setMaximumSize(entry.dockMaximumSize);
    m_window->addDockWidget(area, dock);
    entry.homeArea = area;
    if (entry.titleBar)
        entry.titleBar->setPresentation(m_locked, false, false);
    if (wasVisible)
        dock->show();
    dock->setProperty(placementTransitionProperty, false);
    setLocked(m_locked);
    emit placementChanged(dock, placementName(dock));
    if (!m_restoring)
        emit layoutStateChanged();
}

void PanelLayoutController::moveOverlay(
    PanelEntry &entry, const QPoint &topLeft)
{
    if (!entry.overlay || !entry.dock)
        return;
    QRect geometry = entry.dock->geometry();
    geometry.moveTopLeft(topLeft);
    entry.dock->setGeometry(snapOverlayGeometry(entry, geometry));
    entry.overlayGeometry = entry.dock->geometry();
    if (!m_restoring)
        emit layoutStateChanged();
    scheduleOverlayReflow();
}

void PanelLayoutController::resizeOverlay(
    PanelEntry &entry, const QSize &size)
{
    if (!entry.overlay || !entry.dock)
        return;
    QRect geometry(entry.dock->pos(), size.expandedTo(minimumOverlaySize));
    entry.dock->setGeometry(snapOverlayGeometry(entry, geometry));
    entry.overlayGeometry = entry.dock->geometry();
    updateResizeHandle(entry);
    if (!m_restoring)
        emit layoutStateChanged();
    scheduleOverlayReflow();
}

void PanelLayoutController::constrainOverlay(PanelEntry &entry)
{
    if (!entry.overlay || !entry.dock)
        return;
    entry.dock->setGeometry(anchoredOverlayGeometry(
        entry, entry.dock->geometry()));
    entry.overlayGeometry = entry.dock->geometry();
    updateResizeHandle(entry);
}

QRect PanelLayoutController::defaultOverlayGeometry(
    const PanelEntry &entry) const
{
    if (!m_overlayRoot)
        return {};
    const QSize available = m_overlayRoot->size();
    QSize desired;
    QPoint topLeft(overlayMargin, overlayMargin);
    switch (entry.defaultPosition) {
    case DefaultFloatingPosition::Bottom:
        desired = QSize(std::min(720, available.width() - 2 * overlayMargin),
                        std::min(210, available.height() - 2 * overlayMargin));
        topLeft = QPoint(overlayMargin,
                         available.height() - desired.height()
                             - overlayMargin);
        break;
    case DefaultFloatingPosition::Right:
        desired = QSize(std::min(420, available.width() - 2 * overlayMargin),
                        std::min(560, available.height() - 2 * overlayMargin));
        topLeft = QPoint(available.width() - desired.width()
                             - overlayMargin,
                         overlayMargin);
        break;
    case DefaultFloatingPosition::Left:
        desired = QSize(std::min(280, available.width() - 2 * overlayMargin),
                        std::min(430, available.height() - 2 * overlayMargin));
        break;
    }
    return QRect(topLeft, desired.expandedTo(minimumOverlaySize));
}

QRect PanelLayoutController::boundedOverlayGeometry(
    const QRect &geometry) const
{
    const QRect bounds = overlayBounds();
    if (bounds.width() <= 0 || bounds.height() <= 0)
        return geometry;
    QSize size = geometry.size().expandedTo(minimumOverlaySize);
    size.setWidth(std::min(size.width(), bounds.width()));
    size.setHeight(std::min(size.height(), bounds.height()));
    QRect result(geometry.topLeft(), size);
    if (result.left() < bounds.left())
        result.moveLeft(bounds.left());
    if (result.top() < bounds.top())
        result.moveTop(bounds.top());
    if (result.right() > bounds.right())
        result.moveRight(bounds.right());
    if (result.bottom() > bounds.bottom())
        result.moveBottom(bounds.bottom());
    return result;
}

QRect PanelLayoutController::overlayBounds() const
{
    if (!m_overlayRoot)
        return {};
    return m_overlayRoot->rect().adjusted(
        overlayMargin, overlayMargin, -overlayMargin, -overlayMargin);
}

QRect PanelLayoutController::snapOverlayGeometry(
    PanelEntry &entry, const QRect &geometry)
{
    const QRect bounds = overlayBounds();
    if (bounds.width() <= 0 || bounds.height() <= 0)
        return geometry;

    QRect result = geometry;
    clearPeerAttachment(entry);
    if (snapOverlayToPeer(entry, result)) {
        entry.anchorLeft = false;
        entry.anchorRight = false;
        entry.anchorTop = false;
        entry.anchorBottom = false;
        return result;
    }
    const int leftDistance = std::abs(result.left() - bounds.left());
    const int rightDistance = std::abs(result.right() - bounds.right());
    const int topDistance = std::abs(result.top() - bounds.top());
    const int bottomDistance = std::abs(result.bottom() - bounds.bottom());

    entry.anchorLeft = leftDistance <= overlaySnapDistance
        && leftDistance <= rightDistance;
    entry.anchorRight = !entry.anchorLeft
        && rightDistance <= overlaySnapDistance;
    entry.anchorTop = topDistance <= overlaySnapDistance
        && topDistance <= bottomDistance;
    entry.anchorBottom = !entry.anchorTop
        && bottomDistance <= overlaySnapDistance;

    if (entry.anchorLeft)
        result.moveLeft(bounds.left());
    else if (entry.anchorRight)
        result.moveRight(bounds.right());
    if (entry.anchorTop)
        result.moveTop(bounds.top());
    else if (entry.anchorBottom)
        result.moveBottom(bounds.bottom());
    return boundedOverlayGeometry(result);
}

bool PanelLayoutController::snapOverlayToPeer(
    PanelEntry &entry, QRect &geometry)
{
    struct Candidate {
        PanelEntry *peer = nullptr;
        PeerAttachment attachment = PeerAttachment::None;
        QRect geometry;
        int distance = overlaySnapDistance + 1;
        int offset = 0;
    } best;

    const QRect bounds = overlayBounds();
    for (PanelEntry &peer : m_panels) {
        if (&peer == &entry || !peer.overlay || !peer.dock
            || !peer.dock->isVisible()
            || wouldCreatePeerCycle(entry, peer)) {
            continue;
        }
        const QRect target = peer.dock->geometry();
        const int horizontalOverlap = std::min(
            geometry.right(), target.right())
            - std::max(geometry.left(), target.left()) + 1;
        const int verticalOverlap = std::min(
            geometry.bottom(), target.bottom())
            - std::max(geometry.top(), target.top()) + 1;
        const int requiredHorizontalOverlap = std::min(
            40, std::min(geometry.width(), target.width()) / 4);
        const int requiredVerticalOverlap = std::min(
            40, std::min(geometry.height(), target.height()) / 4);

        const auto consider = [&](PeerAttachment attachment,
                                  int distance,
                                  const QRect &snapped,
                                  int offset) {
            if (distance > overlaySnapDistance
                || distance >= best.distance
                || !bounds.contains(snapped)) {
                return;
            }
            best = {&peer, attachment, snapped, distance, offset};
        };
        if (horizontalOverlap >= requiredHorizontalOverlap) {
            QRect below = geometry;
            below.moveTop(target.bottom() + 1);
            consider(PeerAttachment::Below,
                     std::abs(geometry.top() - (target.bottom() + 1)),
                     below, geometry.left() - target.left());
            QRect above = geometry;
            above.moveBottom(target.top() - 1);
            consider(PeerAttachment::Above,
                     std::abs(geometry.bottom() - (target.top() - 1)),
                     above, geometry.left() - target.left());
        }
        if (verticalOverlap >= requiredVerticalOverlap) {
            QRect right = geometry;
            right.moveLeft(target.right() + 1);
            consider(PeerAttachment::RightOf,
                     std::abs(geometry.left() - (target.right() + 1)),
                     right, geometry.top() - target.top());
            QRect left = geometry;
            left.moveRight(target.left() - 1);
            consider(PeerAttachment::LeftOf,
                     std::abs(geometry.right() - (target.left() - 1)),
                     left, geometry.top() - target.top());
        }
    }

    if (!best.peer)
        return false;
    geometry = best.geometry;
    entry.peerId = best.peer->settingsId;
    entry.peerAttachment = best.attachment;
    entry.peerOffset = best.offset;
    return true;
}

QRect PanelLayoutController::peerAttachedGeometry(
    const PanelEntry &entry, const PanelEntry &peer) const
{
    if (!entry.dock || !peer.dock)
        return {};
    QRect result = entry.dock->geometry();
    const QRect target = peer.dock->geometry();
    switch (entry.peerAttachment) {
    case PeerAttachment::Above:
        result.moveBottom(target.top() - 1);
        result.moveLeft(target.left() + entry.peerOffset);
        break;
    case PeerAttachment::Below:
        result.moveTop(target.bottom() + 1);
        result.moveLeft(target.left() + entry.peerOffset);
        break;
    case PeerAttachment::LeftOf:
        result.moveRight(target.left() - 1);
        result.moveTop(target.top() + entry.peerOffset);
        break;
    case PeerAttachment::RightOf:
        result.moveLeft(target.right() + 1);
        result.moveTop(target.top() + entry.peerOffset);
        break;
    case PeerAttachment::None:
        break;
    }
    return result;
}

void PanelLayoutController::clearPeerAttachment(PanelEntry &entry)
{
    entry.peerId.clear();
    entry.peerAttachment = PeerAttachment::None;
    entry.peerOffset = 0;
}

bool PanelLayoutController::wouldCreatePeerCycle(
    const PanelEntry &entry, const PanelEntry &peer) const
{
    const PanelEntry *current = &peer;
    for (int depth = 0; current && depth < m_panels.size(); ++depth) {
        if (current->settingsId == entry.settingsId)
            return true;
        if (current->peerId.isEmpty())
            return false;
        current = entryForSettingsId(current->peerId);
    }
    return current != nullptr;
}

void PanelLayoutController::scheduleOverlayReflow()
{
    if (m_overlayReflowScheduled || m_reflowingOverlays)
        return;
    m_overlayReflowScheduled = true;
    QTimer::singleShot(0, this, [this] {
        m_overlayReflowScheduled = false;
        reflowOverlays();
    });
}

void PanelLayoutController::reflowOverlays()
{
    if (m_reflowingOverlays)
        return;
    m_reflowingOverlays = true;
    bool changed = false;
    for (PanelEntry &entry : m_panels) {
        if (entry.overlay && entry.peerId.isEmpty()) {
            const QRect before = entry.dock
                ? entry.dock->geometry() : QRect();
            constrainOverlay(entry);
            changed = changed || (entry.dock
                && entry.dock->geometry() != before);
        }
    }
    for (int pass = 0; pass < m_panels.size(); ++pass) {
        for (PanelEntry &entry : m_panels) {
            if (!entry.overlay || !entry.dock || entry.peerId.isEmpty())
                continue;
            PanelEntry *peer = entryForSettingsId(entry.peerId);
            if (!peer || !peer->overlay || !peer->dock
                || !peer->dock->isVisible()) {
                clearPeerAttachment(entry);
                constrainOverlay(entry);
                changed = true;
                continue;
            }
            const QRect attached = peerAttachedGeometry(entry, *peer);
            const QRect bounded = boundedOverlayGeometry(attached);
            if (bounded != attached) {
                clearPeerAttachment(entry);
                constrainOverlay(entry);
                changed = true;
                continue;
            }
            changed = changed || entry.dock->geometry() != attached;
            entry.dock->setGeometry(attached);
            entry.overlayGeometry = attached;
            updateResizeHandle(entry);
        }
    }
    m_reflowingOverlays = false;
    if (changed && !m_restoring)
        emit layoutStateChanged();
}

QRect PanelLayoutController::anchoredOverlayGeometry(
    const PanelEntry &entry, const QRect &geometry) const
{
    const QRect bounds = overlayBounds();
    if (bounds.width() <= 0 || bounds.height() <= 0)
        return geometry;
    QRect result = geometry;
    if (entry.anchorLeft)
        result.moveLeft(bounds.left());
    else if (entry.anchorRight)
        result.moveRight(bounds.right());
    if (entry.anchorTop)
        result.moveTop(bounds.top());
    else if (entry.anchorBottom)
        result.moveBottom(bounds.bottom());
    return boundedOverlayGeometry(result);
}

QString PanelLayoutController::overlayAnchorName(
    const PanelEntry &entry) const
{
    QStringList anchors;
    if (entry.anchorLeft)
        anchors.append(QStringLiteral("left"));
    else if (entry.anchorRight)
        anchors.append(QStringLiteral("right"));
    if (entry.anchorTop)
        anchors.append(QStringLiteral("top"));
    else if (entry.anchorBottom)
        anchors.append(QStringLiteral("bottom"));
    return anchors.join(QLatin1Char(','));
}

void PanelLayoutController::restoreOverlayAnchors(
    PanelEntry &entry, const QString &anchors)
{
    const QStringList values = anchors.split(
        QLatin1Char(','), Qt::SkipEmptyParts);
    entry.anchorLeft = values.contains(QStringLiteral("left"));
    entry.anchorRight = !entry.anchorLeft
        && values.contains(QStringLiteral("right"));
    entry.anchorTop = values.contains(QStringLiteral("top"));
    entry.anchorBottom = !entry.anchorTop
        && values.contains(QStringLiteral("bottom"));
}

QString PanelLayoutController::peerAttachmentName(
    PeerAttachment attachment) const
{
    switch (attachment) {
    case PeerAttachment::Above:
        return QStringLiteral("above");
    case PeerAttachment::Below:
        return QStringLiteral("below");
    case PeerAttachment::LeftOf:
        return QStringLiteral("leftOf");
    case PeerAttachment::RightOf:
        return QStringLiteral("rightOf");
    case PeerAttachment::None:
    default:
        return {};
    }
}

PanelLayoutController::PeerAttachment
PanelLayoutController::peerAttachmentFromName(
    const QString &name) const
{
    if (name == QStringLiteral("above"))
        return PeerAttachment::Above;
    if (name == QStringLiteral("below"))
        return PeerAttachment::Below;
    if (name == QStringLiteral("leftOf"))
        return PeerAttachment::LeftOf;
    if (name == QStringLiteral("rightOf"))
        return PeerAttachment::RightOf;
    return PeerAttachment::None;
}

void PanelLayoutController::updateResizeHandle(PanelEntry &entry)
{
    if (!entry.resizeHandle || !entry.dock)
        return;
    entry.resizeHandle->move(
        std::max(0, entry.dock->width()
                        - entry.resizeHandle->width()),
        std::max(0, entry.dock->height()
                        - entry.resizeHandle->height()));
    if (entry.overlay)
        entry.resizeHandle->raise();
}

void PanelLayoutController::updatePanelBorder(PanelEntry &entry)
{
    if (!entry.borderOverlay || !entry.dock)
        return;
    const bool elevated = entry.overlay || entry.dock->isFloating();
    entry.borderOverlay->setGeometry(entry.dock->rect());
    entry.borderOverlay->setVisible(elevated);
    if (!elevated)
        return;
    entry.borderOverlay->raise();
    entry.borderOverlay->update();
    if (entry.resizeHandle && entry.overlay)
        entry.resizeHandle->raise();
}

void PanelLayoutController::setFloating(
    PanelEntry &entry, bool floating)
{
    QDockWidget *dock = entry.dock;
    if (!dock || dock->isFloating() == floating)
        return;
    if (!floating) {
        dock->setFloating(false);
        return;
    }

    const QByteArray savedGeometry = dock->property(
        floatingGeometryProperty).toByteArray();
    const bool popupActive = QApplication::activePopupWidget();
    dock->setAttribute(Qt::WA_ShowWithoutActivating, popupActive);
    dock->setFloating(true);
    dock->show();
    if (!popupActive)
        dock->raise();
    QPointer<QDockWidget> guardedDock(dock);
    const QString settingsId = entry.settingsId;
    QTimer::singleShot(0, this,
                       [this, guardedDock, savedGeometry,
                        settingsId] {
        if (!guardedDock || !guardedDock->isFloating())
            return;
        PanelEntry *current = entryFor(guardedDock);
        if (!current || current->settingsId != settingsId)
            return;
        if (!savedGeometry.isEmpty())
            guardedDock->restoreGeometry(savedGeometry);
        else
            positionDefault(*current);
        guardedDock->setProperty(floatingGeometryProperty,
                                 guardedDock->saveGeometry());
        guardedDock->setAttribute(Qt::WA_ShowWithoutActivating, false);
        if (!QApplication::activePopupWidget())
            guardedDock->raise();
    });
}

void PanelLayoutController::positionDefault(PanelEntry &entry)
{
    if (!entry.dock || !m_window)
        return;
    QScreen *targetScreen = m_window->screen();
    if (!targetScreen)
        targetScreen = QApplication::primaryScreen();
    if (!targetScreen)
        return;

    const QRect available = targetScreen->availableGeometry();
    QSize desired;
    switch (entry.defaultPosition) {
    case DefaultFloatingPosition::Bottom:
        desired = QSize(720, 190);
        break;
    case DefaultFloatingPosition::Right:
        desired = QSize(420, 560);
        break;
    case DefaultFloatingPosition::Left:
        desired = QSize(280, 450);
        break;
    }
    const int maximumWidth = std::max(1,
        available.width() - 2 * screenMargin);
    const int maximumHeight = std::max(1,
        available.height() - 2 * screenMargin);
    desired.setWidth(std::clamp(
        desired.width(), std::min(280, maximumWidth), maximumWidth));
    desired.setHeight(std::clamp(
        desired.height(), std::min(160, maximumHeight), maximumHeight));
    entry.dock->resize(desired);

    const QRect mainFrame = m_window->frameGeometry();
    int x = available.left() + screenMargin;
    int y = available.top() + screenMargin;
    switch (entry.defaultPosition) {
    case DefaultFloatingPosition::Bottom: {
        x = std::clamp(
            mainFrame.left() + 24,
            available.left() + screenMargin,
            available.right() - desired.width() + 1 - screenMargin);
        const int below = mainFrame.bottom() + screenMargin;
        y = below + desired.height() <= available.bottom() + 1
            ? below
            : mainFrame.bottom() - desired.height() - 24;
        break;
    }
    case DefaultFloatingPosition::Right: {
        const int right = mainFrame.right() + screenMargin;
        x = right + desired.width() <= available.right() + 1
            ? right
            : mainFrame.right() - desired.width() - 24;
        y = mainFrame.top() + 48;
        break;
    }
    case DefaultFloatingPosition::Left: {
        const int left = mainFrame.left() - desired.width()
            - screenMargin;
        x = left >= available.left()
            ? left : mainFrame.left() + 24;
        y = mainFrame.top() + 48;
        break;
    }
    }
    const int minimumX = available.left() + screenMargin;
    const int minimumY = available.top() + screenMargin;
    const int maximumX = std::max(
        minimumX,
        available.right() - desired.width() + 1 - screenMargin);
    const int maximumY = std::max(
        minimumY,
        available.bottom() - desired.height() + 1 - screenMargin);
    x = std::clamp(x, minimumX, maximumX);
    y = std::clamp(y, minimumY, maximumY);
    entry.dock->move(x, y);
}

void PanelLayoutController::restorePanel(
    PanelEntry &entry, bool floating,
    const QByteArray &geometry)
{
    QDockWidget *dock = entry.dock;
    if (!dock)
        return;
    dock->setProperty(floatingGeometryProperty, geometry);
    dock->setFloating(floating);
    if (!floating)
        return;

    bool restored = false;
    if (!geometry.isEmpty())
        restored = dock->restoreGeometry(geometry);
    const QRect panelFrame = dock->frameGeometry();
    bool onScreen = false;
    for (QScreen *candidate : QApplication::screens()) {
        const QRect overlap = candidate->availableGeometry()
            .intersected(panelFrame);
        if (overlap.width() >= 48 && overlap.height() >= 48) {
            onScreen = true;
            break;
        }
    }
    if (!restored || !onScreen)
        positionDefault(entry);
    dock->setProperty(floatingGeometryProperty,
                      dock->saveGeometry());
}
