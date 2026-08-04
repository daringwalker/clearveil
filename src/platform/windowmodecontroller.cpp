#include "windowmodecontroller.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QScreen>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QWindow>

#include <algorithm>

WindowModeController::WindowModeController(
    const Components &components, const Actions &actions,
    QObject *parent)
    : QObject(parent)
    , m_components(components)
    , m_actions(actions)
    , m_fitTimer(new QTimer(this))
{
    Q_ASSERT(m_components.window);
    m_fitTimer->setSingleShot(true);
    m_fitTimer->setInterval(0);
    connect(m_fitTimer, &QTimer::timeout,
            this, &WindowModeController::fitWindowToContent);
}

void WindowModeController::toggleFullscreen()
{
    QMainWindow *window = m_components.window;
    if (!window)
        return;

    if (window->isFullScreen()) {
        window->showNormal();
        if (m_normalGeometryBeforeFullscreen.isValid())
            window->setGeometry(m_normalGeometryBeforeFullscreen);
        if (m_wasMaximizedBeforeFullscreen)
            window->showMaximized();
        restorePreFullscreenComponentVisibility();
    } else {
        m_wasMaximizedBeforeFullscreen = window->isMaximized();
        m_normalGeometryBeforeFullscreen = window->isMaximized()
            ? window->normalGeometry() : window->geometry();
        m_toolbarVisibleBeforeFullscreen =
            m_components.toolbar && m_components.toolbar->isVisible();
        m_filmstripVisibleBeforeFullscreen =
            m_components.filmstrip && m_components.filmstrip->isVisible();
        m_statusBarVisibleBeforeFullscreen =
            m_components.statusBar && m_components.statusBar->isVisible();
        m_informationVisibleBeforeFullscreen =
            m_components.information && m_components.information->isVisible();
        m_fullscreenVisibilitySnapshotValid = true;
        window->showFullScreen();
        applyFullscreenComponentVisibility();
    }

    if (m_actions.fullscreen) {
        const QSignalBlocker blocker(m_actions.fullscreen);
        m_actions.fullscreen->setChecked(window->isFullScreen());
    }
    emit presentationChanged();
}

void WindowModeController::applyFullscreenComponentVisibility()
{
    QMainWindow *window = m_components.window;
    if (!window || !window->isFullScreen()
        || !m_fullscreenVisibilitySnapshotValid) {
        return;
    }

    m_applyingComponentVisibility = true;
    if (m_components.toolbar && m_actions.fullscreenToolbar) {
        m_components.toolbar->setVisible(
            m_toolbarVisibleBeforeFullscreen
            && m_actions.fullscreenToolbar->isChecked());
    }
    if (m_components.filmstrip && m_actions.fullscreenFilmstrip) {
        m_components.filmstrip->setVisible(
            m_filmstripVisibleBeforeFullscreen
            && m_actions.fullscreenFilmstrip->isChecked());
    }
    if (m_components.information && m_actions.fullscreenInformation) {
        m_components.information->setVisible(
            m_informationVisibleBeforeFullscreen
            && m_actions.fullscreenInformation->isChecked());
    }
    if (m_components.statusBar && m_actions.fullscreenStatusBar) {
        m_components.statusBar->setVisible(
            m_statusBarVisibleBeforeFullscreen
            && m_actions.fullscreenStatusBar->isChecked());
    }
    m_applyingComponentVisibility = false;
    emit presentationChanged();
}

void WindowModeController::restorePreFullscreenComponentVisibility()
{
    if (!m_fullscreenVisibilitySnapshotValid)
        return;

    m_applyingComponentVisibility = true;
    if (m_components.toolbar)
        m_components.toolbar->setVisible(m_toolbarVisibleBeforeFullscreen);
    if (m_components.filmstrip)
        m_components.filmstrip->setVisible(m_filmstripVisibleBeforeFullscreen);
    if (m_components.information) {
        m_components.information->setVisible(
            m_informationVisibleBeforeFullscreen);
    }
    if (m_components.statusBar)
        m_components.statusBar->setVisible(m_statusBarVisibleBeforeFullscreen);
    m_applyingComponentVisibility = false;
    m_fullscreenVisibilitySnapshotValid = false;
}

