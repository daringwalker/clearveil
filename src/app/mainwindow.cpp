#include "mainwindow.h"

#include "aboutdialog.h"

#include "actionregistry.h"
#include "browserwidget.h"
#include "browserfileoperationscontroller.h"
#include "breezetheme.h"
#include "canvasappearancecontroller.h"
#include "clearveilicon.h"
#include "colorpickercontroller.h"
#include "colorpickerpanel.h"
#include "comparewidget.h"
#include "desktopintegration.h"
#include "directorymonitor.h"
#include "directoryscanservice.h"
#include "displaycolorcontroller.h"
#include "documentworkflowcontroller.h"
#include "editdialogs.h"
#include "formatcapabilities.h"
#include "formatcapabilitiesdialog.h"
#include "filmstripcontroller.h"
#include "filmstriplayoutcontroller.h"
#include "filmstripview.h"
#include "fileoperations.h"
#include "imagecanvas.h"
#include "imageeditcontroller.h"
#include "imageexportservice.h"
#include "imageloadcontroller.h"
#include "imagesequence.h"
#include "metadatapanel.h"
#include "ocrcontroller.h"
#include "ocrengine.h"
#include "ocrsupportdialog.h"
#include "panellayoutcontroller.h"
#include "paneltitlebar.h"
#include "persistentthumbnailcache.h"
#include "selectablelabel.h"
#include "selectablestatusbar.h"
#include "slideshowcontroller.h"
#include "systemappearancecontroller.h"
#include "thumbnailmodel.h"
#include "viewernavigationcontroller.h"
#include "vieweruistatecontroller.h"
#include "windowdragcontroller.h"
#include "windowmodecontroller.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QImageReader>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPalette>
#include <QPrintDialog>
#include <QPrinter>
#include <QResizeEvent>
#include <QSettings>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleFactory>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

namespace {
constexpr auto breezeColorSchemeProperty = "KDE_COLOR_SCHEME_PATH";

QString imageDialogFilter()
{
    return QObject::tr("Images (%1);;All files (*)")
        .arg(FormatCapabilities::imageDialogPatterns());
}

ThumbnailModel::SortKey thumbnailSortKey(
    const QString &key)
{
    if (key == QStringLiteral("modified"))
        return ThumbnailModel::SortKey::ModifiedTime;
    if (key == QStringLiteral("size"))
        return ThumbnailModel::SortKey::FileSize;
    if (key == QStringLiteral("type"))
        return ThumbnailModel::SortKey::FileType;
    return ThumbnailModel::SortKey::Name;
}

QIcon themedIcon(const QString &name, QStyle *style, QStyle::StandardPixmap fallback)
{
    return QIcon::fromTheme(name, style->standardIcon(fallback));
}

QString toolbarPositionName(Qt::ToolBarArea area)
{
    if (area == Qt::BottomToolBarArea)
        return QStringLiteral("bottom");
    if (area == Qt::LeftToolBarArea)
        return QStringLiteral("left");
    if (area == Qt::RightToolBarArea)
        return QStringLiteral("right");
    return QStringLiteral("top");
}

QString dockPositionName(Qt::DockWidgetArea area)
{
    if (area == Qt::TopDockWidgetArea)
        return QStringLiteral("top");
    if (area == Qt::LeftDockWidgetArea)
        return QStringLiteral("left");
    if (area == Qt::RightDockWidgetArea)
        return QStringLiteral("right");
    return QStringLiteral("bottom");
}

Qt::ToolBarArea toolbarAreaFromName(const QString &position)
{
    if (position == QStringLiteral("bottom"))
        return Qt::BottomToolBarArea;
    if (position == QStringLiteral("left"))
        return Qt::LeftToolBarArea;
    if (position == QStringLiteral("right"))
        return Qt::RightToolBarArea;
    return Qt::TopToolBarArea;
}

Qt::DockWidgetArea dockAreaFromName(const QString &position)
{
    if (position == QStringLiteral("top"))
        return Qt::TopDockWidgetArea;
    if (position == QStringLiteral("left"))
        return Qt::LeftDockWidgetArea;
    if (position == QStringLiteral("right"))
        return Qt::RightDockWidgetArea;
    return Qt::BottomDockWidgetArea;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_systemStyleName = QApplication::style()->objectName();
    m_systemPalette = QApplication::palette();
    const QVariant systemColorSchemePath =
        qApp->property(breezeColorSchemeProperty);
    m_systemColorSchemePathWasSet = systemColorSchemePath.isValid();
    m_systemColorSchemePath = systemColorSchemePath.toString();
    m_systemAppearanceController =
        new SystemAppearanceController(this);
    connect(m_systemAppearanceController,
            &SystemAppearanceController::colorSchemeChanged,
            this, [this] {
        if (m_settings.theme == QStringLiteral("system"))
            applyTheme(m_settings.theme);
    });
    setWindowTitle(tr("Clearveil"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/clearveil.svg")));
    setMinimumSize(820, 480);
    resize(1120, 760);

    m_editController = new ImageEditController(
        &m_document, &m_frames, this);
    m_documentWorkflowController =
        new DocumentWorkflowController(
            &m_document, &m_frames, this);
    m_directoryScanService =
        new DirectoryScanService(this);
    m_ocrController = new OcrController(this);
    m_ocrLanguages = OcrEngine::recognitionLanguages();
    m_browserFileOperationsController =
        new BrowserFileOperationsController(this, this);
    buildActions();
    buildActionRegistry();
    buildUi();
    m_uiStateController = new ViewerUiStateController(
        {
            m_previousAction, m_nextAction,
            m_slideshowAction, m_saveAsAction,
            m_copyAction, m_trashAction,
            {m_renameAction, m_copyFileAction,
             m_moveFileAction, m_revealAction,
             m_openWithAction},
            m_printAction, m_wallpaperAction,
            m_undoAction, m_redoAction,
            {m_rotateLeftAction, m_rotateRightAction,
             m_flipHorizontalAction, m_flipVerticalAction,
             m_cropAction, m_resizeAction,
             m_adjustAction, m_redEyeAction},
            {m_fitAction, m_fitWidthAction,
             m_fitHeightAction, m_fillAction,
             m_lockZoomAction, m_actualSizeAction,
             m_zoomInAction, m_zoomOutAction,
             m_fitWindowToImageAction},
            m_exportFrameAction, m_metadataAction,
            m_colorPickerAction, m_textSelectionAction,
            m_filmstripSourceAction
        },
        {this, m_fileLabel, m_detailLabel, m_zoomLabel},
        {
            tr("Ready"), tr("Clearveil"),
            tr("%1 — Clearveil"), tr("%1 image(s)"),
            tr("Clipboard image"), tr("Not saved"),
            tr("%1 × %2  ·  %3  ·  %4/%5"),
            tr(" · Locked"), tr("%1%2 — Clearveil")
        },
        this);
    connect(m_directoryScanService,
            &DirectoryScanService::scanFinished,
            this, &MainWindow::completeDirectoryScan);
    buildMenus();
    buildToolbar();
    m_windowDragController = new WindowDragController(this, this);
    m_windowDragController->addDragSurface(menuBar());
    m_windowDragController->addDragSurface(m_mainToolbar);
    const WindowModeController::Components windowComponents{
        this, m_canvas, m_mainToolbar, m_filmstripDock,
        m_metadataDock, statusBar()};
    const WindowModeController::Actions windowActions{
        m_fullscreenAction, m_fitWindowToImageAction,
        m_borderlessAction, m_alwaysOnTopAction,
        m_fullscreenToolbarAction, m_fullscreenFilmstripAction,
        m_fullscreenStatusBarAction,
        m_fullscreenInformationAction};
    m_windowModeController = new WindowModeController(
        windowComponents, windowActions, this);
    connect(m_windowModeController,
            &WindowModeController::presentationChanged,
            this, &MainWindow::updateMenuBarVisibility);
    m_displayColorController = new DisplayColorController(
        m_canvas, this, this);
    connect(m_displayColorController,
            &DisplayColorController::imageReady,
            this, [this] {
        if (!m_ocrRecognitionPending)
            return;
        m_ocrRecognitionPending = false;
        startOcrRecognition();
    });
    connect(m_ocrController, &OcrController::recognitionStarted,
            this, [this](quint64) {
        if (m_textSelectionAction
            && m_textSelectionAction->isChecked()) {
            statusBar()->showMessage(tr("Recognizing text…"));
        }
    });
    connect(m_ocrController, &OcrController::recognitionFinished,
            this, [this](quint64, const OcrResult &result) {
        if (!m_textSelectionAction
            || !m_textSelectionAction->isChecked()) {
            return;
        }
        if (!result.succeeded()) {
            statusBar()->showMessage(
                tr("Text recognition failed: %1")
                    .arg(result.error), 6000);
            return;
        }
        m_canvas->setOcrResult(result);
        if (result.symbols.isEmpty()) {
            statusBar()->showMessage(
                tr("No text was found in this image."), 4000);
        } else {
            statusBar()->showMessage(
                tr("Recognized %1 characters. Drag across text to select it.")
                    .arg(result.symbols.size()), 5000);
        }
    });
    connect(m_canvas, &ImageCanvas::ocrSelectionChanged,
            this, [this](bool hasSelection) {
        m_copyAction->setText(
            hasSelection ? tr("Copy selected text")
                         : tr("Copy image"));
        updateActions();
    });
    m_canvasAppearanceController =
        new CanvasAppearanceController(
            m_canvas, m_checkerboardAction, this);
    applyStyle();
    updateToolbarDensity();
    m_imageLoader = new ImageLoadController(this);
    connect(m_imageLoader, &ImageLoadController::loadStarted,
            this, [this](const QString &path, int) {
        if (m_openedFilmstripModel)
            m_openedFilmstripModel->setPrimaryImagePath(path);
        if (m_directoryFilmstripModel)
            m_directoryFilmstripModel->setPrimaryImagePath(path);
    });
    m_viewerNavigationController =
        new ViewerNavigationController(
            &m_session, m_imageLoader,
            m_documentWorkflowController,
            m_filmstripController, this);
    connect(m_viewerNavigationController,
            &ViewerNavigationController::loadingStarted,
            this, [this](const QString &path) {
        if (!displayedImage().isNull())
            updateNavigationActions();
        else
            updateActions();
        statusBar()->showMessage(
            tr("Loading %1…").arg(
                QFileInfo(path).fileName()));
    });
    connect(m_viewerNavigationController,
            &ViewerNavigationController::activationFailed,
            this, [this](const QString &path,
                         const QString &error) {
        if (m_openedFilmstripModel)
            m_openedFilmstripModel->setPrimaryImagePath({});
        if (m_directoryFilmstripModel)
            m_directoryFilmstripModel->setPrimaryImagePath({});
        updateActions();
        syncFilmstripSelection();
        statusBar()->clearMessage();
        QMessageBox::critical(
            this, tr("Cannot open image"),
            tr("Could not open “%1”.\n\n%2")
                .arg(QFileInfo(path).fileName(), error));
    });
    connect(m_viewerNavigationController,
            &ViewerNavigationController::activationSucceeded,
            this, [this](const QString &, int) {
        updateFrameControls();
        updateMetadataPanel();
        if (m_filmstripController
            && m_filmstripController->source()
                == FilmstripController::Source::CurrentDirectory) {
            rebuildFilmstrip();
        } else {
            syncFilmstripSelection();
        }
        updateActions();
        updateStatus();
        statusBar()->clearMessage();
    });

    m_slideshowController = new SlideshowController(this);
    connect(m_slideshowController,
            &SlideshowController::activateIndexRequested,
            this, [this](int index) {
        if (m_filmstripController
            && m_filmstripController->source()
                == FilmstripController::Source::CurrentDirectory) {
            m_filmstripController->selectRow(index);
        } else {
            setCurrentSequenceIndex(index);
        }
    });
    connect(m_slideshowController,
            &SlideshowController::runningChanged,
            this, [this](bool running) {
        if (!m_slideshowAction
            || m_slideshowAction->isChecked() == running) {
            return;
        }
        const QSignalBlocker blocker(m_slideshowAction);
        m_slideshowAction->setChecked(running);
    });
    connect(m_slideshowController,
            &SlideshowController::fullscreenRequested,
            this, [this](bool fullscreen) {
        if (fullscreen != isFullScreen())
            toggleFullscreen();
    });

    connect(&m_document, &ImageDocument::imageChanged,
            this, &MainWindow::documentChanged);
    connect(&m_document, &ImageDocument::historyChanged,
            this, &MainWindow::updateActions);
    connect(&m_document, &ImageDocument::modifiedChanged,
            this, [this](bool) { updateStatus(); });
    connect(&m_frames, &FrameController::frameChanged,
            this, [this](const QImage &image, int current, int count) {
        m_ocrController->cancel();
        m_canvas->clearOcrResult();
        m_ocrRecognitionPending = shouldRecognizeText();
        m_displayColorController->setImage(image, true);
        if (m_frameLabel) {
            m_frameLabel->setText(count > 0
                ? tr("%1 / %2").arg(current + 1).arg(count)
                : tr("%1 / ?").arg(current + 1));
        }
        m_frameFirstAction->setEnabled(current > 0);
        m_framePreviousAction->setEnabled(current > 0);
        m_frameNextAction->setEnabled(count > 0 && current + 1 < count);
        m_frameLastAction->setEnabled(count > 0 && current + 1 < count);
    });
    connect(&m_frames, &FrameController::stateChanged,
            this, &MainWindow::updateFrameControls);

    m_directoryMonitor = new DirectoryMonitor(this);
    connect(m_documentWorkflowController,
            &DocumentWorkflowController::currentFileChanged,
            this, [this](const QString &path) {
        m_directoryMonitor->setDirectory(
            path.isEmpty() ? QString()
                           : QFileInfo(path).absolutePath());
    });
    connect(m_documentWorkflowController,
            &DocumentWorkflowController::externalChangeBlocked,
            this, [this](const QString &) {
        statusBar()->showMessage(
            tr("The file changed on disk; reload is paused because you have unsaved changes."),
            6000);
    });
    connect(m_documentWorkflowController,
            &DocumentWorkflowController::externalReloadRequested,
            this, &MainWindow::reloadCurrentFile);
    connect(m_directoryMonitor,
            &DirectoryMonitor::refreshRequested,
            this, [this](const QString &directoryPath) {
        if (directoryPath != m_session.directoryPath())
            return;
        m_directoryScanService->invalidate(directoryPath);
        requestDirectoryScan(directoryPath, true);
    });

    updateActions();
    updateFrameControls();
    updateStatus();
    loadSettings();
}

SelectableStatusBar *MainWindow::statusBar() const
{
    return static_cast<SelectableStatusBar *>(
        QMainWindow::statusBar());
}

bool MainWindow::openPath(const QString &path)
{
    const QFileInfo info(path);
    if (info.isDir())
        return openDirectoryPath(info.absoluteFilePath());
    if (!confirmDiscardChanges())
        return false;
    cancelPendingDirectoryScan();

    QString error;
    if (!loadDocumentPath(info.absoluteFilePath(), &error)) {
        QMessageBox::critical(this, tr("Cannot open image"),
                              tr("Could not open “%1”.\n\n%2")
                                  .arg(info.fileName(), error));
        return false;
    }
    m_session.appendOpenedFiles({info.absoluteFilePath()});
    m_session.setLoadedPath(info.absoluteFilePath());
    rebuildFilmstrip();
    setFilmstripSource(false);
    prefetchAdjacentImages(m_session.currentOpenedIndex());
    updateActions();
    updateStatus();
    showViewer();
    present();
    return true;
}

bool MainWindow::openDirectoryPath(const QString &directoryPath)
{
    if (!confirmDiscardChanges())
        return false;

    const QFileInfo directory(directoryPath);
    if (!directory.isDir()) {
        statusBar()->showMessage(
            tr("The folder “%1” does not exist.")
                .arg(directory.fileName()),
            5000);
        return false;
    }

    requestDirectoryScan(
        directory.absoluteFilePath(), false, true);
    showViewer();
    setFilmstripSource(true);
    statusBar()->showMessage(
        tr("Scanning %1…").arg(directory.fileName()));
    present();
    return true;
}

void MainWindow::openBrowserImage(const QString &path)
{
    const QFileInfo image(path);
    if (!image.isFile() || !confirmDiscardChanges())
        return;
    requestDirectoryScan(
        image.absolutePath(), false, true,
        image.absoluteFilePath());
    showViewer();
    setFilmstripSource(true);
}

void MainWindow::requestDirectoryScan(
    const QString &directoryPath, bool forceRefresh,
    bool openImage, const QString &preferredImage)
{
    if (!m_directoryScanService)
        return;
    cancelPendingDirectoryScan();
    m_directoryScanPath =
        QDir(directoryPath).absolutePath();
    m_directoryScanPreferredImage = preferredImage;
    m_directoryScanOpensImage = openImage;
    m_directoryScanRequestId =
        m_directoryScanService->requestScan(
            m_directoryScanPath, forceRefresh);
}

void MainWindow::cancelPendingDirectoryScan()
{
    if (!m_directoryScanRequestId)
        return;
    if (m_directoryScanService) {
        m_directoryScanService->cancel(
            m_directoryScanRequestId);
    }
    m_directoryScanRequestId = 0;
    m_directoryScanPath.clear();
    m_directoryScanPreferredImage.clear();
    m_directoryScanOpensImage = false;
}

void MainWindow::completeDirectoryScan(
    quint64 requestId,
    const DirectoryScanResult &result)
{
    if (requestId != m_directoryScanRequestId)
        return;
    m_directoryScanRequestId = 0;
    const bool openImage = m_directoryScanOpensImage;
    const QString preferredImage =
        m_directoryScanPreferredImage;
    m_directoryScanPath.clear();
    m_directoryScanOpensImage = false;
    m_directoryScanPreferredImage.clear();

    if (!result.succeeded()) {
        statusBar()->showMessage(
            result.error.isEmpty()
                ? tr("Could not scan the folder.")
                : result.error,
            5000);
        return;
    }

    const QStringList imageFiles = result.imageFiles();
    if (openImage && imageFiles.isEmpty()) {
        if (displayedImage().isNull()) {
            m_session.setDirectoryFiles(
                result.directoryPath, imageFiles);
            m_directoryFilmstripModel->setDirectoryEntries(
                result.directoryPath, result.entries, false);
            if (isBrowseMode()
                && m_browser->directoryPath()
                    == result.directoryPath) {
                m_browser->applyDirectoryResult(result);
            }
            toggleFilmstrip();
            updateActions();
            updateStatus();
        }
        statusBar()->showMessage(
            tr("No supported images were found in “%1”.")
                .arg(QFileInfo(result.directoryPath).fileName()),
            5000);
        return;
    }
    m_session.setDirectoryFiles(
        result.directoryPath, imageFiles);
    m_directoryFilmstripModel->setDirectoryEntries(
        result.directoryPath, result.entries, false);
    if (isBrowseMode()
        && m_browser->directoryPath()
            == result.directoryPath) {
        m_browser->applyDirectoryResult(result);
    }
    syncFilmstripSelection();
    toggleFilmstrip();
    updateActions();
    updateStatus();

    if (!openImage)
        return;

    QString target = preferredImage;
    if (target.isEmpty()
        || !imageFiles.contains(target)) {
        target = m_directoryFilmstripModel->filePath(
            m_directoryFilmstripModel->index(0));
    }
    setFilmstripSource(true);
    requestDocumentPath(target, -1);
}

bool MainWindow::openPaths(const QStringList &paths)
{
    QStringList normalized;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (info.exists())
            normalized.append(info.absoluteFilePath());
    }
    if (normalized.isEmpty()) {
        present();
        return false;
    }
    if (normalized.size() == 1)
        return openPath(normalized.constFirst());
    if (!confirmDiscardChanges())
        return false;
    cancelPendingDirectoryScan();

    ImageSequence incoming;
    incoming.loadFiles(normalized);
    if (incoming.isEmpty()) {
        const QString firstFile = normalized.constFirst();
        QMessageBox::information(
            this, tr("Unsupported image format"),
            FormatCapabilities::friendlyDecodeError(
                firstFile, QImageReader::UnsupportedFormatError,
                tr("No compatible decoder was detected.")));
        return false;
    }
    m_session.appendOpenedFiles(incoming.files());

    QString error;
    const QString firstIncoming = incoming.at(0);
    if (!loadDocumentPath(firstIncoming, &error)) {
        QMessageBox::critical(this, tr("Cannot open image"), error);
        return false;
    }
    m_session.setLoadedPath(firstIncoming);
    rebuildFilmstrip();
    setFilmstripSource(false);
    prefetchAdjacentImages(m_session.currentOpenedIndex());
    updateActions();
    updateStatus();
    showViewer();
    present();
    return true;
}

void MainWindow::present()
{
    if (isMinimized())
        showNormal();
    show();
    raise();
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardChanges()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateToolbarDensity();
    positionCornerMenuButton();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape
        && m_colorPickerController
        && m_colorPickerController->isEnabled()
        && m_colorPickerController->isSamplePinned()) {
        m_colorPickerController->resumeSampling();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        toggleFullscreen();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::openFile()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Open image"), m_document.filePath(), imageDialogFilter());
    if (!files.isEmpty())
        openPaths(files);
}

void MainWindow::openFolder()
{
    const QString initial = m_document.filePath().isEmpty()
        ? QString() : QFileInfo(m_document.filePath()).absolutePath();
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Open folder"), initial);
    if (!folder.isEmpty())
        openPath(folder);
}

void MainWindow::pasteImage()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile() && openPath(url.toLocalFile()))
                return;
        }
    }
    if (!confirmDiscardChanges())
        return;

    QString error;
    if (!m_documentWorkflowController->loadClipboardImage(
            QApplication::clipboard()->image(), &error)) {
        QMessageBox::information(this, tr("Nothing to paste"), error);
        return;
    }
    m_session.clearOpenedFiles();
    rebuildFilmstrip();
    showViewer();
}

