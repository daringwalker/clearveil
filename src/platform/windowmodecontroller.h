#pragma once

#include <QObject>
#include <QRect>
#include <QSize>

class QAction;
class QDockWidget;
class QMainWindow;
class QStatusBar;
class QTimer;
class QToolBar;
class QWidget;

class WindowModeController final : public QObject
{
    Q_OBJECT

public:
    struct Components {
        QMainWindow *window = nullptr;
        QWidget *content = nullptr;
        QToolBar *toolbar = nullptr;
        QDockWidget *filmstrip = nullptr;
        QDockWidget *information = nullptr;
        QStatusBar *statusBar = nullptr;
    };

    struct Actions {
        QAction *fullscreen = nullptr;
        QAction *fitWindowToImage = nullptr;
        QAction *borderless = nullptr;
        QAction *alwaysOnTop = nullptr;
        QAction *fullscreenToolbar = nullptr;
        QAction *fullscreenFilmstrip = nullptr;
        QAction *fullscreenStatusBar = nullptr;
        QAction *fullscreenInformation = nullptr;
    };

    explicit WindowModeController(const Components &components,
                                  const Actions &actions,
                                  QObject *parent = nullptr);

    void toggleFullscreen();
    void applyFullscreenComponentVisibility();
    void applyWindowModeFlags();
    void scheduleFitWindowToContent(const QSize &contentSize,
                                    bool restoreNormalWindow = false);

    [[nodiscard]] bool isApplyingComponentVisibility() const;

signals:
    void presentationChanged();

private:
    void restorePreFullscreenComponentVisibility();
    void fitWindowToContent();

    Components m_components;
    Actions m_actions;
    QTimer *m_fitTimer = nullptr;
    QSize m_pendingContentSize;
    bool m_applyingWindowModeFlags = false;
    bool m_applyingComponentVisibility = false;
    bool m_fullscreenVisibilitySnapshotValid = false;
    bool m_toolbarVisibleBeforeFullscreen = false;
    bool m_filmstripVisibleBeforeFullscreen = false;
    bool m_statusBarVisibleBeforeFullscreen = false;
    bool m_informationVisibleBeforeFullscreen = false;
    bool m_wasMaximizedBeforeFullscreen = false;
    QRect m_normalGeometryBeforeFullscreen;
};