void WindowModeController::scheduleFitWindowToContent(
    const QSize &contentSize, bool restoreNormalWindow)
{
    QMainWindow *window = m_components.window;
    if (!window || !m_actions.fitWindowToImage
        || !m_actions.fitWindowToImage->isChecked()
        || !contentSize.isValid() || window->isFullScreen()) {
        return;
    }

    if (restoreNormalWindow && window->isMaximized())
        window->showNormal();
    m_pendingContentSize = contentSize;
    m_fitTimer->start();
}

void WindowModeController::fitWindowToContent()
{
    QMainWindow *window = m_components.window;
    if (!window || !m_components.content
        || !m_actions.fitWindowToImage
        || !m_actions.fitWindowToImage->isChecked()
        || !m_pendingContentSize.isValid()
        || window->isFullScreen() || window->isMaximized()) {
        return;
    }

    const QSize chromeSize(
        std::max(0, window->width() - m_components.content->width()),
        std::max(0, window->height() - m_components.content->height()));
    QSize desiredSize = m_pendingContentSize + chromeSize;

    QScreen *targetScreen = window->screen();
    if (!targetScreen)
        targetScreen = QApplication::primaryScreen();
    if (targetScreen) {
        const QSize frameOverhead(
            std::max(0, window->frameGeometry().width() - window->width()),
            std::max(0, window->frameGeometry().height() - window->height()));
        QSize maximumClient =
            targetScreen->availableGeometry().size() - frameOverhead;
        maximumClient.setWidth(
            std::max(window->minimumWidth(), maximumClient.width()));
        maximumClient.setHeight(
            std::max(window->minimumHeight(), maximumClient.height()));
        desiredSize = desiredSize.boundedTo(maximumClient);
    }
    desiredSize = desiredSize.expandedTo(window->minimumSize());
    if (desiredSize != window->size())
        window->resize(desiredSize);
}

void WindowModeController::applyWindowModeFlags()
{
    QMainWindow *window = m_components.window;
    if (!window || m_applyingWindowModeFlags
        || !m_actions.borderless || !m_actions.alwaysOnTop) {
        return;
    }

    Qt::WindowFlags flags = window->windowFlags();
    flags.setFlag(Qt::FramelessWindowHint,
                  m_actions.borderless->isChecked());
    flags.setFlag(Qt::WindowStaysOnTopHint,
                  m_actions.alwaysOnTop->isChecked());
    if (flags == window->windowFlags())
        return;

    const bool wasVisible = window->isVisible();
    const bool wasFullscreen = window->isFullScreen();
    const bool wasMaximized = window->isMaximized();
    const bool wasMinimized = window->isMinimized();
    const QRect previousGeometry = wasFullscreen || wasMaximized
        ? window->normalGeometry() : window->geometry();

    m_applyingWindowModeFlags = true;
    if (wasVisible)
        window->hide();
    if (QWindow *nativeWindow = window->windowHandle())
        nativeWindow->destroy();
    window->setWindowFlags(flags);
    if (wasVisible) {
        if (wasFullscreen) {
            window->showFullScreen();
            applyFullscreenComponentVisibility();
        } else if (wasMaximized) {
            window->showMaximized();
        } else if (wasMinimized) {
            window->showMinimized();
        } else {
            window->showNormal();
            if (previousGeometry.isValid())
                window->setGeometry(previousGeometry);
        }
    }
    m_applyingWindowModeFlags = false;
    if (m_actions.fullscreen) {
        const QSignalBlocker blocker(m_actions.fullscreen);
        m_actions.fullscreen->setChecked(window->isFullScreen());
    }
    emit presentationChanged();
}

bool WindowModeController::isApplyingComponentVisibility() const
{
    return m_applyingComponentVisibility;
}