void MainWindow::copyImage()
{
    if (m_canvas && m_canvas->hasSelectedText()) {
        QApplication::clipboard()->setText(
            m_canvas->selectedText());
        statusBar()->showMessage(tr("Selected text copied."), 2500);
        return;
    }
    if (!displayedImage().isNull())
        QApplication::clipboard()->setImage(displayedImage());
}

void MainWindow::saveAs()
{
    if (m_document.image().isNull())
        return;
    QString initial = m_document.filePath();
    if (m_frames.isActive()) {
        const QFileInfo source(initial);
        initial = source.absolutePath() + QLatin1Char('/')
            + source.completeBaseName()
            + tr("-frame-%1.png").arg(m_frames.currentFrame() + 1);
    } else if (initial.isEmpty()) {
        initial = tr("Untitled.png");
    }
    const QString target = QFileDialog::getSaveFileName(
        this, tr("Save image as"), initial,
        tr("PNG image (*.png);;JPEG image (*.jpg *.jpeg);;WebP image (*.webp);;"
           "BMP image (*.bmp);;TIFF image (*.tif *.tiff)"));
    if (target.isEmpty())
        return;

    const QString previousPath = m_document.filePath();
    const ImageExportService::Result exportResult =
        m_documentWorkflowController->saveAs(target);
    if (!exportResult.succeeded()) {
        QMessageBox::critical(this, tr("Cannot save image"),
                              tr("Could not save the image.\n\n%1")
                                  .arg(exportResult.detail));
        return;
    }
    if (m_frames.isActive()) {
        statusBar()->showMessage(tr("Current frame exported."), 3500);
        return;
    }
    if (!previousPath.isEmpty())
        m_session.replaceOpenedFile(
            previousPath, exportResult.filePath);
    rebuildSequence(exportResult.filePath);
}

void MainWindow::cropImage()
{
    if (m_frames.isActive() || m_document.image().isNull())
        return;
    CropDialog dialog(m_document.image(), this);
    if (dialog.exec() == QDialog::Accepted)
        m_editController->crop(dialog.cropRectangle());
}

void MainWindow::resizeImage()
{
    if (m_frames.isActive() || m_document.image().isNull())
        return;
    ResizeDialog dialog(m_document.image().size(), this);
    if (dialog.exec() == QDialog::Accepted)
        m_editController->resize(dialog.targetSize());
}

