#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <Qt>

class QAction;
class QDockWidget;
class QEvent;
class QMainWindow;
class QSettings;
class QWidget;
class PanelTitleBar;

class PanelLayoutController final : public QObject
{
    Q_OBJECT

public:
    enum class DefaultFloatingPosition {
        Bottom,
        Right,
        Left
    };

    explicit PanelLayoutController(QMainWindow *window);

    void addPanel(QDockWidget *dock,
                  QAction *floatingAction,
                  const QString &settingsId,
                  DefaultFloatingPosition defaultPosition,
                  Qt::DockWidgetArea homeArea,
                  bool closableWhenUnlocked = false);
    void setLocked(bool locked);
    void setPlacement(QDockWidget *dock, const QString &placement);
    [[nodiscard]] QString placementName(QDockWidget *dock) const;
    [[nodiscard]] bool isOverlay(QDockWidget *dock) const;
    void restore(QSettings &settings);
    void save(QSettings &settings) const;
    void resetFloatingPanels();

signals:
    void placementChanged(QDockWidget *dock,
                          const QString &placement);
    void layoutStateChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class PeerAttachment {
        None,
        Above,
        Below,
        LeftOf,
        RightOf
    };

    struct PanelEntry {
        QPointer<QDockWidget> dock;
        QPointer<QAction> floatingAction;
        QPointer<PanelTitleBar> titleBar;
        QPointer<QWidget> resizeHandle;
        QPointer<QWidget> borderOverlay;
        QString settingsId;
        DefaultFloatingPosition defaultPosition;
        Qt::DockWidgetArea homeArea;
        QRect overlayGeometry;
        QSize dockMinimumSize;
        QSize dockMaximumSize;
        bool overlay = false;
        bool anchorLeft = false;
        bool anchorRight = false;
        bool anchorTop = false;
        bool anchorBottom = false;
        bool anchorStateRestored = false;
        QString peerId;
        PeerAttachment peerAttachment = PeerAttachment::None;
        int peerOffset = 0;
        bool closableWhenUnlocked = false;
    };

    PanelEntry *entryFor(QObject *object);
    const PanelEntry *entryFor(const QObject *object) const;
    PanelEntry *entryForSettingsId(const QString &settingsId);
    const PanelEntry *entryForSettingsId(
        const QString &settingsId) const;
    void setFloating(PanelEntry &entry, bool floating);
    void enterOverlay(PanelEntry &entry);
    void leaveOverlay(PanelEntry &entry, Qt::DockWidgetArea area);
    void moveOverlay(PanelEntry &entry, const QPoint &topLeft);
    void resizeOverlay(PanelEntry &entry, const QSize &size);
    void constrainOverlay(PanelEntry &entry);
    QRect snapOverlayGeometry(PanelEntry &entry,
                              const QRect &geometry);
    bool snapOverlayToPeer(PanelEntry &entry, QRect &geometry);
    QRect peerAttachedGeometry(const PanelEntry &entry,
                               const PanelEntry &peer) const;
    void clearPeerAttachment(PanelEntry &entry);
    bool wouldCreatePeerCycle(const PanelEntry &entry,
                              const PanelEntry &peer) const;
    void scheduleOverlayReflow();
    void reflowOverlays();
    QRect anchoredOverlayGeometry(const PanelEntry &entry,
                                  const QRect &geometry) const;
    QRect overlayBounds() const;
    QString overlayAnchorName(const PanelEntry &entry) const;
    void restoreOverlayAnchors(PanelEntry &entry,
                               const QString &anchors);
    QString peerAttachmentName(PeerAttachment attachment) const;
    PeerAttachment peerAttachmentFromName(const QString &name) const;
    QRect defaultOverlayGeometry(const PanelEntry &entry) const;
    QRect boundedOverlayGeometry(const QRect &geometry) const;
    void updateResizeHandle(PanelEntry &entry);
    void updatePanelBorder(PanelEntry &entry);
    void positionDefault(PanelEntry &entry);
    void restorePanel(PanelEntry &entry, bool floating,
                      const QByteArray &geometry);

    QPointer<QMainWindow> m_window;
    QPointer<QWidget> m_overlayRoot;
    QList<PanelEntry> m_panels;
    bool m_restoring = false;
    bool m_locked = false;
    bool m_overlayReflowScheduled = false;
    bool m_reflowingOverlays = false;
};