void MainWindow::adjustImage()
{
    if (m_frames.isActive() || m_document.image().isNull())
        return;
    AdjustDialog dialog(m_document.image(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_editController->adjustColors(
            dialog.brightness(), dialog.contrast(),
            dialog.gamma());
    }
}

void MainWindow::reduceRedEye()
{
    if (m_frames.isActive() || m_document.image().isNull())
        return;
    CropDialog dialog(m_document.image(), this);
    dialog.setOperationText(tr("Red-eye correction"), tr("Correct red eye"));
    if (dialog.exec() == QDialog::Accepted) {
        const ImageEditController::Result result =
            m_editController->reduceRedEye(
                dialog.cropRectangle());
        if (result.error
            == ImageEditController::Error::NoChange) {
            statusBar()->showMessage(
                tr("No red-eye pixels were detected in the selected area."),
                4000);
        }
    }
}

void MainWindow::moveToTrash()
{
    const QString target = m_document.filePath();
    if (target.isEmpty() || !QFileInfo::exists(target))
        return;

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Move image to Trash"),
        tr("Move “%1” to the Trash?").arg(QFileInfo(target).fileName()),
        QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;

    const FileOperations::Result trashResult =
        FileOperations::moveToTrash(target);
    if (!trashResult.succeeded()) {
        QMessageBox::critical(this, tr("Cannot move image"),
                              tr("The image could not be moved to the Trash."));
        return;
    }

    cancelPendingImageLoad();
    const ImageSessionController::RemovalResult removal =
        m_session.removeOpenedPath(target, target);
    if (removal.openedImagesEmpty) {
        m_documentWorkflowController->clear();
        m_session.invalidateDirectory();
        rebuildFilmstrip();
        return;
    }

    QString error;
    if (!loadDocumentPath(removal.nextPath, &error)) {
        QMessageBox::critical(this, tr("Cannot open image"), error);
        m_documentWorkflowController->clear();
    } else {
        m_session.setLoadedPath(
            removal.nextPath, removal.nextIndex);
    }
    m_session.invalidateDirectory();
    rebuildFilmstrip();
    setFilmstripSource(false);
    updateActions();
    updateStatus();
}

void MainWindow::renameFile()
{
    if (m_document.filePath().isEmpty() || !confirmDiscardChanges())
        return;
    const QFileInfo source(m_document.filePath());
    bool accepted = false;
    const QString newName = QInputDialog::getText(
        this, tr("Rename image"), tr("New file name"), QLineEdit::Normal,
        source.fileName(), &accepted).trimmed();
    if (!accepted || newName.isEmpty() || newName == source.fileName())
        return;
    if (newName.contains(QLatin1Char('/'))) {
        QMessageBox::warning(this, tr("Invalid file name"),
                             tr("The file name cannot contain “/”."));
        return;
    }
    const QString target = source.dir().filePath(newName);
    if (QFileInfo::exists(target)) {
        QMessageBox::warning(this, tr("File already exists"),
                             tr("A file named “%1” already exists.").arg(newName));
        return;
    }

    m_frames.close();
    const FileOperations::Result renameResult =
        FileOperations::renameFile(
            source.absoluteFilePath(), newName);
    if (!renameResult.succeeded()) {
        QMessageBox::critical(this, tr("Cannot rename image"),
                              tr("The image could not be renamed."));
        return;
    }
    const QString renamedPath = renameResult.targetPath;
    m_session.replaceOpenedFile(
        source.absoluteFilePath(), renamedPath);
    QString error;
    if (!loadDocumentPath(renamedPath, &error)) {
        QMessageBox::critical(this, tr("Cannot open image"), error);
        return;
    }
    rebuildSequence(renamedPath);
    statusBar()->showMessage(tr("Image renamed."), 3000);
}

void MainWindow::copyFileTo()
{
    const QFileInfo source(m_document.filePath());
    if (!source.exists())
        return;
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Copy image to"), source.absolutePath());
    if (folder.isEmpty())
        return;
    const FileOperations::Result copyResult =
        FileOperations::copyToDirectory(
            source.absoluteFilePath(), folder);
    if (copyResult.error
            == FileOperations::Error::TargetExists
        || copyResult.isNoChange()) {
        QMessageBox::warning(this, tr("File already exists"),
                             tr("The destination already contains “%1”.")
                                 .arg(source.fileName()));
        return;
    }
    if (!copyResult.succeeded()) {
        QMessageBox::critical(this, tr("Cannot copy image"),
                              tr("The image could not be copied."));
        return;
    }
    statusBar()->showMessage(tr("Copied to %1").arg(folder), 4000);
}

void MainWindow::moveFileTo()
{
    if (m_document.filePath().isEmpty() || !confirmDiscardChanges())
        return;
    const QFileInfo source(m_document.filePath());
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Move image to"), source.absolutePath());
    if (folder.isEmpty() || QDir(folder) == source.dir())
        return;
    const QString target = QDir(folder).filePath(source.fileName());
    if (QFileInfo::exists(target)) {
        QMessageBox::warning(this, tr("File already exists"),
                             tr("The destination already contains “%1”.")
                                 .arg(source.fileName()));
        return;
    }

    m_frames.close();
    const FileOperations::Result moveResult =
        FileOperations::moveToDirectory(
            source.absoluteFilePath(), folder);
    if (!moveResult.succeeded()) {
        QMessageBox::critical(this, tr("Cannot move image"),
                              tr("The image could not be moved."));
        return;
    }
    const QString movedPath = moveResult.targetPath;
    m_session.replaceOpenedFile(
        source.absoluteFilePath(), movedPath);
    QString error;
    if (!loadDocumentPath(movedPath, &error)) {
        QMessageBox::critical(this, tr("Cannot open image"), error);
        return;
    }
    rebuildSequence(movedPath);
    statusBar()->showMessage(tr("Moved to %1").arg(folder), 4000);
}

void MainWindow::revealInFileManager()
{
    const FileOperations::Result result =
        FileOperations::revealInFileManager(
            m_document.filePath());
    if (!result.succeeded()) {
        QMessageBox::critical(
            this, tr("Cannot open file manager"),
            tr("The image location could not be opened."));
    }
}

void MainWindow::openWithApplication()
{
    const QString imagePath = m_document.filePath();
    if (imagePath.isEmpty())
        return;
    const QString application = QFileDialog::getOpenFileName(
        this, tr("Choose an application"), QStringLiteral("/usr/bin"));
    if (application.isEmpty())
        return;
    const FileOperations::Result result =
        FileOperations::launchApplication(
            application, imagePath);
    if (!result.succeeded()) {
        QMessageBox::critical(this, tr("Cannot open application"),
                              tr("The selected application could not be started."));
    }
}

void MainWindow::printImage()
{
    const QImage &image = displayedImage();
    if (image.isNull())
        return;
    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(QFileInfo(m_document.filePath()).fileName());
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const DesktopIntegration::Result result =
        DesktopIntegration::printImage(printer, image);
    if (!result.succeeded()) {
        QMessageBox::critical(this, tr("Cannot print"),
                              tr("The print job could not be started."));
    }
}

void MainWindow::setAsWallpaper()
{
    const DesktopIntegration::Result result =
        DesktopIntegration::requestWallpaper(
            m_document.filePath());
    if (result.error
        == DesktopIntegration::Error::ImageFileOpenFailed) {
        QMessageBox::critical(this, tr("Cannot set wallpaper"),
                              tr("The image could not be opened."));
        return;
    }
    if (result.error
        == DesktopIntegration::Error::WallpaperServiceUnavailable) {
        QMessageBox::critical(this, tr("Cannot set wallpaper"),
                              tr("The desktop wallpaper service is not available."));
        return;
    }
    if (!result.succeeded()) {
        QMessageBox::critical(this, tr("Cannot set wallpaper"),
                              result.detail);
        return;
    }
    statusBar()->showMessage(tr("Wallpaper request sent to the desktop."), 4000);
}

void MainWindow::showPreferences()
{
    showPreferencesPage(false);
}

void MainWindow::showInterfaceLayout()
{
    showPreferencesPage(true);
}

void MainWindow::showPreferencesPage(bool interfaceLayoutPage)
{
    const InterfaceLayoutState layoutState = currentInterfaceLayout();
    SettingsDialog dialog(m_settings.theme, m_settings.language,
                          layoutState.toolbarPosition,
                          layoutState.thumbnailsPlacement,
                          m_slideshowController->intervalMs() / 1000,
                          layoutState.showThumbnails,
                          m_settings.showFilmstripFileNames,
                          m_settings.filmstripThumbnailExtent,
                          m_settings.filmstripVerticalColumns,
                          m_settings.directoryThumbnailSortKey,
                          m_settings.directoryThumbnailSortAscending,
                          m_settings.randomSlideshow,
                          m_settings.fullscreenSlideshow,
                          m_settings.imageMemoryCacheMiB,
                          m_settings.persistentThumbnailCacheEnabled,
                          m_settings.persistentThumbnailCacheMiB,
                          PersistentThumbnailCache::sizeBytes(),
                          m_actionRegistry->toolbarItemDefinitions(),
                          m_settings.toolbarLayout,
                          m_actionRegistry->defaultToolbarLayout(),
                          m_actionRegistry->shortcutItemDefinitions(),
                          m_settings.shortcutLayout,
                          m_actionRegistry->defaultShortcutLayout(),
                          m_settings.wheelAction, m_settings.ctrlWheelAction,
                          m_settings.doubleClickAction,
                          m_settings.middleButtonAction,
                          m_settings.backButtonAction,
                          m_settings.forwardButtonAction,
                          layoutState, this);
    if (interfaceLayoutPage)
        dialog.showInterfaceLayoutPage();
    const auto applyDialogSettings = [this, &dialog] {
        const bool languageChanged =
            m_settings.language != dialog.language();
        applyPreferences(dialog);
        if (languageChanged) {
            statusBar()->showMessage(
                tr("Restart Clearveil to use the selected interface language."),
                5000);
        }
    };
    connect(&dialog, &SettingsDialog::applyRequested,
            this, applyDialogSettings);
    if (dialog.exec() == QDialog::Accepted)
        applyDialogSettings();
}

InterfaceLayoutState MainWindow::currentInterfaceLayout() const
{
    const auto placement = [this](QDockWidget *dock,
                                  const QString &fallback) {
        if (!dock)
            return fallback;
        if (m_panelLayoutController) {
            const QString managed =
                m_panelLayoutController->placementName(dock);
            if (!managed.isEmpty())
                return managed;
        }
        return dock->isFloating()
            ? QStringLiteral("floating")
            : dockPositionName(dockWidgetArea(dock));
    };

    InterfaceLayoutState result;
    result.showMenuBar = m_menuBarAction
        && m_menuBarAction->isChecked();
    result.showToolbar = m_mainToolbar
        && m_mainToolbar->isVisible();
    result.showStatusBar = m_statusBarAction
        && m_statusBarAction->isChecked();
    result.layoutLocked = m_layoutLocked;
    result.showToolbarInFullscreen = m_fullscreenToolbarAction
        && m_fullscreenToolbarAction->isChecked();
    result.showThumbnailsInFullscreen = m_fullscreenFilmstripAction
        && m_fullscreenFilmstripAction->isChecked();
    result.showStatusBarInFullscreen = m_fullscreenStatusBarAction
        && m_fullscreenStatusBarAction->isChecked();
    result.showInformationInFullscreen =
        m_fullscreenInformationAction
        && m_fullscreenInformationAction->isChecked();
    result.toolbarPosition = m_mainToolbar
        ? toolbarPositionName(toolBarArea(m_mainToolbar))
        : QStringLiteral("top");
    result.showThumbnails = m_filmstripDock
        && m_filmstripDock->isVisible();
    result.thumbnailsPlacement = placement(
        m_filmstripDock, QStringLiteral("bottom"));
    result.floatingThumbnailLayout =
        m_filmstripLayoutController
        ? m_filmstripLayoutController->modeName()
        : m_settings.floatingThumbnailLayout;
    result.showInformation = m_metadataDock
        && m_metadataDock->isVisible();
    result.informationPlacement = placement(
        m_metadataDock, QStringLiteral("right"));
    result.showColorPicker = m_colorPickerDock
        && m_colorPickerDock->isVisible();
    result.colorPickerPlacement = placement(
        m_colorPickerDock, QStringLiteral("overlay"));
    result.panelOrder = currentPanelOrder();
    return result;
}

void MainWindow::applyInterfaceLayout(
    const InterfaceLayoutState &layout)
{
    const auto applyPanelPlacement = [this](
        QDockWidget *dock, const QString &placement,
        Qt::DockWidgetArea fallback) {
        if (!dock)
            return;
        if (m_panelLayoutController) {
            m_panelLayoutController->setPlacement(
                dock, placement.isEmpty()
                          ? dockPositionName(fallback) : placement);
            return;
        }
        QAction *floatingAction = dock == m_filmstripDock
            ? m_floatFilmstripAction
            : dock == m_metadataDock
                ? m_floatMetadataAction : m_floatColorPickerAction;
        if (!floatingAction)
            return;
        if (placement == QStringLiteral("floating")) {
            floatingAction->setChecked(true);
            return;
        }
        floatingAction->setChecked(false);
        Qt::DockWidgetArea area = dockAreaFromName(placement);
        if (placement.isEmpty())
            area = fallback;
        addDockWidget(area, dock);
    };

    if (m_mainToolbar) {
        addToolBar(toolbarAreaFromName(layout.toolbarPosition),
                   m_mainToolbar);
        m_mainToolbar->setVisible(layout.showToolbar);
    }
    applyPanelPlacement(m_filmstripDock,
                        layout.thumbnailsPlacement,
                        Qt::BottomDockWidgetArea);
    applyPanelPlacement(m_metadataDock,
                        layout.informationPlacement,
                        Qt::RightDockWidgetArea);
    applyPanelPlacement(m_colorPickerDock,
                        layout.colorPickerPlacement,
                        Qt::RightDockWidgetArea);
    m_settings.panelOrder = layout.panelOrder;
    m_settings.normalize();
    applyPanelOrder(m_settings.panelOrder);

    if (m_filmstripAction)
        m_filmstripAction->setChecked(layout.showThumbnails);
    if (m_filmstripDock)
        m_filmstripDock->setVisible(layout.showThumbnails);
    if (m_metadataAction)
        m_metadataAction->setChecked(layout.showInformation);
    if (m_metadataDock)
        m_metadataDock->setVisible(layout.showInformation);
    if (m_colorPickerAction)
        m_colorPickerAction->setChecked(layout.showColorPicker);
    if (m_colorPickerController)
        m_colorPickerController->setEnabled(layout.showColorPicker);
    if (m_colorPickerDock)
        m_colorPickerDock->setVisible(layout.showColorPicker);
    if (m_menuBarAction)
        m_menuBarAction->setChecked(layout.showMenuBar);
    if (m_statusBarAction)
        m_statusBarAction->setChecked(layout.showStatusBar);
    if (m_fullscreenToolbarAction) {
        m_fullscreenToolbarAction->setChecked(
            layout.showToolbarInFullscreen);
    }
    if (m_fullscreenFilmstripAction) {
        m_fullscreenFilmstripAction->setChecked(
            layout.showThumbnailsInFullscreen);
    }
    if (m_fullscreenStatusBarAction) {
        m_fullscreenStatusBarAction->setChecked(
            layout.showStatusBarInFullscreen);
    }
    if (m_fullscreenInformationAction) {
        m_fullscreenInformationAction->setChecked(
            layout.showInformationInFullscreen);
    }

    m_settings.floatingThumbnailLayout =
        layout.floatingThumbnailLayout;
    if (m_filmstripLayoutController) {
        m_filmstripLayoutController->setModeName(
            m_settings.floatingThumbnailLayout);
    }
    updateFilmstripLayout(dockWidgetArea(m_filmstripDock));
    setLayoutLocked(layout.layoutLocked);
}

void MainWindow::applyPreferences(const SettingsDialog &dialog)
{
    m_settings.theme = dialog.theme();
    m_settings.language = dialog.language();
    applyInterfaceLayout(dialog.interfaceLayout());
    m_slideshowController->setIntervalMs(
        dialog.slideshowSeconds() * 1000);
    m_settings.showFilmstripFileNames =
        dialog.showFilmstripFileNames();
    m_settings.filmstripThumbnailExtent =
        dialog.filmstripThumbnailExtent();
    m_settings.filmstripVerticalColumns =
        dialog.filmstripVerticalColumns();
    m_settings.directoryThumbnailSortKey =
        dialog.directoryThumbnailSortKey();
    m_settings.directoryThumbnailSortAscending =
        dialog.directoryThumbnailSortAscending();
    applyFilmstripPreferences();
    m_settings.randomSlideshow = dialog.randomSlideshow();
    m_settings.fullscreenSlideshow = dialog.fullscreenSlideshow();
    m_slideshowController->setRandomOrder(
        m_settings.randomSlideshow);
    m_slideshowController->setFullscreenEnabled(
        m_settings.fullscreenSlideshow);
    m_settings.imageMemoryCacheMiB =
        dialog.imageMemoryCacheMiB();
    m_imageLoader->setCacheLimitMiB(
        m_settings.imageMemoryCacheMiB);
    m_settings.persistentThumbnailCacheEnabled =
        dialog.persistentThumbnailCacheEnabled();
    m_settings.persistentThumbnailCacheMiB =
        dialog.persistentThumbnailCacheMiB();
    PersistentThumbnailCache::configure(
        m_settings.persistentThumbnailCacheEnabled,
        static_cast<qint64>(
            m_settings.persistentThumbnailCacheMiB)
            * 1024LL * 1024LL);
    m_settings.toolbarLayout = m_actionRegistry->normalizedToolbarLayout(
        dialog.toolbarLayout());
    applyToolbarLayout();
    m_settings.shortcutLayout = m_actionRegistry->normalizedShortcutLayout(
        dialog.shortcutLayout());
    applyShortcuts();
    m_settings.wheelAction = dialog.wheelAction();
    m_settings.ctrlWheelAction =
        dialog.ctrlWheelAction();
    m_settings.doubleClickAction = dialog.doubleClickAction();
    m_settings.middleButtonAction = dialog.middleButtonAction();
    m_settings.backButtonAction = dialog.backButtonAction();
    m_settings.forwardButtonAction = dialog.forwardButtonAction();
    applyMouseActions();
    applyTheme(m_settings.theme);
    toggleFilmstrip();
    saveSettings();
}

void MainWindow::showFormatCapabilities()
{
    FormatCapabilitiesDialog dialog(this);
    dialog.exec();
}

void MainWindow::showOcrSupport()
{
    OcrSupportDialog dialog(this);
    dialog.exec();
}

void MainWindow::showAbout()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::previousImage()
{
    if (m_filmstripController
        && m_filmstripController->source()
            == FilmstripController::Source::CurrentDirectory
        && m_filmstripController) {
        const int row = m_filmstripController->currentRow();
        if (row > 0)
            m_filmstripController->selectRow(row - 1, false);
        return;
    }
    const int index = m_session.effectiveOpenedIndex();
    setCurrentSequenceIndex(index - 1);
}

void MainWindow::nextImage()
{
    if (m_filmstripController
        && m_filmstripController->source()
            == FilmstripController::Source::CurrentDirectory
        && m_filmstripController) {
        const int row = m_filmstripController->currentRow();
        if (row >= 0
            && row + 1 < m_filmstripController->count()) {
            m_filmstripController->selectRow(row + 1, false);
        }
        return;
    }
    const int index = m_session.effectiveOpenedIndex();
    setCurrentSequenceIndex(index + 1);
}

void MainWindow::toggleFullscreen()
{
    if (!m_windowModeController)
        return;
    const bool wasFullscreen = isFullScreen();
    m_windowModeController->toggleFullscreen();
    if (wasFullscreen)
        scheduleFitWindowToImage();
}

void MainWindow::toggleFilmstrip()
{
    if (!m_filmstripDock)
        return;
    const QSignalBlocker blocker(m_filmstripDock);
    const bool hasThumbnails = !m_session.openedFilesEmpty()
        || m_session.directoryCount() > 0;
    const bool viewerVisible = m_viewStack && m_canvas
        && m_viewStack->currentWidget() == m_canvas;
    m_filmstripDock->setVisible(m_filmstripAction->isChecked()
                                && hasThumbnails
                                && viewerVisible);
}

void MainWindow::toggleBrowseMode()
{
    if (m_browseAction->isChecked()) {
        QString directory;
        if (!m_document.filePath().isEmpty())
            directory = QFileInfo(m_document.filePath()).absolutePath();
        if (directory.isEmpty() && m_browser)
            directory = m_browser->directoryPath();
        if (directory.isEmpty())
            directory = QDir::homePath();
        showBrowser(directory);
    } else {
        showViewer();
    }
}

void MainWindow::toggleSlideshow()
{
    if (!m_slideshowController)
        return;
    syncSlideshowNavigationState();
    if (m_slideshowAction->isChecked()) {
        if (!m_slideshowController->start(isFullScreen())) {
            const QSignalBlocker blocker(m_slideshowAction);
            m_slideshowAction->setChecked(false);
        }
    } else {
        m_slideshowController->stop();
    }
}

void MainWindow::documentChanged()
{
    if (m_ocrController)
        m_ocrController->cancel();
    if (m_canvas)
        m_canvas->clearOcrResult();
    m_ocrRecognitionPending = shouldRecognizeText();
    if (m_document.isModified() && m_frames.isActive())
        m_frames.close();
    if (!m_document.filePath().isEmpty()
        && !displayedImage().isNull()) {
        if (m_openedFilmstripModel) {
            m_openedFilmstripModel->cacheThumbnail(
                m_document.filePath(), displayedImage());
        }
        if (m_directoryFilmstripModel) {
            m_directoryFilmstripModel->cacheThumbnail(
                m_document.filePath(), displayedImage());
        }
    }
    const bool frameActive = m_frames.isActive();
    m_displayColorController->setImage(
        displayedImage(), false,
        frameActive ? displayedImage().size()
                    : m_document.logicalSize(),
        frameActive ? ImageSourcePtr{}
                    : m_document.imageSource());
    if (m_colorPickerController)
        m_colorPickerController->resetForImage();
    scheduleFitWindowToImage();
    updateStatus();
    updateActions();
    updateMetadataPanel();
}

void MainWindow::updateActions()
{
    const bool hasImage = !displayedImage().isNull();
    updateNavigationActions();
    m_uiStateController->applyDocumentActions({
        hasImage,
        m_imageLoader && m_imageLoader->isLoading(),
        m_editController && m_editController->canEdit(),
        m_editController && m_editController->canUndo(),
        m_editController && m_editController->canRedo(),
        QFileInfo::exists(m_document.filePath()),
        m_frames.isActive(),
        m_session.directoryCount() > 0,
        !m_document.isRegionBacked()
    });
    if (m_canvas && m_canvas->hasSelectedText())
        m_copyAction->setEnabled(true);
    if (m_textSelectionAction)
        m_textSelectionAction->setEnabled(
            hasImage && !isBrowseMode());
    if (m_ocrDebugAction)
        m_ocrDebugAction->setEnabled(
            hasImage && !isBrowseMode());
}

void MainWindow::updateNavigationActions()
{
    if (m_filmstripController
        && m_filmstripController->source()
            == FilmstripController::Source::CurrentDirectory
        && m_filmstripController) {
        const int row = m_filmstripController->currentRow();
        m_uiStateController->applyNavigationActions(
            row, m_filmstripController->count());
        syncSlideshowNavigationState();
        return;
    }
    const int navigationIndex = m_session.effectiveOpenedIndex();
    m_uiStateController->applyNavigationActions(
        navigationIndex, m_session.openedCount());
    syncSlideshowNavigationState();
}

void MainWindow::buildUi()
{
    setStatusBar(new SelectableStatusBar(this));

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_viewStack = new QStackedWidget(central);
    m_canvas = new ImageCanvas(m_viewStack);
    m_browser = new BrowserWidget(
        m_directoryScanService, m_viewStack);
    m_compare = new CompareWidget(m_viewStack);
    m_viewStack->addWidget(m_canvas);
    m_viewStack->addWidget(m_browser);
    m_viewStack->addWidget(m_compare);
    layout->addWidget(m_viewStack, 1);

    auto *frameBar = new QFrame(central);
    frameBar->setObjectName(QStringLiteral("frameBar"));
    frameBar->setAccessibleName(tr("Frame navigation"));
    auto *frameLayout = new QHBoxLayout(frameBar);
    frameLayout->setContentsMargins(8, 4, 8, 4);
    frameLayout->setSpacing(4);
    frameLayout->addStretch();
    for (QAction *action : {m_frameFirstAction, m_framePreviousAction,
                            m_framePlayAction, m_frameNextAction,
                            m_frameLastAction}) {
        auto *button = new QToolButton(frameBar);
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        frameLayout->addWidget(button);
    }
    m_frameLabel = new SelectableLabel(frameBar);
    m_frameLabel->setObjectName(
        QStringLiteral("currentFrameLabel"));
    m_frameLabel->setAccessibleName(tr("Current frame"));
    m_frameLabel->setMinimumWidth(70);
    m_frameLabel->setAlignment(Qt::AlignCenter);
    frameLayout->addWidget(m_frameLabel);
    auto *exportButton = new QToolButton(frameBar);
    exportButton->setDefaultAction(m_exportFrameAction);
    exportButton->setAutoRaise(true);
    frameLayout->addWidget(exportButton);
    frameLayout->addStretch();
    m_frameBar = frameBar;
    m_frameBar->hide();
    layout->addWidget(m_frameBar);

    m_filmstripDock = new QDockWidget(QString(), this);
    m_filmstripDock->setObjectName(QStringLiteral("filmstripDock"));
    m_filmstripDock->setWindowTitle(tr("Thumbnails"));
    m_filmstripDock->setAccessibleName(tr("Thumbnails"));
    m_filmstripDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_filmstripDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    auto *filmstripPanel = new QWidget(m_filmstripDock);
    auto *filmstripLayout = new QVBoxLayout(filmstripPanel);
    filmstripLayout->setContentsMargins(0, 0, 0, 0);
    filmstripLayout->setSpacing(0);
    auto *filmstripView = new FilmstripView(filmstripPanel);
    m_filmstrip = filmstripView;
    m_filmstrip->setObjectName(QStringLiteral("filmstrip"));
    m_openedFilmstripModel = new ThumbnailModel(m_filmstrip);
    m_directoryFilmstripModel = new ThumbnailModel(m_filmstrip);
    m_openedFilmstripModel->setThumbnailSize(QSize(80, 64));
    m_directoryFilmstripModel->setThumbnailSize(QSize(80, 64));
    m_filmstripController = new FilmstripController(
        m_filmstrip, m_openedFilmstripModel,
        m_directoryFilmstripModel, this);
    m_filmstrip->setFlow(QListView::LeftToRight);
    m_filmstrip->setWrapping(false);
    m_filmstrip->setResizeMode(QListView::Fixed);
    m_filmstrip->setMovement(QListView::Static);
    m_filmstrip->setViewMode(QListView::IconMode);
    m_filmstrip->setUniformItemSizes(true);
    m_filmstrip->setItemAlignment({});
    m_filmstrip->setSelectionMode(QAbstractItemView::SingleSelection);
    m_filmstrip->setDragEnabled(true);
    m_filmstrip->setDragDropMode(QAbstractItemView::DragOnly);
    m_filmstrip->setDefaultDropAction(Qt::CopyAction);
    m_filmstrip->setIconSize(QSize(80, 64));
    m_filmstrip->setGridSize(QSize(94, 82));
    m_filmstrip->setMinimumHeight(86);
    m_filmstrip->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_filmstrip->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_filmstrip->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_filmstrip->setMouseTracking(true);
    m_filmstrip->setAccessibleName(tr("Opened images"));
    m_filmstripResizeTimer = new QTimer(this);
    m_filmstripResizeTimer->setSingleShot(true);
    m_filmstripResizeTimer->setInterval(100);
    connect(m_filmstripResizeTimer, &QTimer::timeout, this, [this] {
        if (m_pendingFilmstripThumbnailSize.isValid()) {
            if (m_openedFilmstripModel) {
                m_openedFilmstripModel->setThumbnailSize(
                    m_pendingFilmstripThumbnailSize);
            }
            if (m_directoryFilmstripModel) {
                m_directoryFilmstripModel->setThumbnailSize(
                    m_pendingFilmstripThumbnailSize);
            }
        }
    });
    filmstripView->setResizeHandler(
        [this] {
            if (m_filmstripLayoutController)
                m_filmstripLayoutController->updateLayout();
            updateFilmstripThumbnailSize();
        });
    filmstripView->setCloseHandler(
        [this](int row) { closeOpenedImageAt(row); });
    filmstripLayout->addWidget(m_filmstrip, 1);
    m_filmstripDock->setWidget(filmstripPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_filmstripDock);
    resizeDocks({m_filmstripDock}, {108}, Qt::Vertical);
    setCentralWidget(central);

    m_fileLabel = new SelectableLabel(this);
    m_fileLabel->setObjectName(
        QStringLiteral("currentFileLabel"));
    m_fileLabel->setAccessibleName(tr("Current image"));
    m_detailLabel = new SelectableLabel(this);
    m_detailLabel->setObjectName(
        QStringLiteral("imageDetailsLabel"));
    m_detailLabel->setAccessibleName(tr("Image details"));
    m_zoomLabel = new SelectableLabel(this);
    m_zoomLabel->setObjectName(
        QStringLiteral("zoomLevelLabel"));
    m_zoomLabel->setAccessibleName(tr("Zoom level"));
    m_fileLabel->setMinimumWidth(160);
    statusBar()->addWidget(m_fileLabel, 1);
    statusBar()->addPermanentWidget(m_detailLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->setPrimaryWidget(m_fileLabel);
    statusBar()->messageLabel()->setAccessibleName(
        tr("Status message"));

    m_metadataDock = new QDockWidget(QString(), this);
    m_metadataDock->setObjectName(QStringLiteral("metadataDock"));
    m_metadataDock->setWindowTitle(tr("Information"));
    m_metadataDock->setAccessibleName(tr("Image information"));
    m_metadataDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_metadataPanel = new MetadataPanel(m_metadataDock);
    m_metadataDock->setWidget(m_metadataPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_metadataDock);
    m_metadataDock->hide();

    m_colorPickerDock = new QDockWidget(QString(), this);
    m_colorPickerDock->setObjectName(
        QStringLiteral("colorPickerDock"));
    m_colorPickerDock->setWindowTitle(tr("Color picker"));
    m_colorPickerDock->setAccessibleName(
        tr("Color picker panel"));
    m_colorPickerDock->setAllowedAreas(
        Qt::AllDockWidgetAreas);
    m_colorPickerPanel =
        new ColorPickerPanel(m_colorPickerDock);
    m_colorPickerDock->setWidget(m_colorPickerPanel);
    m_colorPickerController = new ColorPickerController(
        m_canvas, m_colorPickerPanel, this);
    connect(m_colorPickerController,
            &ColorPickerController::copyTextRequested,
            this, [this](const QString &text,
                         const QString &format) {
        QApplication::clipboard()->setText(text);
        m_colorPickerPanel->showCopyConfirmation(format);
    });
    addDockWidget(Qt::RightDockWidgetArea, m_colorPickerDock);
    tabifyDockWidget(m_metadataDock, m_colorPickerDock);
    m_colorPickerDock->hide();

    m_panelLayoutController =
        new PanelLayoutController(this);
    m_panelLayoutController->addPanel(
        m_filmstripDock, m_floatFilmstripAction,
        QStringLiteral("filmstrip"),
        PanelLayoutController::DefaultFloatingPosition::Bottom,
        Qt::BottomDockWidgetArea);
    m_panelLayoutController->addPanel(
        m_metadataDock, m_floatMetadataAction,
        QStringLiteral("information"),
        PanelLayoutController::DefaultFloatingPosition::Right,
        Qt::RightDockWidgetArea);
    m_panelLayoutController->addPanel(
        m_colorPickerDock, m_floatColorPickerAction,
        QStringLiteral("colorPicker"),
        PanelLayoutController::DefaultFloatingPosition::Left,
        Qt::RightDockWidgetArea, true);
    connect(m_colorPickerPanel,
            &ColorPickerPanel::preferredHeightChanged,
            this, [this] {
        QTimer::singleShot(
            0, this,
            &MainWindow::updateColorPickerDockSizeConstraint);
    });
    connect(m_colorPickerDock, &QDockWidget::topLevelChanged,
            this, [this] {
        updateColorPickerDockSizeConstraint();
    });
    connect(m_colorPickerDock,
            &QDockWidget::dockLocationChanged,
            this, [this] {
        updateColorPickerDockSizeConstraint();
    });
    connect(m_panelLayoutController,
            &PanelLayoutController::placementChanged,
            this, [this](QDockWidget *dock, const QString &) {
        if (dock == m_colorPickerDock)
            updateColorPickerDockSizeConstraint();
    });
    QTimer::singleShot(
        0, this,
        &MainWindow::updateColorPickerDockSizeConstraint);
    auto *layoutSaveTimer = new QTimer(this);
    layoutSaveTimer->setObjectName(
        QStringLiteral("panelLayoutSaveTimer"));
    layoutSaveTimer->setSingleShot(true);
    layoutSaveTimer->setInterval(300);
    connect(m_panelLayoutController,
            &PanelLayoutController::layoutStateChanged,
            layoutSaveTimer,
            qOverload<>(&QTimer::start));
    connect(layoutSaveTimer, &QTimer::timeout,
            this, &MainWindow::saveSettings);
    m_filmstripLayoutController =
        new FilmstripLayoutController(
            m_filmstripDock, filmstripView,
            qobject_cast<PanelTitleBar *>(
                m_filmstripDock->titleBarWidget()), this);
    connect(m_filmstripLayoutController,
            &FilmstripLayoutController::modeChanged,
            this, [this](const QString &mode) {
        m_settings.floatingThumbnailLayout = mode;
        updateFilmstripLayout(
            dockWidgetArea(m_filmstripDock));
        saveSettings();
    });

    connect(m_metadataAction, &QAction::toggled,
            m_metadataDock, &QDockWidget::setVisible);
    connect(m_metadataDock, &QDockWidget::visibilityChanged,
            this, [this](bool visible) {
        if (m_metadataDock->property(
                "clearveilPanelPlacementTransition").toBool())
            return;
        if (m_windowModeController
            && m_windowModeController->isApplyingComponentVisibility())
            return;
        const QSignalBlocker blocker(m_metadataAction);
        m_metadataAction->setChecked(visible);
            if (visible)
                updateMetadataPanel();
    });
    connect(m_colorPickerDock,
            &QDockWidget::visibilityChanged,
            this, [this](bool visible) {
        if (m_colorPickerDock->property(
                "clearveilPanelPlacementTransition").toBool())
            return;
        if (visible || !m_colorPickerAction
            || !m_colorPickerAction->isChecked())
            return;
        const QSignalBlocker blocker(
            m_colorPickerAction);
        m_colorPickerAction->setChecked(false);
        m_colorPickerController->setEnabled(false);
    });
    connect(m_filmstripDock, &QDockWidget::visibilityChanged,
            this, [this](bool visible) {
        if (m_filmstripDock->property(
                "clearveilPanelPlacementTransition").toBool())
            return;
        if (m_windowModeController
            && m_windowModeController->isApplyingComponentVisibility())
            return;
        const QSignalBlocker blocker(m_filmstripAction);
        m_filmstripAction->setChecked(visible);
    });
    connect(m_filmstripDock, &QDockWidget::dockLocationChanged,
            this, &MainWindow::updateFilmstripLayout);

    connect(m_canvas, &ImageCanvas::zoomChanged, this, [this](qreal) {
        updateZoomModeActions();
        updateStatus();
        scheduleFitWindowToImage();
    });
    connect(m_canvas, &ImageCanvas::filesDropped, this, [this](const QStringList &files) {
        if (!files.isEmpty())
            openPaths(files);
    });
    m_canvas->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_canvas, &QWidget::customContextMenuRequested, this, [this](const QPoint &position) {
        QMenu menu;
        menu.addAction(m_previousAction);
        menu.addAction(m_nextAction);
        menu.addSeparator();
        menu.addAction(m_fitAction);
        menu.addAction(m_actualSizeAction);
        menu.addAction(m_zoomInAction);
        menu.addAction(m_zoomOutAction);
        menu.addSeparator();
        menu.addAction(m_rotateLeftAction);
        menu.addAction(m_rotateRightAction);
        menu.addAction(m_cropAction);
        menu.addSeparator();
        menu.addAction(m_ocrDebugAction);
        menu.addAction(m_copyAction);
        menu.addAction(m_saveAsAction);
        menu.addAction(m_trashAction);
        menu.exec(m_canvas->mapToGlobal(position));
    });
    connect(m_canvas, &ImageCanvas::mouseActionRequested,
            this, [this](const QString &actionId) {
                if (QAction *action =
                        m_actionRegistry->action(actionId)) {
                    action->trigger();
                }
            });
    connect(m_filmstripController,
            &FilmstripController::activationRequested,
            this, [this](FilmstripController::Source,
                         int row, const QString &) {
        activateFilmstripRow(row);
    });
    connect(m_filmstrip, &QListView::doubleClicked,
            this, [this](const QModelIndex &index) {
                if (index.isValid())
                    openDirectoryFilmstripRow(index.row());
            });
    connect(m_browser, &BrowserWidget::imageActivated,
            this, &MainWindow::openBrowserImage);
    connect(m_browser, &BrowserWidget::directoryChanged,
            this, [this](const QString &directory) {
        if (isBrowseMode())
            showBrowser(directory);
    });
    connect(m_browser, &BrowserWidget::compareRequested,
            this, &MainWindow::showCompare);
    connect(m_browser, &BrowserWidget::addToOpenedRequested,
            this, [this](const QStringList &paths) {
        const int before = m_session.openedCount();
        m_session.appendOpenedFiles(paths);
        const int added = m_session.openedCount() - before;
        m_openedFilmstripModel->setFiles(
            m_session.openedFiles());
        updateActions();
        updateStatus();
        statusBar()->showMessage(
            tr("Added %1 image(s) to opened images.")
                .arg(added),
            3000);
    });
    connect(m_browser, &BrowserWidget::revealRequested,
            this, [this](const QString &path) {
        const FileOperations::Result result =
            FileOperations::revealInFileManager(path);
        if (!result.succeeded()) {
            statusBar()->showMessage(
                tr("Could not show the image in the file manager."),
                5000);
        }
    });
    connect(m_browser, &BrowserWidget::fileOperationRequested,
            this, [this](const QString &operationId,
                         const QStringList &paths) {
        const bool changesSource =
            operationId == QStringLiteral("rename")
            || operationId == QStringLiteral("move")
            || operationId == QStringLiteral("trash");
        const QString displayedPath =
            QFileInfo(m_document.filePath()).absoluteFilePath();
        bool includesDisplayedImage = false;
        for (const QString &path : paths) {
            if (QFileInfo(path).absoluteFilePath()
                == displayedPath) {
                includesDisplayedImage = true;
                break;
            }
        }
        if (changesSource && includesDisplayedImage
            && !confirmDiscardChanges()) {
            return;
        }
        m_browserFileOperationsController->perform(
            operationId, paths);
    });
    connect(m_browserFileOperationsController,
            &BrowserFileOperationsController::statusMessage,
            statusBar(), &SelectableStatusBar::showMessage);
    connect(m_browserFileOperationsController,
            &BrowserFileOperationsController::operationCompleted,
            this, [this](
                BrowserFileOperationsController::Operation operation,
                const QList<FileOperations::Result> &results) {
        if (results.isEmpty())
            return;

        const QString displayedPath =
            QFileInfo(m_document.filePath()).absoluteFilePath();
        const int displayedDirectoryIndex =
            m_session.directoryIndexOf(displayedPath);
        const bool displayedUsesDirectory =
            m_filmstripController
            && m_filmstripController->source()
                == FilmstripController::Source::CurrentDirectory
            && displayedDirectoryIndex >= 0;
        QString nextDirectoryPath;
        if (displayedUsesDirectory) {
            if (displayedDirectoryIndex + 1
                < m_session.directoryCount()) {
                nextDirectoryPath = m_session.directoryPathAt(
                    displayedDirectoryIndex + 1);
            } else if (displayedDirectoryIndex > 0) {
                nextDirectoryPath = m_session.directoryPathAt(
                    displayedDirectoryIndex - 1);
            }
        }
        QString relocatedCurrentPath;
        bool removedCurrent = false;
        QSet<QString> changedDirectories;
        for (const FileOperations::Result &result : results) {
            if (operation
                != BrowserFileOperationsController::Operation::Copy) {
                changedDirectories.insert(
                    QFileInfo(result.sourcePath).absolutePath());
            }
            if (operation
                    == BrowserFileOperationsController::Operation::Copy
                || operation
                    == BrowserFileOperationsController::Operation::Move) {
                changedDirectories.insert(
                    QFileInfo(result.targetPath).absolutePath());
            }
            if (operation
                    == BrowserFileOperationsController::Operation::Rename
                || operation
                    == BrowserFileOperationsController::Operation::Move) {
                m_session.replaceOpenedFile(
                    result.sourcePath, result.targetPath);
                if (QFileInfo(result.sourcePath).absoluteFilePath()
                    == displayedPath) {
                    relocatedCurrentPath = result.targetPath;
                }
            } else if (operation
                    == BrowserFileOperationsController::Operation::Trash) {
                const auto removal = m_session.removeOpenedPath(
                    result.sourcePath, displayedPath);
                Q_UNUSED(removal);
                if (QFileInfo(result.sourcePath).absoluteFilePath()
                    == displayedPath) {
                    removedCurrent = true;
                }
            }
        }

        for (const QString &directory :
             std::as_const(changedDirectories)) {
            m_directoryScanService->invalidate(directory);
        }

        if (!relocatedCurrentPath.isEmpty()) {
            QString error;
            if (loadDocumentPath(
                    relocatedCurrentPath, &error)) {
                m_session.setLoadedPath(relocatedCurrentPath);
                m_session.invalidateDirectory();
                rebuildFilmstrip();
            } else {
                m_documentWorkflowController->clear();
                statusBar()->showMessage(error, 5000);
            }
        } else if (removedCurrent) {
            cancelPendingImageLoad();
            m_documentWorkflowController->clear();
            if (displayedUsesDirectory) {
                requestDirectoryScan(
                    QFileInfo(displayedPath).absolutePath(),
                    true, true, nextDirectoryPath);
            } else if (!m_session.openedFilesEmpty()) {
                const QString nextPath =
                    m_session.openedPathAt(0);
                setFilmstripSource(false);
                requestDocumentPath(nextPath, 0);
            } else {
                const QString sourceDirectory =
                    QFileInfo(displayedPath).absolutePath();
                requestDirectoryScan(
                    sourceDirectory, true, true);
            }
        } else {
            m_openedFilmstripModel->setFiles(
                m_session.openedFiles());
            const QString sessionDirectory =
                m_session.directoryPath();
            if (changedDirectories.contains(
                    sessionDirectory)) {
                requestDirectoryScan(
                    sessionDirectory, true);
            }
        }

        if (isBrowseMode()) {
            const QString browserDirectory =
                m_browser->directoryPath();
            if (changedDirectories.contains(
                    browserDirectory)
                && (!m_directoryScanRequestId
                    || m_directoryScanPath
                        != browserDirectory)) {
                m_browser->refreshDirectory();
            }
        }
        updateActions();
        updateStatus();
    });
    connect(m_browser, &BrowserWidget::backRequested,
            this, &MainWindow::showViewer);
    connect(m_browser, &BrowserWidget::scanFailed,
            this, [this](const QString &, const QString &error) {
        statusBar()->showMessage(
            error.isEmpty()
                ? tr("Could not scan the folder.") : error,
            5000);
    });
    connect(m_compare, &CompareWidget::backRequested, this, [this] {
        showBrowser(m_browser->directoryPath());
    });
    updateZoomModeActions();
}

void MainWindow::buildActions()
{
    auto icon = [this](QStyle::StandardPixmap pixmap) {
        return style()->standardIcon(pixmap);
    };

    m_openAction = makeAction(tr("Open image…"), QKeySequence::Open, SLOT(openFile()));
    m_openAction->setIcon(themedIcon(QStringLiteral("document-open"), style(),
                                    QStyle::SP_DialogOpenButton));
    m_openFolderAction = makeAction(tr("Open folder…"),
                                    QKeySequence(QStringLiteral("Ctrl+Shift+O")),
                                    SLOT(openFolder()));
    m_openFolderAction->setIcon(themedIcon(QStringLiteral("folder-open"), style(),
                                          QStyle::SP_DirOpenIcon));
    m_pasteAction = makeAction(tr("Paste"), QKeySequence::Paste, SLOT(pasteImage()));
    m_pasteAction->setIcon(themedIcon(QStringLiteral("edit-paste"), style(),
                                     QStyle::SP_DialogOpenButton));
    m_copyAction = makeAction(tr("Copy image"), QKeySequence::Copy, SLOT(copyImage()));
    m_copyAction->setIcon(themedIcon(QStringLiteral("edit-copy"), style(),
                                    QStyle::SP_DialogSaveButton));
    m_saveAsAction = makeAction(tr("Save as…"), QKeySequence::SaveAs, SLOT(saveAs()));
    m_saveAsAction->setIcon(icon(QStyle::SP_DialogSaveButton));

    m_previousAction = makeAction(
        tr("Previous"), QKeySequence(Qt::Key_Left),
        SLOT(previousImage()));
    m_previousAction->setIcon(icon(QStyle::SP_ArrowBack));
    m_nextAction = makeAction(
        tr("Next"), QKeySequence(Qt::Key_Right),
        SLOT(nextImage()));
    m_nextAction->setIcon(icon(QStyle::SP_ArrowForward));

    m_undoAction = new QAction(tr("Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered,
            this, [this] { m_editController->undo(); });
    m_undoAction->setIcon(icon(QStyle::SP_ArrowBack));
    m_redoAction = new QAction(tr("Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered,
            this, [this] { m_editController->redo(); });
    m_redoAction->setIcon(icon(QStyle::SP_ArrowForward));

    m_rotateLeftAction = new QAction(tr("Rotate left"), this);
    m_rotateLeftAction->setIcon(themedIcon(QStringLiteral("object-rotate-left"), style(),
                                          QStyle::SP_BrowserReload));
    m_rotateLeftAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+R")));
    connect(m_rotateLeftAction, &QAction::triggered,
            this, [this] {
        m_editController->rotateCounterClockwise();
    });
    m_rotateRightAction = new QAction(tr("Rotate right"), this);
    m_rotateRightAction->setIcon(themedIcon(QStringLiteral("object-rotate-right"), style(),
                                           QStyle::SP_BrowserReload));
    m_rotateRightAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));
    connect(m_rotateRightAction, &QAction::triggered,
            this, [this] {
        m_editController->rotateClockwise();
    });
    m_flipHorizontalAction = new QAction(tr("Mirror horizontally"), this);
    m_flipHorizontalAction->setIcon(
        themedIcon(QStringLiteral("object-flip-horizontal"), style(),
                   QStyle::SP_ArrowLeft));
    connect(m_flipHorizontalAction, &QAction::triggered,
            this, [this] {
        m_editController->flipHorizontal();
    });
    m_flipVerticalAction = new QAction(tr("Flip vertically"), this);
    m_flipVerticalAction->setIcon(
        themedIcon(QStringLiteral("object-flip-vertical"), style(),
                   QStyle::SP_ArrowUp));
    connect(m_flipVerticalAction, &QAction::triggered,
            this, [this] {
        m_editController->flipVertical();
    });
    m_cropAction = makeAction(tr("Crop image…"), QKeySequence(QStringLiteral("C")),
                              SLOT(cropImage()));
    m_cropAction->setIcon(themedIcon(QStringLiteral("transform-crop"), style(),
                                    QStyle::SP_DialogApplyButton));
    m_resizeAction = makeAction(tr("Resize image…"),
                                QKeySequence(QStringLiteral("Shift+R")),
                                SLOT(resizeImage()));
    m_resizeAction->setIcon(themedIcon(QStringLiteral("transform-scale"), style(),
                                      QStyle::SP_TitleBarMaxButton));
    m_adjustAction = makeAction(tr("Adjust colors…"),
                                QKeySequence(QStringLiteral("A")),
                                SLOT(adjustImage()));
    m_adjustAction->setIcon(themedIcon(QStringLiteral("color-management"), style(),
                                      QStyle::SP_ComputerIcon));
    m_redEyeAction = makeAction(tr("Red-eye correction…"), QKeySequence(),
                                SLOT(reduceRedEye()));
    m_redEyeAction->setIcon(themedIcon(QStringLiteral("redeyes"), style(),
                                      QStyle::SP_DialogApplyButton));

    m_fitAction = new QAction(tr("Fit to window"), this);
    m_fitAction->setIcon(themedIcon(QStringLiteral("zoom-fit-best"), style(),
                                   QStyle::SP_DesktopIcon));
    m_fitAction->setShortcut(QKeySequence(QStringLiteral("0")));
    m_actualSizeAction = new QAction(tr("Actual size"), this);
    m_actualSizeAction->setIcon(themedIcon(QStringLiteral("zoom-original"), style(),
                                          QStyle::SP_FileIcon));
    m_actualSizeAction->setShortcut(QKeySequence(QStringLiteral("1")));
    m_zoomInAction = new QAction(tr("Zoom in"), this);
    m_zoomInAction->setIcon(themedIcon(QStringLiteral("zoom-in"), style(),
                                      QStyle::SP_ArrowUp));
    m_zoomInAction->setShortcuts({QKeySequence::ZoomIn, QKeySequence(QStringLiteral("="))});
    m_zoomOutAction = new QAction(tr("Zoom out"), this);
    m_zoomOutAction->setIcon(themedIcon(QStringLiteral("zoom-out"), style(),
                                       QStyle::SP_ArrowDown));
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_fitWidthAction = new QAction(tr("Fit to width"), this);
    m_fitWidthAction->setIcon(themedIcon(QStringLiteral("zoom-fit-width"), style(),
                                        QStyle::SP_ArrowRight));
    m_fitHeightAction = new QAction(tr("Fit to height"), this);
    m_fitHeightAction->setIcon(themedIcon(QStringLiteral("zoom-fit-height"), style(),
                                         QStyle::SP_ArrowUp));
    m_fillAction = new QAction(tr("Fill window"), this);
    m_fillAction->setIcon(themedIcon(QStringLiteral("zoom-fit-best"), style(),
                                    QStyle::SP_TitleBarMaxButton));
    m_lockZoomAction = new QAction(tr("Lock zoom"), this);
    m_lockZoomAction->setCheckable(true);
    m_lockZoomAction->setObjectName(
        QStringLiteral("lockZoomAction"));
    m_lockZoomAction->setToolTip(
        tr("Keep the current zoom percentage when switching images or resizing the window"));
    m_lockZoomAction->setIcon(themedIcon(QStringLiteral("object-locked"), style(),
                                        QStyle::SP_DialogYesButton));
    m_zoomModeGroup = new QActionGroup(this);
    m_zoomModeGroup->setExclusionPolicy(
        QActionGroup::ExclusionPolicy::ExclusiveOptional);
    for (QAction *action : {m_fitAction, m_fitWidthAction, m_fitHeightAction,
                            m_fillAction, m_actualSizeAction}) {
        action->setCheckable(true);
        m_zoomModeGroup->addAction(action);
    }
    connect(m_fitAction, &QAction::triggered, this, [this] { m_canvas->fitToWindow(); });
    connect(m_fitWidthAction, &QAction::triggered, this, [this] { m_canvas->fitToWidth(); });
    connect(m_fitHeightAction, &QAction::triggered, this, [this] { m_canvas->fitToHeight(); });
    connect(m_fillAction, &QAction::triggered, this, [this] { m_canvas->fillWindow(); });
    connect(m_lockZoomAction, &QAction::toggled, this, [this](bool locked) {
        m_canvas->setZoomLocked(locked);
        statusBar()->showMessage(
            locked
                ? tr("Zoom locked at %1%. New images keep this percentage.")
                      .arg(QString::number(
                          m_canvas->zoom() * 100.0, 'f', 0))
                : tr("Zoom unlocked. New images use Fit to window."),
            4000);
        updateStatus();
    });
    connect(m_actualSizeAction, &QAction::triggered, this, [this] { m_canvas->actualSize(); });
    connect(m_zoomInAction, &QAction::triggered, this, [this] { m_canvas->zoomIn(); });
    connect(m_zoomOutAction, &QAction::triggered, this, [this] { m_canvas->zoomOut(); });

    m_metadataAction = new QAction(tr("Information panel"), this);
    m_metadataAction->setObjectName(
        QStringLiteral("metadataAction"));
    m_metadataAction->setCheckable(true);
    m_metadataAction->setIcon(themedIcon(QStringLiteral("document-properties"), style(),
                                        QStyle::SP_MessageBoxInformation));
    m_handToolAction = new QAction(tr("Hand tool"), this);
    m_handToolAction->setObjectName(
        QStringLiteral("handToolAction"));
    m_handToolAction->setCheckable(true);
    m_handToolAction->setShortcut(
        QKeySequence(QStringLiteral("H")));
    m_handToolAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("pointer_hand")));
    m_handToolAction->setToolTip(tr(
        "Drag the image to move the visible area"));

    m_colorPickerAction = new QAction(tr("Color picker tool"), this);
    m_colorPickerAction->setObjectName(
        QStringLiteral("colorPickerAction"));
    m_colorPickerAction->setCheckable(true);
    m_colorPickerAction->setShortcut(QKeySequence(QStringLiteral("K")));
    m_colorPickerAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("color_picker")));
    connect(m_colorPickerAction, &QAction::toggled, this, [this](bool enabled) {
        m_colorPickerController->setEnabled(enabled);
        if (enabled) {
            if (m_colorPickerDock) {
                m_colorPickerDock->show();
                m_colorPickerDock->raise();
            }
        } else {
            if (m_colorPickerDock)
                m_colorPickerDock->hide();
        }
    });
    m_textSelectionAction = new QAction(
        tr("Text selection tool"), this);
    m_textSelectionAction->setObjectName(
        QStringLiteral("textSelectionToolAction"));
    m_textSelectionAction->setCheckable(true);
    m_textSelectionAction->setShortcut(
        QKeySequence(QStringLiteral("Shift+T")));
    m_textSelectionAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("text_select")));
    m_textSelectionAction->setToolTip(tr(
        "Recognize text, then drag to select and copy it"));
    connect(m_textSelectionAction, &QAction::toggled,
            this, [this](bool enabled) {
        if (enabled && (!OcrEngine::isAvailable()
                        || m_ocrLanguages.isEmpty())) {
            if (m_handToolAction)
                m_handToolAction->setChecked(true);
            showOcrSupport();
            return;
        }
        if (m_canvas)
            m_canvas->setOcrTextSelectionEnabled(enabled);
        if (enabled) {
            if (m_displayColorController
                && m_displayColorController->isTransforming()) {
                m_ocrRecognitionPending = true;
            } else {
                startOcrRecognition();
            }
        } else if (m_ocrController) {
            m_ocrRecognitionPending = false;
            m_ocrController->cancel();
            statusBar()->clearMessage();
            if (m_ocrDebugAction)
                m_ocrDebugAction->setChecked(false);
        }
    });

    m_pointerToolGroup = new QActionGroup(this);
    m_pointerToolGroup->setExclusive(true);
    m_pointerToolGroup->addAction(m_handToolAction);
    m_pointerToolGroup->addAction(m_textSelectionAction);
    m_pointerToolGroup->addAction(m_colorPickerAction);
    m_handToolAction->setChecked(true);

    m_ocrDebugAction = new QAction(
        tr("OCR debug overlay"), this);
    m_ocrDebugAction->setObjectName(
        QStringLiteral("ocrDebugAction"));
    m_ocrDebugAction->setCheckable(true);
    m_ocrDebugAction->setToolTip(tr(
        "Show opaque OCR character cells, recognized text, and line/word indexes"));
    connect(m_ocrDebugAction, &QAction::toggled,
            this, [this](bool enabled) {
        if (enabled && m_textSelectionAction
            && !m_textSelectionAction->isChecked()) {
            m_textSelectionAction->setChecked(true);
            if (!m_textSelectionAction->isChecked()) {
                const QSignalBlocker blocker(m_ocrDebugAction);
                m_ocrDebugAction->setChecked(false);
                enabled = false;
            }
        }
        if (m_canvas)
            m_canvas->setOcrDebugOverlayEnabled(enabled);
    });
    m_checkerboardAction = new QAction(
        tr("Transparency checkerboard"), this);
    m_checkerboardAction->setObjectName(
        QStringLiteral("checkerboardAction"));
    m_checkerboardAction->setCheckable(true);
    m_checkerboardAction->setChecked(true);
    m_checkerboardAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("checkerboard")));
    m_checkerboardAction->setToolTip(
        tr("Show a checkerboard behind transparent pixels"));
    m_fullscreenAction = makeAction(tr("Full screen"),
                                    QKeySequence(Qt::Key_F11), SLOT(toggleFullscreen()));
    m_fullscreenAction->setObjectName(
        QStringLiteral("fullscreenAction"));
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setIcon(icon(QStyle::SP_TitleBarMaxButton));
    m_fitWindowToImageAction = new QAction(tr("Fit window to image"), this);
    m_fitWindowToImageAction->setObjectName(
        QStringLiteral("fitWindowToImageAction"));
    m_fitWindowToImageAction->setCheckable(true);
    m_fitWindowToImageAction->setIcon(
        themedIcon(QStringLiteral("view-restore"), style(),
                   QStyle::SP_TitleBarNormalButton));
    connect(m_fitWindowToImageAction, &QAction::toggled,
            this, [this](bool enabled) {
        if (enabled)
            scheduleFitWindowToImage(true);
    });
    m_borderlessAction = new QAction(tr("Borderless mode"), this);
    m_borderlessAction->setObjectName(
        QStringLiteral("borderlessAction"));
    m_borderlessAction->setCheckable(true);
    m_borderlessAction->setIcon(
        themedIcon(QStringLiteral("view-fullscreen"), style(),
                   QStyle::SP_TitleBarShadeButton));
    connect(m_borderlessAction, &QAction::toggled,
            this, &MainWindow::applyWindowModeFlags);
    m_alwaysOnTopAction = new QAction(tr("Always on top"), this);
    m_alwaysOnTopAction->setObjectName(
        QStringLiteral("alwaysOnTopAction"));
    m_alwaysOnTopAction->setCheckable(true);
    m_alwaysOnTopAction->setIcon(
        themedIcon(QStringLiteral("window-pin"), style(),
                   QStyle::SP_ArrowUp));
    connect(m_alwaysOnTopAction, &QAction::toggled,
            this, &MainWindow::applyWindowModeFlags);
    const auto makeFullscreenComponentAction =
        [this](const QString &text, const QString &objectName) {
        auto *action = new QAction(text, this);
        action->setObjectName(objectName);
        action->setCheckable(true);
        connect(action, &QAction::toggled, this, [this] {
            applyFullscreenComponentVisibility();
            updateMenuBarVisibility();
        });
        return action;
    };
    m_fullscreenToolbarAction = makeFullscreenComponentAction(
        tr("Show toolbar in full screen"),
        QStringLiteral("fullscreenToolbarAction"));
    m_fullscreenFilmstripAction = makeFullscreenComponentAction(
        tr("Show thumbnails in full screen"),
        QStringLiteral("fullscreenFilmstripAction"));
    m_fullscreenStatusBarAction = makeFullscreenComponentAction(
        tr("Show status bar in full screen"),
        QStringLiteral("fullscreenStatusBarAction"));
    m_fullscreenInformationAction = makeFullscreenComponentAction(
        tr("Show information panel in full screen"),
        QStringLiteral("fullscreenInformationAction"));
    m_filmstripAction = makeAction(tr("Show thumbnails"),
                                   QKeySequence(QStringLiteral("T")), SLOT(toggleFilmstrip()));
    m_filmstripAction->setObjectName(
        QStringLiteral("filmstripAction"));
    m_filmstripAction->setCheckable(true);
    m_filmstripAction->setChecked(true);
    m_filmstripAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("filmstrip")));
    m_filmstripSourceAction = new QAction(
        tr("Use current folder thumbnails"), this);
    m_filmstripSourceAction->setObjectName(
        QStringLiteral("filmstripSourceAction"));
    m_filmstripSourceAction->setCheckable(true);
    m_filmstripSourceAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("filmstrip_source")));
    m_filmstripSourceAction->setToolTip(
        tr("Use current folder thumbnails"));
    connect(m_filmstripSourceAction, &QAction::toggled,
            this, &MainWindow::setFilmstripSource);
    m_browseAction = new QAction(tr("Folder overview"), this);
    m_browseAction->setObjectName(
        QStringLiteral("folderOverviewAction"));
    m_browseAction->setCheckable(true);
    m_browseAction->setShortcut(QKeySequence(QStringLiteral("B")));
    m_browseAction->setIcon(themedIcon(QStringLiteral("view-grid"), style(),
                                      QStyle::SP_FileDialogListView));
    connect(m_browseAction, &QAction::toggled,
            this, &MainWindow::toggleBrowseMode);
    m_slideshowAction = new QAction(tr("Slideshow"), this);
    m_slideshowAction->setObjectName(
        QStringLiteral("slideshowAction"));
    m_slideshowAction->setShortcut(QKeySequence(Qt::Key_Space));
    m_slideshowAction->setCheckable(true);
    m_slideshowAction->setIcon(themedIcon(QStringLiteral("media-playback-start"), style(),
                                         QStyle::SP_MediaPlay));
    connect(m_slideshowAction, &QAction::toggled,
            this, &MainWindow::toggleSlideshow);
    m_trashAction = makeAction(tr("Move to Trash"), QKeySequence(Qt::Key_Delete),
                               SLOT(moveToTrash()));
    m_trashAction->setIcon(themedIcon(QStringLiteral("user-trash"), style(),
                                     QStyle::SP_TrashIcon));
    m_renameAction = makeAction(tr("Rename…"), QKeySequence(Qt::Key_F2),
                                SLOT(renameFile()));
    m_renameAction->setIcon(themedIcon(QStringLiteral("edit-rename"), style(),
                                      QStyle::SP_FileIcon));
    m_copyFileAction = makeAction(tr("Copy file to…"), QKeySequence(),
                                  SLOT(copyFileTo()));
    m_copyFileAction->setIcon(themedIcon(QStringLiteral("edit-copy"), style(),
                                        QStyle::SP_DialogSaveButton));
    m_moveFileAction = makeAction(tr("Move file to…"), QKeySequence(),
                                  SLOT(moveFileTo()));
    m_moveFileAction->setIcon(themedIcon(QStringLiteral("go-jump"), style(),
                                        QStyle::SP_ArrowForward));
    m_revealAction = makeAction(tr("Show in file manager"), QKeySequence(),
                                SLOT(revealInFileManager()));
    m_revealAction->setIcon(themedIcon(QStringLiteral("folder-open"), style(),
                                      QStyle::SP_DirOpenIcon));
    m_openWithAction = makeAction(tr("Open with…"), QKeySequence(),
                                  SLOT(openWithApplication()));
    m_openWithAction->setIcon(themedIcon(QStringLiteral("system-run"), style(),
                                        QStyle::SP_CommandLink));
    m_printAction = makeAction(tr("Print…"), QKeySequence::Print,
                               SLOT(printImage()));
    m_printAction->setIcon(themedIcon(QStringLiteral("document-print"), style(),
                                     QStyle::SP_DialogSaveButton));
    m_settingsAction = makeAction(tr("Preferences…"),
                                  QKeySequence(QStringLiteral("Ctrl+,")),
                                  SLOT(showPreferences()));
    m_settingsAction->setIcon(themedIcon(QStringLiteral("settings-configure"), style(),
                                        QStyle::SP_FileDialogDetailedView));
    m_formatCapabilitiesAction = makeAction(
        tr("Supported image formats…"), QKeySequence(),
        SLOT(showFormatCapabilities()));
    m_formatCapabilitiesAction->setObjectName(
        QStringLiteral("formatCapabilitiesAction"));
    m_formatCapabilitiesAction->setIcon(
        themedIcon(QStringLiteral("help-about"), style(),
                   QStyle::SP_MessageBoxInformation));
    m_ocrSupportAction = makeAction(
        tr("OCR support…"), QKeySequence(),
        SLOT(showOcrSupport()));
    m_ocrSupportAction->setObjectName(
        QStringLiteral("ocrSupportAction"));
    m_ocrSupportAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("text_select")));
    m_aboutAction = makeAction(
        tr("About Clearveil…"), QKeySequence(),
        SLOT(showAbout()));
    m_aboutAction->setObjectName(QStringLiteral("aboutAction"));
    m_aboutAction->setIcon(
        themedIcon(QStringLiteral("help-about"), style(),
                   QStyle::SP_MessageBoxInformation));
    m_layoutLockAction = new QAction(tr("Lock layout"), this);
    m_layoutLockAction->setObjectName(
        QStringLiteral("layoutLockAction"));
    m_layoutLockAction->setCheckable(true);
    m_layoutLockAction->setChecked(true);
    m_layoutLockAction->setIcon(
        themedIcon(QStringLiteral("object-locked"), style(),
                   QStyle::SP_DialogYesButton));
    connect(m_layoutLockAction, &QAction::toggled,
            this, &MainWindow::setLayoutLocked);
    m_interfaceLayoutAction = new QAction(
        tr("Interface and layout…"), this);
    m_interfaceLayoutAction->setObjectName(
        QStringLiteral("interfaceLayoutAction"));
    connect(m_interfaceLayoutAction, &QAction::triggered,
            this, &MainWindow::showInterfaceLayout);
    m_floatFilmstripAction = new QAction(
        tr("Float thumbnails panel"), this);
    m_floatFilmstripAction->setObjectName(
        QStringLiteral("floatFilmstripAction"));
    m_floatFilmstripAction->setCheckable(true);
    m_floatMetadataAction = new QAction(
        tr("Float information panel"), this);
    m_floatMetadataAction->setObjectName(
        QStringLiteral("floatMetadataAction"));
    m_floatMetadataAction->setCheckable(true);
    m_floatColorPickerAction = new QAction(
        tr("Float color picker panel"), this);
    m_floatColorPickerAction->setObjectName(
        QStringLiteral("floatColorPickerAction"));
    m_floatColorPickerAction->setCheckable(true);
    m_menuBarAction = new QAction(tr("Show menu bar"), this);
    m_menuBarAction->setCheckable(true);
    m_menuBarAction->setChecked(false);
    connect(m_menuBarAction, &QAction::toggled,
            this, [this] { updateMenuBarVisibility(); });
    m_statusBarAction = new QAction(tr("Show status bar"), this);
    m_statusBarAction->setCheckable(true);
    m_statusBarAction->setChecked(true);
    connect(m_statusBarAction, &QAction::toggled,
            this, [this] { updateMenuBarVisibility(); });
    m_wallpaperAction = makeAction(tr("Set as wallpaper…"), QKeySequence(),
                                   SLOT(setAsWallpaper()));
    m_wallpaperAction->setIcon(
        themedIcon(QStringLiteral("preferences-desktop-wallpaper"), style(),
                   QStyle::SP_DesktopIcon));

    m_frameFirstAction = new QAction(tr("First frame"), this);
    m_frameFirstAction->setIcon(icon(QStyle::SP_MediaSkipBackward));
    connect(m_frameFirstAction, &QAction::triggered, &m_frames, &FrameController::first);
    m_framePreviousAction = new QAction(tr("Previous frame"), this);
    m_framePreviousAction->setIcon(icon(QStyle::SP_MediaSeekBackward));
    connect(m_framePreviousAction, &QAction::triggered,
            &m_frames, &FrameController::previous);
    m_framePlayAction = new QAction(tr("Play animation"), this);
    m_framePlayAction->setCheckable(true);
    m_framePlayAction->setIcon(icon(QStyle::SP_MediaPlay));
    connect(m_framePlayAction, &QAction::toggled,
            &m_frames, &FrameController::setPlaying);
    m_frameNextAction = new QAction(tr("Next frame"), this);
    m_frameNextAction->setIcon(icon(QStyle::SP_MediaSeekForward));
    connect(m_frameNextAction, &QAction::triggered, &m_frames, &FrameController::next);
    m_frameLastAction = new QAction(tr("Last frame"), this);
    m_frameLastAction->setIcon(icon(QStyle::SP_MediaSkipForward));
    connect(m_frameLastAction, &QAction::triggered, &m_frames, &FrameController::last);
    m_exportFrameAction = new QAction(tr("Export current frame…"), this);
    m_exportFrameAction->setIcon(themedIcon(QStringLiteral("document-export"), style(),
                                           QStyle::SP_DialogSaveButton));
    connect(m_exportFrameAction, &QAction::triggered, this, &MainWindow::saveAs);

    addAction(m_openAction);
    addAction(m_openFolderAction);
    addAction(m_pasteAction);
}

void MainWindow::buildActionRegistry()
{
    m_actionRegistry = new ActionRegistry(this);
    const auto add = [this](const char *id, QAction *action,
                            const QKeySequence &shortcut = {}) {
        action->setIconVisibleInMenu(false);
        m_actionRegistry->addAction(
            QString::fromLatin1(id), action, shortcut);
    };

    add("open", m_openAction, QKeySequence::Open);
    add("open_folder", m_openFolderAction,
        QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    add("paste", m_pasteAction, QKeySequence::Paste);
    add("copy", m_copyAction, QKeySequence::Copy);
    add("save_as", m_saveAsAction, QKeySequence::SaveAs);
    add("print", m_printAction, QKeySequence::Print);
    add("previous", m_previousAction,
        QKeySequence(Qt::Key_Left));
    add("next", m_nextAction, QKeySequence(Qt::Key_Right));
    add("undo", m_undoAction, QKeySequence::Undo);
    add("redo", m_redoAction, QKeySequence::Redo);
    add("rotate_left", m_rotateLeftAction,
        QKeySequence(QStringLiteral("Ctrl+Shift+R")));
    add("rotate_right", m_rotateRightAction,
        QKeySequence(QStringLiteral("Ctrl+R")));
    add("flip_horizontal", m_flipHorizontalAction);
    add("flip_vertical", m_flipVerticalAction);
    add("crop", m_cropAction, QKeySequence(QStringLiteral("C")));
    add("resize", m_resizeAction,
        QKeySequence(QStringLiteral("Shift+R")));
    add("adjust", m_adjustAction, QKeySequence(QStringLiteral("A")));
    add("red_eye", m_redEyeAction);
    add("fit", m_fitAction, QKeySequence(QStringLiteral("0")));
    add("fit_width", m_fitWidthAction);
    add("fit_height", m_fitHeightAction);
    add("fill", m_fillAction);
    add("actual_size", m_actualSizeAction,
        QKeySequence(QStringLiteral("1")));
    add("zoom_in", m_zoomInAction, QKeySequence::ZoomIn);
    add("zoom_out", m_zoomOutAction, QKeySequence::ZoomOut);
    add("lock_zoom", m_lockZoomAction);
    add("metadata", m_metadataAction);
    add("pointer_hand", m_handToolAction,
        QKeySequence(QStringLiteral("H")));
    add("color_picker", m_colorPickerAction,
        QKeySequence(QStringLiteral("K")));
    add("text_select", m_textSelectionAction,
        QKeySequence(QStringLiteral("Shift+T")));
    add("checkerboard", m_checkerboardAction);
    add("fullscreen", m_fullscreenAction,
        QKeySequence(Qt::Key_F11));
    add("fit_window_to_image", m_fitWindowToImageAction,
        QKeySequence(Qt::Key_F9));
    add("borderless", m_borderlessAction,
        QKeySequence(Qt::Key_F10));
    add("always_on_top", m_alwaysOnTopAction);
    add("filmstrip", m_filmstripAction,
        QKeySequence(QStringLiteral("T")));
    add("filmstrip_source", m_filmstripSourceAction);
    add("browse", m_browseAction,
        QKeySequence(QStringLiteral("B")));
    add("slideshow", m_slideshowAction,
        QKeySequence(Qt::Key_Space));
    add("trash", m_trashAction, QKeySequence(Qt::Key_Delete));
    add("rename", m_renameAction, QKeySequence(Qt::Key_F2));
    add("copy_file", m_copyFileAction);
    add("move_file", m_moveFileAction);
    add("reveal", m_revealAction);
    add("open_with", m_openWithAction);
    add("wallpaper", m_wallpaperAction);
    add("settings", m_settingsAction,
        QKeySequence(QStringLiteral("Ctrl+,")));
    add("layout_lock", m_layoutLockAction);
    add("menu_bar", m_menuBarAction);
    add("status_bar", m_statusBarAction);
    add("frame_first", m_frameFirstAction);
    add("frame_previous", m_framePreviousAction);
    add("frame_play", m_framePlayAction);
    add("frame_next", m_frameNextAction);
    add("frame_last", m_frameLastAction);
    add("export_frame", m_exportFrameAction);

    const auto toolbar = [this](const char *id, bool enabled) {
        m_actionRegistry->addToolbarItem(
            QString::fromLatin1(id), enabled);
    };
    toolbar("previous", true);
    toolbar("next", true);
    m_actionRegistry->addToolbarItem(
        QStringLiteral("separator1"), true, tr("Separator"));
    toolbar("zoom_out", true);
    toolbar("fit", true);
    toolbar("actual_size", true);
    toolbar("zoom_in", true);
    toolbar("lock_zoom", true);
    m_actionRegistry->addToolbarItem(
        QStringLiteral("separator2"), true, tr("Separator"));
    toolbar("rotate_left", true);
    toolbar("rotate_right", true);
    toolbar("crop", true);
    toolbar("pointer_hand", true);
    toolbar("text_select", true);
    toolbar("color_picker", true);
    toolbar("checkerboard", false);
    toolbar("filmstrip", true);
    toolbar("filmstrip_source", true);
    toolbar("browse", true);
    toolbar("slideshow", true);
    m_actionRegistry->addToolbarItem(
        QStringLiteral("spacer"), true, tr("Flexible space"));
    toolbar("fullscreen", true);
    toolbar("open", true);
    toolbar("trash", true);
    toolbar("open_folder", false);
    toolbar("save_as", false);
    toolbar("copy", false);
    toolbar("undo", false);
    toolbar("redo", false);
    toolbar("fit_width", false);
    toolbar("fit_height", false);
    toolbar("fill", false);
    toolbar("flip_horizontal", false);
    toolbar("flip_vertical", false);
    toolbar("resize", false);
    toolbar("adjust", false);
    toolbar("metadata", false);
    toolbar("print", false);
    toolbar("wallpaper", false);
    toolbar("fit_window_to_image", false);
    toolbar("borderless", false);
    toolbar("always_on_top", false);

    const QStringList vectorToolbarIcons{
        QStringLiteral("previous"), QStringLiteral("next"),
        QStringLiteral("zoom_out"), QStringLiteral("fit"),
        QStringLiteral("actual_size"), QStringLiteral("zoom_in"),
        QStringLiteral("lock_zoom"), QStringLiteral("rotate_left"),
        QStringLiteral("rotate_right"), QStringLiteral("crop"),
        QStringLiteral("pointer_hand"), QStringLiteral("text_select"),
        QStringLiteral("color_picker"),
        QStringLiteral("checkerboard"),
        QStringLiteral("filmstrip"), QStringLiteral("filmstrip_source"),
        QStringLiteral("browse"), QStringLiteral("slideshow"),
        QStringLiteral("fullscreen"), QStringLiteral("open"),
        QStringLiteral("trash"), QStringLiteral("open_folder"),
        QStringLiteral("save_as"), QStringLiteral("copy"),
        QStringLiteral("undo"), QStringLiteral("redo"),
        QStringLiteral("fit_width"), QStringLiteral("fit_height"),
        QStringLiteral("fill"), QStringLiteral("flip_horizontal"),
        QStringLiteral("flip_vertical"), QStringLiteral("resize"),
        QStringLiteral("adjust"), QStringLiteral("metadata"),
        QStringLiteral("print"), QStringLiteral("wallpaper"),
        QStringLiteral("fit_window_to_image"),
        QStringLiteral("borderless"), QStringLiteral("always_on_top")
    };
    for (const QString &id : vectorToolbarIcons) {
        if (QAction *action = m_actionRegistry->action(id)) {
            action->setIcon(ClearveilIcon::fromName(id));
            action->setIconVisibleInMenu(false);
        }
    }
}

void MainWindow::buildMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_openFolderAction);
    fileMenu->addAction(m_pasteAction);
    fileMenu->addAction(m_copyAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addAction(m_printAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_renameAction);
    fileMenu->addAction(m_copyFileAction);
    fileMenu->addAction(m_moveFileAction);
    fileMenu->addAction(m_revealAction);
    fileMenu->addAction(m_openWithAction);
    fileMenu->addAction(m_trashAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_settingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quit"), QKeySequence::Quit,
                        qApp, &QApplication::closeAllWindows);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(m_rotateLeftAction);
    editMenu->addAction(m_rotateRightAction);
    editMenu->addAction(m_flipHorizontalAction);
    editMenu->addAction(m_flipVerticalAction);
    editMenu->addSeparator();
    editMenu->addAction(m_cropAction);
    editMenu->addAction(m_resizeAction);
    editMenu->addAction(m_adjustAction);
    editMenu->addAction(m_redEyeAction);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_previousAction);
    viewMenu->addAction(m_nextAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_fitAction);
    viewMenu->addAction(m_fitWidthAction);
    viewMenu->addAction(m_fitHeightAction);
    viewMenu->addAction(m_fillAction);
    viewMenu->addAction(m_lockZoomAction);
    viewMenu->addAction(m_actualSizeAction);
    viewMenu->addAction(m_zoomInAction);
    viewMenu->addAction(m_zoomOutAction);
    viewMenu->addSeparator();
    QMenu *backgroundMenu = viewMenu->addMenu(tr("Background"));
    backgroundMenu->addAction(m_checkerboardAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_interfaceLayoutAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_browseAction);
    viewMenu->addAction(m_slideshowAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_fitWindowToImageAction);
    viewMenu->addAction(m_borderlessAction);
    viewMenu->addAction(m_alwaysOnTopAction);
    viewMenu->addAction(m_fullscreenAction);

    QMenu *imageMenu = menuBar()->addMenu(tr("&Image"));
    imageMenu->addAction(m_metadataAction);
    imageMenu->addAction(m_ocrDebugAction);
    imageMenu->addAction(m_wallpaperAction);
    QMenu *frameMenu = imageMenu->addMenu(tr("Frames"));
    frameMenu->addAction(m_frameFirstAction);
    frameMenu->addAction(m_framePreviousAction);
    frameMenu->addAction(m_framePlayAction);
    frameMenu->addAction(m_frameNextAction);
    frameMenu->addAction(m_frameLastAction);
    frameMenu->addSeparator();
    frameMenu->addAction(m_exportFrameAction);
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_formatCapabilitiesAction);
    helpMenu->addAction(m_ocrSupportAction);
    helpMenu->addSeparator();
    helpMenu->addAction(m_aboutAction);
    for (QAction *action : findChildren<QAction *>())
        action->setIconVisibleInMenu(false);
    menuBar()->hide();
}

void MainWindow::buildToolbar()
{
    auto *toolbar = addToolBar(tr("Main toolbar"));
    m_mainToolbar = toolbar;
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(true);
    toolbar->setFloatable(true);
    toolbar->setAllowedAreas(Qt::AllToolBarAreas);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(22, 22));
    m_mainMenu = new QMenu(this);
    m_mainMenu->setObjectName(QStringLiteral("mainMenu"));
    for (QAction *menuAction : menuBar()->actions())
        m_mainMenu->addAction(menuAction);
    m_settings.toolbarLayout = m_actionRegistry->defaultToolbarLayout();
    applyToolbarLayout();

    m_cornerMenuButton = new QToolButton(this);
    m_cornerMenuButton->setObjectName(QStringLiteral("cornerMenuButton"));
    m_cornerMenuButton->setIcon(
        ClearveilIcon::fromName(QStringLiteral("menu")));
    m_cornerMenuButton->setToolTip(tr("Main menu"));
    m_cornerMenuButton->setAccessibleName(tr("Main menu"));
    m_cornerMenuButton->setPopupMode(QToolButton::InstantPopup);
    m_cornerMenuButton->setMenu(m_mainMenu);
    m_cornerMenuButton->setAutoRaise(false);
    m_cornerMenuButton->setFixedSize(32, 32);
    m_cornerMenuButton->hide();
    toolbar->toggleViewAction()->setIconVisibleInMenu(false);
    connect(toolbar->toggleViewAction(), &QAction::toggled,
            this, [this] { updateMenuBarVisibility(); });
    setLayoutLocked(true);
    updateMenuBarVisibility();
}

QMenu *MainWindow::createPopupMenu()
{
    auto *menu = new QMenu(this);
    menu->setObjectName(
        QStringLiteral("windowLayoutPopupMenu"));
    menu->addAction(m_interfaceLayoutAction);
    return menu;
}

void MainWindow::applyShortcuts()
{
    m_settings.shortcutLayout =
        m_actionRegistry->normalizedShortcutLayout(m_settings.shortcutLayout);
    for (const QString &encoded :
         std::as_const(m_settings.shortcutLayout)) {
        QAction *action =
            m_actionRegistry->action(ActionRegistry::shortcutEntryId(encoded));
        if (!action)
            continue;
        const QKeySequence shortcut =
            ActionRegistry::shortcutEntrySequence(encoded);
        if (action == m_zoomInAction
            && shortcut == QKeySequence::ZoomIn) {
            action->setShortcuts(
                {QKeySequence::ZoomIn,
                 QKeySequence(QStringLiteral("="))});
        } else {
            action->setShortcut(shortcut);
        }
    }
}

void MainWindow::applyMouseActions()
{
    m_settings.normalize();
    if (m_canvas) {
        m_canvas->setMouseActions(
            m_settings.wheelAction, m_settings.ctrlWheelAction,
            m_settings.doubleClickAction,
            m_settings.middleButtonAction, m_settings.backButtonAction,
            m_settings.forwardButtonAction);
    }
}

void MainWindow::applyToolbarLayout()
{
    if (!m_mainToolbar || !m_mainMenu)
        return;

    m_settings.toolbarLayout = m_actionRegistry->normalizedToolbarLayout(m_settings.toolbarLayout);
    m_mainToolbar->clear();
    for (const QString &encoded : std::as_const(m_settings.toolbarLayout)) {
        if (encoded.startsWith(QLatin1Char('!')))
            continue;
        if (encoded.startsWith(QStringLiteral("separator"))) {
            m_mainToolbar->addSeparator();
            continue;
        }
        if (encoded == QStringLiteral("spacer")) {
            auto *spacer = new QWidget(m_mainToolbar);
            spacer->setObjectName(QStringLiteral("toolbarFlexibleSpace"));
            spacer->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Preferred);
            m_mainToolbar->addWidget(spacer);
            continue;
        }
        if (QAction *action = m_actionRegistry->action(encoded))
            m_mainToolbar->addAction(action);
    }

    auto *menuButton = new QToolButton(m_mainToolbar);
    menuButton->setObjectName(QStringLiteral("mainMenuButton"));
    menuButton->setIcon(
        ClearveilIcon::fromName(QStringLiteral("menu")));
    menuButton->setToolTip(tr("Main menu"));
    menuButton->setAccessibleName(tr("Main menu"));
    menuButton->setPopupMode(QToolButton::InstantPopup);
    menuButton->setMenu(m_mainMenu);
    m_mainToolbar->addWidget(menuButton);
    updateToolbarDensity();
    m_mainToolbar->updateGeometry();
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "QMainWindow { background: palette(window); }"
        "QDockWidget[clearveilFloating=\"true\"] {"
        " border: none;"
        " background-color: palette(window); }"
        "QDockWidget[clearveilOverlay=\"true\"] {"
        " border: none;"
        " background-color: palette(window); }"
        "QLabel#floatingPanelTitle { color: palette(window-text);"
        " font-weight: 600; }"
        "QFrame#colorPickerPreviewCard {"
        " background-color: palette(base);"
        " border: 1px solid palette(mid); border-radius: 4px; }"
        "QListView#filmstrip { border: none; border-top: 1px solid palette(mid);"
        " background: palette(alternate-base); padding: 5px; }"
        "QListView#filmstrip::item { border: 2px solid transparent;"
        " border-radius: 5px; padding: 2px; }"
        "QListView#filmstrip::item:selected { border-color: palette(highlight);"
        " background: palette(highlight); }"
        "QFrame#frameBar { border-top: 1px solid palette(mid);"
        " background: palette(alternate-base); }"));
}

void MainWindow::applyTheme(const QString &theme)
{
    const bool wasApplyingTheme = m_applyingTheme;
    m_applyingTheme = true;
    const QString resolvedTheme = m_systemAppearanceController
        ? m_systemAppearanceController->resolvedTheme(theme)
        : theme;
    const bool systemTheme =
        resolvedTheme == QStringLiteral("system");
    const BreezeTheme::Variant variant =
        resolvedTheme == QStringLiteral("dark")
        ? BreezeTheme::Variant::Dark
        : BreezeTheme::Variant::Light;
    const QString colorSchemePath = systemTheme
        ? QString{} : BreezeTheme::colorSchemePath(variant);
    QString styleName;
    if (systemTheme) {
        styleName = m_systemStyleName;
    } else {
        const QString platformName = QApplication::platformName();
        const bool nativeStylesAvailable =
            platformName != QStringLiteral("offscreen")
            && platformName != QStringLiteral("minimal")
            && !colorSchemePath.isEmpty();
        styleName = BreezeTheme::preferredStyleName(
            QStyleFactory::keys(), nativeStylesAvailable);
    }

    // Breeze gives top tool areas a separate KColorScheme::Header palette.
    // Point it at the same scheme as Clearveil before the application palette
    // change makes Breeze refresh that header palette.
    if (systemTheme && m_systemColorSchemePathWasSet) {
        qApp->setProperty(breezeColorSchemeProperty,
                          m_systemColorSchemePath);
    } else if (!systemTheme && styleName.compare(
                   QStringLiteral("Breeze"),
                   Qt::CaseInsensitive) == 0) {
        qApp->setProperty(breezeColorSchemeProperty,
                          colorSchemePath);
    } else {
        qApp->setProperty(breezeColorSchemeProperty, QVariant{});
    }
    if (qApp->style()->objectName().compare(
            styleName, Qt::CaseInsensitive) != 0) {
        if (QStyle *style = QStyleFactory::create(styleName))
            QApplication::setStyle(style);
    }

    QPalette palette = m_systemPalette;
    if (!systemTheme)
        palette = BreezeTheme::palette(variant);
    qApp->setPalette(palette);
    applyStyle();
    for (QWidget *widget : QApplication::allWidgets()) {
        widget->setPalette(QPalette());
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
    // Breeze may repolish an existing popup menu using the desktop color
    // scheme.  Keep Clearveil's already-created menu hierarchy on the
    // explicitly selected built-in palette after switching away from a
    // differently colored system theme.
    if (menuBar())
        menuBar()->setPalette(palette);
    for (QMenu *menu : findChildren<QMenu *>())
        menu->setPalette(palette);
    if (m_browser)
        m_browser->refreshAppearance();
    positionCornerMenuButton();
    m_applyingTheme = wasApplyingTheme;
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if ((event->type() != QEvent::ApplicationPaletteChange
         && event->type() != QEvent::PaletteChange)
        || m_applyingTheme
        || m_settings.theme != QStringLiteral("system")) {
        return;
    }

    m_systemPalette = QApplication::palette();
    m_applyingTheme = true;
    applyStyle();
    if (m_browser)
        m_browser->refreshAppearance();
    m_applyingTheme = false;
}

void MainWindow::setLayoutLocked(bool locked)
{
    m_layoutLocked = locked;
    if (m_layoutLockAction
        && m_layoutLockAction->isChecked() != locked) {
        const QSignalBlocker blocker(m_layoutLockAction);
        m_layoutLockAction->setChecked(locked);
    }

    if (m_mainToolbar) {
        if (locked
            && toolBarArea(m_mainToolbar)
                == Qt::NoToolBarArea) {
            addToolBar(
                Qt::TopToolBarArea, m_mainToolbar);
        }
        m_mainToolbar->setMovable(!locked);
        m_mainToolbar->setFloatable(!locked);
    }

    if (m_panelLayoutController)
        m_panelLayoutController->setLocked(locked);
}

void MainWindow::updateZoomModeActions()
{
    if (!m_canvas || !m_zoomModeGroup)
        return;
    QAction *active = nullptr;
    switch (m_canvas->zoomMode()) {
    case ImageCanvas::ZoomMode::Fit:
        active = m_fitAction;
        break;
    case ImageCanvas::ZoomMode::FitWidth:
        active = m_fitWidthAction;
        break;
    case ImageCanvas::ZoomMode::FitHeight:
        active = m_fitHeightAction;
        break;
    case ImageCanvas::ZoomMode::Fill:
        active = m_fillAction;
        break;
    case ImageCanvas::ZoomMode::ActualSize:
        active = m_actualSizeAction;
        break;
    case ImageCanvas::ZoomMode::Custom:
        break;
    }
    for (QAction *action : m_zoomModeGroup->actions())
        action->setChecked(action == active);
}

void MainWindow::updateMenuBarVisibility()
{
    if (!menuBar() || !statusBar())
        return;
    const bool toolbarEnabled = m_mainToolbar
        && m_mainToolbar->isVisible();
    const bool menuBarVisible = !isFullScreen() && m_menuBarAction
        && m_menuBarAction->isChecked();
    menuBar()->setVisible(menuBarVisible);
    if (!isFullScreen()) {
        statusBar()->setVisible(
            m_statusBarAction && m_statusBarAction->isChecked());
    }
    if (m_cornerMenuButton) {
        m_cornerMenuButton->setVisible(!toolbarEnabled && !menuBarVisible);
        positionCornerMenuButton();
        m_cornerMenuButton->raise();
    }
}

void MainWindow::applyFullscreenComponentVisibility()
{
    if (m_windowModeController)
        m_windowModeController->applyFullscreenComponentVisibility();
}

void MainWindow::updateToolbarDensity()
{
    if (!m_mainToolbar)
        return;

    constexpr int compactThreshold = 1040;
    const bool compact = width() < compactThreshold;
    if (m_mainToolbar->property("compact").toBool() == compact
        && m_mainToolbar->iconSize()
            == (compact ? QSize(18, 18) : QSize(22, 22))) {
        return;
    }

    m_mainToolbar->setProperty("compact", compact);
    m_mainToolbar->setIconSize(compact ? QSize(18, 18) : QSize(22, 22));
    m_mainToolbar->style()->unpolish(m_mainToolbar);
    m_mainToolbar->style()->polish(m_mainToolbar);
    for (QToolButton *button
         : m_mainToolbar->findChildren<QToolButton *>()) {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
    m_mainToolbar->updateGeometry();
}

void MainWindow::positionCornerMenuButton()
{
    if (!m_cornerMenuButton)
        return;
    constexpr int margin = 8;
    const int top = menuBar() && menuBar()->isVisible()
        ? menuBar()->height() + margin : margin;
    m_cornerMenuButton->move(margin, top);
}

void MainWindow::scheduleFitWindowToImage(
    bool restoreNormalWindow)
{
    if (!m_windowModeController || displayedImage().isNull())
        return;

    qreal scale = 1.0;
    if (m_canvas->zoomMode() == ImageCanvas::ZoomMode::Custom)
        scale = m_canvas->zoom();
    const QSize imageSize(
        std::max(1, qRound(m_document.logicalSize().width() * scale)),
        std::max(1, qRound(m_document.logicalSize().height() * scale)));
    m_windowModeController->scheduleFitWindowToContent(
        imageSize, restoreNormalWindow);
}

void MainWindow::applyWindowModeFlags()
{
    if (m_windowModeController)
        m_windowModeController->applyWindowModeFlags();
}

void MainWindow::loadSettings()
{
    QSettings storage;
    m_settings = ApplicationSettings::load(storage);
    if (m_browser) {
        m_browser->setStoredLocations(
            m_settings.recentFolders,
            m_settings.favoriteFolders);
    }
    QStringList storedToolbarLayout = m_settings.toolbarLayout;
    if (storedToolbarLayout.isEmpty()) {
        // An empty list means no toolbar preference has ever been saved.
        // Apply the complete first-run layout before running migrations;
        // inserting newly introduced actions into the empty list would turn
        // it into an accidental three-button custom layout.
        m_settings.toolbarLayout =
            m_actionRegistry->defaultToolbarLayout();
    } else {
        for (qsizetype index = storedToolbarLayout.size();
             index-- > 0;) {
            QString id = storedToolbarLayout.at(index);
            if (id.startsWith(QLatin1Char('!')))
                id.remove(0, 1);
            if (id == QStringLiteral("ocr_text")
                || id == QStringLiteral("recognize_text")) {
                storedToolbarLayout.removeAt(index);
            }
        }
        const auto hasToolbarItem = [&storedToolbarLayout](
            const QString &id) {
            return std::any_of(
                storedToolbarLayout.cbegin(),
                storedToolbarLayout.cend(),
                [&id](const QString &item) {
                    return item == id
                        || item == QStringLiteral("!") + id;
                });
        };
        int colorPickerIndex = -1;
        for (int index = 0; index < storedToolbarLayout.size();
             ++index) {
            const QString &item = storedToolbarLayout.at(index);
            if (item == QStringLiteral("color_picker")
                || item == QStringLiteral("!color_picker")) {
                colorPickerIndex = index;
                break;
            }
        }
        if (colorPickerIndex < 0)
            colorPickerIndex = storedToolbarLayout.size();
        if (!hasToolbarItem(QStringLiteral("pointer_hand"))) {
            storedToolbarLayout.insert(
                colorPickerIndex++, QStringLiteral("pointer_hand"));
        }
        if (!hasToolbarItem(QStringLiteral("text_select"))) {
            storedToolbarLayout.insert(
                colorPickerIndex, QStringLiteral("text_select"));
        }
        if (!hasToolbarItem(QStringLiteral("filmstrip_source"))) {
            const int filmstripIndex = storedToolbarLayout.indexOf(
                QStringLiteral("filmstrip"));
            storedToolbarLayout.insert(
                filmstripIndex >= 0 ? filmstripIndex + 1
                                    : storedToolbarLayout.size(),
                QStringLiteral("filmstrip_source"));
        }
        m_settings.toolbarLayout =
            m_actionRegistry->normalizedToolbarLayout(
                storedToolbarLayout);
    }
    applyToolbarLayout();
    const QStringList storedShortcutLayout = m_settings.shortcutLayout;
    m_settings.shortcutLayout = m_actionRegistry->normalizedShortcutLayout(
        storedShortcutLayout.isEmpty()
            ? m_actionRegistry->defaultShortcutLayout() : storedShortcutLayout);
    applyShortcuts();
    PersistentThumbnailCache::configure(
        m_settings.persistentThumbnailCacheEnabled,
        static_cast<qint64>(
            m_settings.persistentThumbnailCacheMiB)
            * 1024LL * 1024LL);
    m_imageLoader->setCacheLimitMiB(
        m_settings.imageMemoryCacheMiB);
    applyMouseActions();
    applyTheme(m_settings.theme);
    applyFilmstripPreferences();
    m_slideshowController->setIntervalMs(
        m_settings.slideshowIntervalMs);
    m_slideshowController->setRandomOrder(
        m_settings.randomSlideshow);
    m_slideshowController->setFullscreenEnabled(
        m_settings.fullscreenSlideshow);
    m_filmstripAction->setChecked(m_settings.showFilmstrip);
    if (m_canvasAppearanceController) {
        m_canvasAppearanceController
            ->setTransparencyCheckerboardVisible(
                m_settings.showTransparencyCheckerboard);
    }
    m_menuBarAction->setChecked(m_settings.showMenuBar);
    m_statusBarAction->setChecked(m_settings.showStatusBar);
    m_layoutLocked = m_settings.layoutLocked;
    m_fullscreenToolbarAction->setChecked(
        m_settings.showToolbarInFullscreen);
    m_fullscreenFilmstripAction->setChecked(
        m_settings.showFilmstripInFullscreen);
    m_fullscreenStatusBarAction->setChecked(
        m_settings.showStatusBarInFullscreen);
    m_fullscreenInformationAction->setChecked(
        m_settings.showInformationInFullscreen);
    {
        const QSignalBlocker fitWindowBlocker(
            m_fitWindowToImageAction);
        const QSignalBlocker borderlessBlocker(
            m_borderlessAction);
        const QSignalBlocker alwaysOnTopBlocker(
            m_alwaysOnTopAction);
        m_fitWindowToImageAction->setChecked(
            m_settings.fitWindowToImage);
        m_borderlessAction->setChecked(
            m_settings.borderless);
        m_alwaysOnTopAction->setChecked(
            m_settings.alwaysOnTop);
    }
    applyWindowModeFlags();
    if (!m_settings.windowGeometry.isEmpty())
        restoreGeometry(m_settings.windowGeometry);
    if (!m_settings.windowState.isEmpty())
        restoreState(m_settings.windowState, 1);
    if (m_panelLayoutController)
        m_panelLayoutController->restore(storage);
    QTimer::singleShot(0, this, [this] {
        applyPanelOrder(m_settings.panelOrder);
    });
    if (m_filmstripLayoutController) {
        m_filmstripLayoutController->setModeName(
            m_settings.floatingThumbnailLayout);
    }
    if (m_mainToolbar)
        m_mainToolbar->setVisible(m_settings.showToolbar);
    if (m_metadataAction)
        m_metadataAction->setChecked(m_settings.showInformation);
    if (m_metadataDock)
        m_metadataDock->setVisible(m_settings.showInformation);
    if (m_colorPickerAction)
        m_colorPickerAction->setChecked(m_settings.showColorPicker);
    if (m_colorPickerController)
        m_colorPickerController->setEnabled(
            m_settings.showColorPicker);
    if (m_colorPickerDock)
        m_colorPickerDock->setVisible(m_settings.showColorPicker);
    setLayoutLocked(m_layoutLocked);
    updateFilmstripLayout(dockWidgetArea(m_filmstripDock));
    toggleFilmstrip();
    updateMenuBarVisibility();
}

void MainWindow::saveSettings()
{
    m_settings.slideshowIntervalMs =
        m_slideshowController->intervalMs();
    m_settings.showToolbar = m_mainToolbar
        && m_mainToolbar->isVisible();
    m_settings.showFilmstrip = m_filmstripDock
        && m_filmstripDock->isVisible();
    m_settings.showInformation = m_metadataDock
        && m_metadataDock->isVisible();
    m_settings.showColorPicker = m_colorPickerDock
        && m_colorPickerDock->isVisible();
    m_settings.panelOrder = currentPanelOrder();
    if (m_canvasAppearanceController) {
        m_settings.showTransparencyCheckerboard =
            m_canvasAppearanceController
                ->transparencyCheckerboardVisible();
    }
    m_settings.showMenuBar = m_menuBarAction->isChecked();
    m_settings.showStatusBar = m_statusBarAction->isChecked();
    m_settings.layoutLocked = m_layoutLocked;
    if (m_filmstripLayoutController) {
        m_settings.floatingThumbnailLayout =
            m_filmstripLayoutController->modeName();
    }
    m_settings.fitWindowToImage =
        m_fitWindowToImageAction->isChecked();
    m_settings.borderless = m_borderlessAction->isChecked();
    m_settings.alwaysOnTop = m_alwaysOnTopAction->isChecked();
    m_settings.showToolbarInFullscreen =
        m_fullscreenToolbarAction->isChecked();
    m_settings.showFilmstripInFullscreen =
        m_fullscreenFilmstripAction->isChecked();
    m_settings.showStatusBarInFullscreen =
        m_fullscreenStatusBarAction->isChecked();
    m_settings.showInformationInFullscreen =
        m_fullscreenInformationAction->isChecked();
    m_settings.windowGeometry = saveGeometry();
    m_settings.windowState = saveState(1);
    if (m_browser) {
        m_settings.recentFolders =
            m_browser->recentDirectories();
        m_settings.favoriteFolders =
            m_browser->favoriteDirectories();
    }
    QSettings storage;
    m_settings.save(storage);
    if (m_panelLayoutController)
        m_panelLayoutController->save(storage);
    storage.sync();
}

void MainWindow::rebuildSequence(const QString &currentFile)
{
    m_session.appendOpenedFiles({currentFile});
    m_session.setLoadedPath(currentFile);
    m_session.invalidateDirectory();
    rebuildFilmstrip();
    updateActions();
    updateStatus();
}

void MainWindow::rebuildFilmstrip()
{
    if (!m_filmstrip || !m_openedFilmstripModel
        || !m_directoryFilmstripModel) {
        return;
    }

    m_openedFilmstripModel->setFiles(m_session.openedFiles());
    const QFileInfo currentFile(m_document.filePath());
    const QString directoryPath = currentFile.isFile()
        ? currentFile.absolutePath() : QString();
    if (!directoryPath.isEmpty()
        && directoryPath != m_session.directoryPath()
        && (!m_directoryScanRequestId
            || m_directoryScanPath != directoryPath)) {
        requestDirectoryScan(directoryPath);
    }

    syncFilmstripSelection();
    toggleFilmstrip();
}

void MainWindow::syncFilmstripSelection()
{
    if (m_filmstripController)
        m_filmstripController->syncSelection(
            m_document.filePath());
}

void MainWindow::activateFilmstripRow(int row)
{
    if (!m_filmstripController) {
        return;
    }
    const QString path = m_filmstripController->pathAt(row);
    if (path.isEmpty()
        || QFileInfo(path).absoluteFilePath()
            == QFileInfo(m_document.filePath()).absoluteFilePath()) {
        return;
    }

    if (m_filmstripController->source()
        == FilmstripController::Source::CurrentDirectory) {
        if (!confirmDiscardChanges()) {
            syncFilmstripSelection();
            return;
        }
        requestDocumentPath(path, -1);
        return;
    }
    setCurrentSequenceIndex(m_session.openedIndexOf(path));
}

void MainWindow::openDirectoryFilmstripRow(int row)
{
    if (!m_filmstripController
        || m_filmstripController->source()
            != FilmstripController::Source::CurrentDirectory
        || row < 0
        || row >= m_filmstripController->count()) {
        return;
    }

    const QString path = m_filmstripController->pathAt(row);
    if (path.isEmpty())
        return;

    m_session.appendOpenedFiles({path});
    m_openedFilmstripModel->setFiles(
        m_session.openedFiles());
    if ((!m_imageLoader || !m_imageLoader->isLoading())
        && QFileInfo(m_document.filePath()).absoluteFilePath()
            == QFileInfo(path).absoluteFilePath()) {
        m_session.setLoadedPath(path);
    }
    updateActions();
    updateStatus();
}

void MainWindow::closeOpenedImageAt(int row)
{
    if (!m_openedFilmstripModel || !m_filmstripController
        || m_filmstripController->source()
            != FilmstripController::Source::OpenedImages
        || row < 0 || row >= m_openedFilmstripModel->rowCount()) {
        return;
    }

    const QString path = m_openedFilmstripModel->filePath(
        m_openedFilmstripModel->index(row));
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    const QString currentPath =
        QFileInfo(m_document.filePath()).absoluteFilePath();
    const bool closesCurrent = absolutePath == currentPath;
    if (closesCurrent && !confirmDiscardChanges())
        return;

    cancelPendingImageLoad();
    const ImageSessionController::RemovalResult removal =
        m_session.removeOpenedAt(row, currentPath);
    if (!removal.removed)
        return;
    if (removal.openedImagesEmpty) {
        if (closesCurrent) {
            m_documentWorkflowController->clear();
            m_session.invalidateDirectory();
        }
        rebuildFilmstrip();
        updateActions();
        updateStatus();
        return;
    }

    if (!closesCurrent) {
        rebuildFilmstrip();
        updateActions();
        updateStatus();
        return;
    }

    rebuildFilmstrip();
    requestDocumentPath(removal.nextPath, removal.nextIndex);
}

void MainWindow::setFilmstripSource(bool currentDirectory)
{
    if (!m_filmstrip || !m_filmstripController
        || !m_directoryFilmstripModel) {
        return;
    }

    if (currentDirectory) {
        const QFileInfo currentFile(m_document.filePath());
        const QString directoryPath = currentFile.isFile()
            ? currentFile.absolutePath() : QString();
        if (!directoryPath.isEmpty()
            && directoryPath != m_session.directoryPath()
            && !m_directoryScanRequestId) {
            requestDirectoryScan(directoryPath);
        }
    }
    const FilmstripController::Source source = currentDirectory
        ? FilmstripController::Source::CurrentDirectory
        : FilmstripController::Source::OpenedImages;
    m_filmstripController->setSource(source);
    {
        const QSignalBlocker blocker(m_filmstripSourceAction);
        m_filmstripSourceAction->setChecked(currentDirectory);
    }
    if (auto *filmstripView =
            dynamic_cast<FilmstripView *>(m_filmstrip)) {
        filmstripView->setCloseButtonsVisible(!currentDirectory);
    }
    m_filmstrip->setAccessibleName(currentDirectory
        ? tr("Current folder") : tr("Opened images"));
    syncFilmstripSelection();
    toggleFilmstrip();
    updateStatus();
    updateNavigationActions();
    m_filmstrip->viewport()->update();
}

void MainWindow::updateFilmstripLayout(Qt::DockWidgetArea area)
{
    if (!m_filmstrip)
        return;
    if (m_filmstripLayoutController)
        m_filmstripLayoutController->updateLayout();
    const bool vertical = m_filmstripLayoutController
        ? m_filmstripLayoutController->isVertical()
        : area == Qt::LeftDockWidgetArea
            || area == Qt::RightDockWidgetArea;
    if (auto *filmstripView =
            dynamic_cast<FilmstripView *>(m_filmstrip)) {
        filmstripView->setVerticalLayout(vertical);
    }
    m_filmstrip->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);
    m_filmstrip->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);
    if (vertical) {
        m_filmstrip->setMinimumWidth(110);
        m_filmstrip->setMinimumHeight(0);
    } else {
        m_filmstrip->setMinimumWidth(0);
        m_filmstrip->setMinimumHeight(86);
    }
    if (auto *filmstripView =
            dynamic_cast<FilmstripView *>(m_filmstrip)) {
        filmstripView->setCloseButtonsVisible(
            m_filmstripController
            && m_filmstripController->source()
                == FilmstripController::Source::OpenedImages);
    }
    if (auto *filmstripView =
            dynamic_cast<FilmstripView *>(m_filmstrip)) {
        filmstripView->updateOverlayScrollBars();
    }
    QTimer::singleShot(
        0, this, &MainWindow::updateFilmstripThumbnailSize);
}

void MainWindow::updateFilmstripThumbnailSize()
{
    if (!m_filmstrip || !m_filmstripController
        || !m_filmstrip->viewport()) {
        return;
    }

    const QSize viewportSize = m_filmstrip->viewport()->size();
    if (viewportSize.width() <= 0 || viewportSize.height() <= 0)
        return;

    const auto *filmstripView =
        dynamic_cast<const FilmstripView *>(m_filmstrip);
    const bool vertical = filmstripView
        && filmstripView->isVerticalLayout();
    QSize thumbnailSize;
    QSize gridSize;
    if (vertical) {
        const int columns = std::clamp(
            m_settings.filmstripVerticalColumns, 1, 4);
        const int usableWidth = std::max(
            48, viewportSize.width()
                - FilmstripView::overlayExtent());
        const int cellWidth = std::max(
            48, usableWidth / columns);
        const int thumbnailWidth = std::clamp(
            cellWidth - 16, 36,
            m_settings.filmstripThumbnailExtent);
        const int thumbnailHeight =
            std::max(36, qRound(thumbnailWidth * 0.75));
        thumbnailSize = QSize(thumbnailWidth, thumbnailHeight);
        const int labelExtent = m_settings.showFilmstripFileNames
            ? 26 : 14;
        gridSize = QSize(cellWidth, thumbnailHeight + labelExtent);
    } else {
        const int labelAndMargins =
            (m_settings.showFilmstripFileNames
                 ? m_filmstrip->fontMetrics().height() + 18
                 : 12)
            + FilmstripView::overlayExtent();
        const int thumbnailHeight =
            std::clamp(viewportSize.height() - labelAndMargins,
                       36, m_settings.filmstripThumbnailExtent);
        const int thumbnailWidth =
            std::max(48, qRound(thumbnailHeight * 4.0 / 3.0));
        thumbnailSize = QSize(thumbnailWidth, thumbnailHeight);
        gridSize = QSize(thumbnailWidth + 14, viewportSize.height());
    }

    if (m_filmstrip->iconSize() != thumbnailSize)
        m_filmstrip->setIconSize(thumbnailSize);
    if (m_filmstrip->gridSize() != gridSize)
        m_filmstrip->setGridSize(gridSize);

    m_pendingFilmstripThumbnailSize = thumbnailSize;
    if ((m_openedFilmstripModel
         && m_openedFilmstripModel->thumbnailSize() != thumbnailSize)
        || (m_directoryFilmstripModel
            && m_directoryFilmstripModel->thumbnailSize()
                != thumbnailSize)) {
        m_filmstripResizeTimer->start();
    }
}

void MainWindow::updateColorPickerDockSizeConstraint()
{
    if (!m_colorPickerDock || !m_colorPickerPanel)
        return;
    const bool overlay = m_panelLayoutController
        && m_panelLayoutController->isOverlay(
            m_colorPickerDock);
    const Qt::DockWidgetArea area = dockWidgetArea(
        m_colorPickerDock);
    const bool compactVerticalDock =
        !overlay && !m_colorPickerDock->isFloating()
        && (area == Qt::LeftDockWidgetArea
            || area == Qt::RightDockWidgetArea);
    if (!compactVerticalDock) {
        m_colorPickerDock->setMaximumHeight(
            QWIDGETSIZE_MAX);
        return;
    }

    const int titleHeight = m_colorPickerDock->titleBarWidget()
        ? m_colorPickerDock->titleBarWidget()->sizeHint().height()
        : style()->pixelMetric(QStyle::PM_TitleBarHeight);
    const int preferredHeight = std::max(
        m_colorPickerDock->minimumSizeHint().height(),
        m_colorPickerPanel->preferredHeight()
            + titleHeight + 2);
    m_colorPickerDock->setMaximumHeight(preferredHeight);
    m_colorPickerDock->updateGeometry();
}

void MainWindow::applyPanelOrder(const QStringList &panelOrder)
{
    const QStringList defaults{
        QStringLiteral("thumbnails"),
        QStringLiteral("information"),
        QStringLiteral("colorPicker")};
    QStringList order;
    for (const QString &panelId : panelOrder) {
        if (defaults.contains(panelId) && !order.contains(panelId))
            order.append(panelId);
    }
    for (const QString &panelId : defaults) {
        if (!order.contains(panelId))
            order.append(panelId);
    }
    const auto dockForId = [this](const QString &panelId) {
        if (panelId == QStringLiteral("thumbnails"))
            return m_filmstripDock;
        if (panelId == QStringLiteral("information"))
            return m_metadataDock;
        if (panelId == QStringLiteral("colorPicker"))
            return m_colorPickerDock;
        return static_cast<QDockWidget *>(nullptr);
    };

    const QList<Qt::DockWidgetArea> areas{
        Qt::TopDockWidgetArea, Qt::BottomDockWidgetArea,
        Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea};
    for (Qt::DockWidgetArea area : areas) {
        QList<QDockWidget *> docks;
        for (const QString &panelId : order) {
            QDockWidget *dock = dockForId(panelId);
            if (!dock || dock->isFloating()
                || (m_panelLayoutController
                    && m_panelLayoutController->isOverlay(dock))
                || dockWidgetArea(dock) != area) {
                continue;
            }
            docks.append(dock);
        }
        const Qt::Orientation orientation =
            area == Qt::LeftDockWidgetArea
                || area == Qt::RightDockWidgetArea
            ? Qt::Vertical : Qt::Horizontal;
        for (int index = 1; index < docks.size(); ++index)
            splitDockWidget(docks.at(index - 1), docks.at(index),
                            orientation);
    }
}

QStringList MainWindow::currentPanelOrder() const
{
    const QStringList defaults{
        QStringLiteral("thumbnails"),
        QStringLiteral("information"),
        QStringLiteral("colorPicker")};
    QStringList result;
    for (const QString &panelId : m_settings.panelOrder) {
        if (defaults.contains(panelId) && !result.contains(panelId))
            result.append(panelId);
    }
    for (const QString &panelId : defaults) {
        if (!result.contains(panelId))
            result.append(panelId);
    }
    const auto dockForId = [this](const QString &panelId) {
        if (panelId == QStringLiteral("thumbnails"))
            return m_filmstripDock;
        if (panelId == QStringLiteral("information"))
            return m_metadataDock;
        if (panelId == QStringLiteral("colorPicker"))
            return m_colorPickerDock;
        return static_cast<QDockWidget *>(nullptr);
    };

    const QList<Qt::DockWidgetArea> areas{
        Qt::TopDockWidgetArea, Qt::BottomDockWidgetArea,
        Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea};
    for (Qt::DockWidgetArea area : areas) {
        QList<int> orderPositions;
        QStringList dockedIds;
        for (int index = 0; index < result.size(); ++index) {
            QDockWidget *dock = dockForId(result.at(index));
            if (!dock || dock->isFloating()
                || (m_panelLayoutController
                    && m_panelLayoutController->isOverlay(dock))
                || dockWidgetArea(dock) != area) {
                continue;
            }
            orderPositions.append(index);
            dockedIds.append(result.at(index));
        }
        const bool vertical = area == Qt::LeftDockWidgetArea
            || area == Qt::RightDockWidgetArea;
        std::stable_sort(
            dockedIds.begin(), dockedIds.end(),
            [&](const QString &left, const QString &right) {
                QDockWidget *leftDock = dockForId(left);
                QDockWidget *rightDock = dockForId(right);
                return vertical
                    ? leftDock->geometry().top()
                        < rightDock->geometry().top()
                    : leftDock->geometry().left()
                        < rightDock->geometry().left();
            });
        for (int index = 0; index < orderPositions.size(); ++index)
            result[orderPositions.at(index)] = dockedIds.at(index);
    }
    return result;
}

void MainWindow::applyFilmstripPreferences()
{
    if (auto *filmstripView =
            dynamic_cast<FilmstripView *>(m_filmstrip)) {
        filmstripView->setFileNamesVisible(
            m_settings.showFilmstripFileNames);
    }
    if (m_directoryFilmstripModel) {
        m_directoryFilmstripModel->setSort(
            thumbnailSortKey(
                m_settings.directoryThumbnailSortKey),
            m_settings.directoryThumbnailSortAscending
                ? Qt::AscendingOrder : Qt::DescendingOrder);
    }
    updateFilmstripThumbnailSize();
    syncFilmstripSelection();
}

void MainWindow::syncSlideshowNavigationState()
{
    if (!m_slideshowController)
        return;
    if (m_filmstripController
        && m_filmstripController->source()
            == FilmstripController::Source::CurrentDirectory) {
        m_slideshowController->setNavigationState(
            m_filmstripController->count(),
            m_filmstripController->currentRow());
        return;
    }
    m_slideshowController->setNavigationState(
        m_session.openedCount(),
        m_session.effectiveOpenedIndex());
}

void MainWindow::setCurrentSequenceIndex(int index)
{
    if (index < 0 || index >= m_session.openedCount())
        return;
    if (index == m_session.currentOpenedIndex()) {
        if (m_session.pendingOpenedIndex() >= 0)
            cancelPendingImageLoad();
        syncFilmstripSelection();
        return;
    }
    if (!confirmDiscardChanges())
        return;

    const QString path = m_session.openedPathAt(index);
    requestDocumentPath(path, index);
}

void MainWindow::requestDocumentPath(const QString &path, int sequenceIndex)
{
    if (!m_viewerNavigationController)
        return;
    m_viewerNavigationController->requestPath(
        path, sequenceIndex);
}

void MainWindow::prefetchAdjacentImages(int sequenceIndex)
{
    if (m_viewerNavigationController) {
        m_viewerNavigationController->prefetchAdjacent(
            sequenceIndex);
    }
}

void MainWindow::cancelPendingImageLoad()
{
    if (!m_viewerNavigationController
        || !m_viewerNavigationController->cancel()) {
        return;
    }
    statusBar()->clearMessage();
    if (m_openedFilmstripModel)
        m_openedFilmstripModel->setPrimaryImagePath({});
    if (m_directoryFilmstripModel)
        m_directoryFilmstripModel->setPrimaryImagePath({});
    updateActions();
}

bool MainWindow::loadDocumentPath(const QString &path, QString *errorMessage)
{
    cancelPendingImageLoad();
    if (!m_documentWorkflowController->loadPath(
            path, errorMessage)) {
        return false;
    }
    // Initial/file-manager opens are synchronous and do not pass through the
    // navigation loader. Seed the same cache so returning to this image can
    // reuse its large preview and region source as well.
    if (m_imageLoader) {
        ImageLoadResult loaded;
        loaded.filePath = m_document.filePath();
        loaded.image = m_document.image();
        loaded.source = m_document.imageSource();
        m_imageLoader->remember(loaded);
    }
    updateFrameControls();
    updateMetadataPanel();
    return true;
}

void MainWindow::showBrowser(const QString &directoryPath)
{
    if (!m_browser || !m_viewStack)
        return;
    m_browser->setCurrentPath(m_document.filePath());
    m_browser->setDirectory(directoryPath);
    if (m_frames.isAnimated())
        m_frames.setPlaying(false);
    m_viewStack->setCurrentWidget(m_browser);
    if (m_filmstripDock) {
        const QSignalBlocker blocker(m_filmstripDock);
        m_filmstripDock->hide();
    }
    {
        const QSignalBlocker blocker(m_browseAction);
        m_browseAction->setChecked(true);
    }
    m_frameBar->hide();
    updateStatus();
}

void MainWindow::showViewer()
{
    if (!m_viewStack)
        return;
    m_viewStack->setCurrentWidget(m_canvas);
    {
        const QSignalBlocker blocker(m_browseAction);
        m_browseAction->setChecked(false);
    }
    updateFrameControls();
    toggleFilmstrip();
    updateStatus();
}

void MainWindow::showCompare(const QStringList &filePaths)
{
    if (!m_compare || !m_viewStack || filePaths.size() < 2)
        return;
    if (m_frames.isAnimated())
        m_frames.setPlaying(false);
    m_compare->setFiles(filePaths);
    m_viewStack->setCurrentWidget(m_compare);
    if (m_filmstripDock) {
        const QSignalBlocker blocker(m_filmstripDock);
        m_filmstripDock->hide();
    }
    {
        const QSignalBlocker blocker(m_browseAction);
        m_browseAction->setChecked(true);
    }
    m_frameBar->hide();
    m_uiStateController->showCollection(
        tr("Compare"),
        std::min(4, static_cast<int>(filePaths.size())));
}

bool MainWindow::isBrowseMode() const
{
    return m_viewStack && m_browser
        && m_viewStack->currentWidget() == m_browser;
}

void MainWindow::reloadCurrentFile()
{
    const QString path = m_document.filePath();
    if (path.isEmpty())
        return;
    if (!QFileInfo::exists(path)) {
        statusBar()->showMessage(tr("The current file was removed or moved."), 6000);
        return;
    }
    if (m_document.isModified())
        return;

    QString error;
    cancelPendingImageLoad();
    if (!m_documentWorkflowController->reload(&error)) {
        statusBar()->showMessage(tr("Could not reload the changed file: %1").arg(error),
                                 7000);
        return;
    }
    updateFrameControls();
    updateMetadataPanel();
    rebuildFilmstrip();
    statusBar()->showMessage(tr("Reloaded after an external change."), 3000);
}

void MainWindow::startOcrRecognition()
{
    if (!m_ocrController || !shouldRecognizeText()) {
        return;
    }
    const QImage &image = displayedImage();
    if (image.isNull())
        return;
    if (m_canvas && m_canvas->hasOcrText()) {
        statusBar()->showMessage(
            tr("Drag across text to select it."), 2500);
        return;
    }
    const QSize logicalSize = m_frames.isActive()
        ? image.size() : m_document.logicalSize();
    m_ocrController->recognize(
        image, logicalSize, m_ocrLanguages);
}

bool MainWindow::shouldRecognizeText() const
{
    return m_textSelectionAction
        && m_textSelectionAction->isChecked()
        && OcrEngine::isAvailable()
        && !m_ocrLanguages.isEmpty();
}

const QImage &MainWindow::displayedImage() const
{
    return m_documentWorkflowController->displayedImage();
}

void MainWindow::updateFrameControls()
{
    if (!m_frameBar)
        return;
    const bool active = m_frames.isActive();
    const bool viewerVisible = m_viewStack
        && m_viewStack->currentWidget() == m_canvas;
    m_frameBar->setVisible(active && viewerVisible);
    const int current = m_frames.currentFrame();
    const int count = m_frames.frameCount();
    m_frameFirstAction->setEnabled(active && current > 0);
    m_framePreviousAction->setEnabled(active && current > 0);
    m_frameNextAction->setEnabled(active && count > 0 && current + 1 < count);
    m_frameLastAction->setEnabled(active && count > 0 && current + 1 < count);
    m_framePlayAction->setEnabled(m_frames.isAnimated());
    {
        const QSignalBlocker blocker(m_framePlayAction);
        m_framePlayAction->setChecked(m_frames.isPlaying());
    }
    m_framePlayAction->setIcon(style()->standardIcon(
        m_frames.isPlaying() ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
    m_framePlayAction->setText(m_frames.isPlaying()
        ? tr("Pause animation") : tr("Play animation"));
    m_frameLabel->setText(count > 0
        ? tr("%1 / %2").arg(current + 1).arg(count)
        : tr("%1 / ?").arg(std::max(1, current + 1)));
    updateActions();
}

void MainWindow::updateMetadataPanel()
{
    if (!m_metadataPanel || !m_metadataDock || !m_metadataDock->isVisible())
        return;
    const QImage &image = displayedImage();
    if (image.isNull())
        m_metadataPanel->clear();
    else
        m_metadataPanel->setImage(
            m_document.filePath(), image,
            m_frames.isActive() ? image.size()
                                : m_document.logicalSize());
}

bool MainWindow::confirmDiscardChanges()
{
    if (!m_document.isModified())
        return true;
    const QMessageBox::StandardButton result = QMessageBox::warning(
        this, tr("Unsaved changes"),
        tr("The current image has unsaved changes."),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (result == QMessageBox::Cancel)
        return false;
    if (result == QMessageBox::Discard)
        return true;
    saveAs();
    return !m_document.isModified();
}

void MainWindow::updateStatus()
{
    if (isBrowseMode()) {
        const QFileInfo directory(m_browser->directoryPath());
        const QString name = directory.fileName().isEmpty()
            ? directory.absoluteFilePath() : directory.fileName();
        m_uiStateController->showCollection(
            name, m_browser->imageCount());
        return;
    }
    const QImage &image = displayedImage();
    if (image.isNull()) {
        m_uiStateController->showReady();
        return;
    }

    const QFileInfo file(m_document.filePath());
    QString displayedPosition =
        m_session.currentOpenedIndex() >= 0
        ? QString::number(m_session.currentOpenedIndex() + 1)
        : QStringLiteral("—");
    int displayedCount = m_session.openedCount();
    if (m_filmstripController
        && m_filmstripController->source()
            == FilmstripController::Source::CurrentDirectory
        && m_filmstripController
        && m_filmstripController->count() > 0) {
        displayedCount = m_filmstripController->count();
        const int directoryRow =
            m_filmstripController->rowForPath(
                file.absoluteFilePath());
        displayedPosition = directoryRow >= 0
            ? QString::number(directoryRow + 1)
            : QStringLiteral("—");
    }
    m_uiStateController->showImage({
        file.fileName(), m_document.isModified(),
        m_frames.isActive() ? image.size() : m_document.logicalSize(),
        file.exists(), file.size(), displayedPosition,
        displayedCount, m_canvas->zoom(),
        m_canvas->isZoomLocked()
    });
}

QAction *MainWindow::makeAction(const QString &text, const QKeySequence &shortcut,
                                const char *slot, const QString &accessibleName)
{
    auto *action = new QAction(text, this);
    action->setShortcut(shortcut);
    action->setObjectName(accessibleName.isEmpty() ? text : accessibleName);
    connect(action, SIGNAL(triggered()), this, slot);
    return action;
}
