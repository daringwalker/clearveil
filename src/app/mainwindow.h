#pragma once

#include "applicationsettings.h"
#include "framecontroller.h"
#include "imagedocument.h"
#include "imagesessioncontroller.h"
#include "selectablestatusbar.h"

#include <QList>
#include <QMainWindow>
#include <QPalette>
#include <QStringList>

class QAction;
class QActionGroup;
class ActionRegistry;
class BrowserWidget;
class BrowserFileOperationsController;
class CanvasAppearanceController;
class ColorPickerController;
class ColorPickerPanel;
class CompareWidget;
class DisplayColorController;
class DirectoryMonitor;
class DirectoryScanService;
class DocumentWorkflowController;
struct DirectoryScanResult;
class FilmstripController;
class FilmstripLayoutController;
struct InterfaceLayoutState;
class ImageCanvas;
class ImageEditController;
class ImageLoadController;
class MetadataPanel;
class OcrController;
class PanelLayoutController;
class SlideshowController;
class SettingsDialog;
class SystemAppearanceController;
class ThumbnailModel;
class ViewerNavigationController;
class ViewerUiStateController;
class WindowDragController;
class WindowModeController;
class QLabel;
class QListView;
class QTimer;
class QWidget;
class QStackedWidget;
class QDockWidget;
class QEvent;
class QMenu;
class QToolBar;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool openPath(const QString &path);
    bool openPaths(const QStringList &paths);
    void present();
    [[nodiscard]] SelectableStatusBar *statusBar() const;

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    QMenu *createPopupMenu() override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void openFile();
    void openFolder();
    void pasteImage();
    void copyImage();
    void saveAs();
    void cropImage();
    void resizeImage();
    void adjustImage();
    void reduceRedEye();
    void moveToTrash();
    void renameFile();
    void copyFileTo();
    void moveFileTo();
    void revealInFileManager();
    void openWithApplication();
    void printImage();
    void setAsWallpaper();
    void showPreferences();
    void showInterfaceLayout();
    void showFormatCapabilities();
    void showOcrSupport();
    void showAbout();
    void previousImage();
    void nextImage();
    void toggleFullscreen();
    void toggleFilmstrip();
    void toggleBrowseMode();
    void toggleSlideshow();
    void reloadCurrentFile();
    void documentChanged();
    void updateActions();
    void updateNavigationActions();
    void updateFrameControls();
    void updateMetadataPanel();

private:
    void buildUi();
    void buildActions();
    void buildActionRegistry();
    void buildMenus();
    void buildToolbar();
    void applyToolbarLayout();
    void applyShortcuts();
    void applyMouseActions();
    void applyPreferences(const SettingsDialog &dialog);
    void showPreferencesPage(bool interfaceLayout);
    [[nodiscard]] InterfaceLayoutState currentInterfaceLayout() const;
    void applyInterfaceLayout(const InterfaceLayoutState &layout);
    void applyStyle();
    void applyTheme(const QString &theme);
    void setLayoutLocked(bool locked);
    void updateZoomModeActions();
    void updateMenuBarVisibility();
    void applyFullscreenComponentVisibility();
    void updateToolbarDensity();
    void positionCornerMenuButton();
    void scheduleFitWindowToImage(bool restoreNormalWindow = false);
    void applyWindowModeFlags();
    void loadSettings();
    void saveSettings();
    void rebuildSequence(const QString &currentFile);
    void rebuildFilmstrip();
    void syncFilmstripSelection();
    void activateFilmstripRow(int row);
    void openDirectoryFilmstripRow(int row);
    void closeOpenedImageAt(int row);
    void setFilmstripSource(bool currentDirectory);
    void updateFilmstripLayout(Qt::DockWidgetArea area);
    void updateFilmstripThumbnailSize();
    void updateColorPickerDockSizeConstraint();
    void applyPanelOrder(const QStringList &panelOrder);
    [[nodiscard]] QStringList currentPanelOrder() const;
    void applyFilmstripPreferences();
    void syncSlideshowNavigationState();
    void setCurrentSequenceIndex(int index);
    void requestDocumentPath(const QString &path, int sequenceIndex);
    void startOcrRecognition();
    [[nodiscard]] bool shouldRecognizeText() const;
    void prefetchAdjacentImages(int sequenceIndex);
    void cancelPendingImageLoad();
    bool loadDocumentPath(const QString &path, QString *errorMessage = nullptr);
    bool openDirectoryPath(const QString &directoryPath);
    void openBrowserImage(const QString &path);
    void cancelPendingDirectoryScan();
    void requestDirectoryScan(const QString &directoryPath,
                              bool forceRefresh = false,
                              bool openImage = false,
                              const QString &preferredImage = {});
    void completeDirectoryScan(
        quint64 requestId,
        const DirectoryScanResult &result);
    void showBrowser(const QString &directoryPath);
    void showViewer();
    void showCompare(const QStringList &filePaths);
    [[nodiscard]] bool isBrowseMode() const;
    [[nodiscard]] const QImage &displayedImage() const;
    bool confirmDiscardChanges();
    void updateStatus();
    QAction *makeAction(const QString &text, const QKeySequence &shortcut,
                        const char *slot, const QString &accessibleName = {});

    ImageDocument m_document;
    FrameController m_frames;
    ImageSessionController m_session;
    ImageCanvas *m_canvas = nullptr;
    BrowserWidget *m_browser = nullptr;
    CompareWidget *m_compare = nullptr;
    QStackedWidget *m_viewStack = nullptr;
    QListView *m_filmstrip = nullptr;
    ThumbnailModel *m_openedFilmstripModel = nullptr;
    ThumbnailModel *m_directoryFilmstripModel = nullptr;
    FilmstripController *m_filmstripController = nullptr;
    FilmstripLayoutController *m_filmstripLayoutController = nullptr;
    QSize m_pendingFilmstripThumbnailSize;
    QTimer *m_filmstripResizeTimer = nullptr;
    QWidget *m_frameBar = nullptr;
    QLabel *m_frameLabel = nullptr;
    QLabel *m_fileLabel = nullptr;
    QLabel *m_detailLabel = nullptr;
    QLabel *m_zoomLabel = nullptr;
    ColorPickerController *m_colorPickerController = nullptr;
    CanvasAppearanceController *m_canvasAppearanceController = nullptr;
    DisplayColorController *m_displayColorController = nullptr;
    DocumentWorkflowController *m_documentWorkflowController = nullptr;
    ColorPickerPanel *m_colorPickerPanel = nullptr;
    QDockWidget *m_colorPickerDock = nullptr;
    MetadataPanel *m_metadataPanel = nullptr;
    QDockWidget *m_metadataDock = nullptr;
    QDockWidget *m_filmstripDock = nullptr;
    PanelLayoutController *m_panelLayoutController = nullptr;
    QMenu *m_mainMenu = nullptr;
    QToolBar *m_mainToolbar = nullptr;
    QToolButton *m_cornerMenuButton = nullptr;
    ImageLoadController *m_imageLoader = nullptr;
    ViewerNavigationController *m_viewerNavigationController = nullptr;
    ViewerUiStateController *m_uiStateController = nullptr;
    ImageEditController *m_editController = nullptr;
    SlideshowController *m_slideshowController = nullptr;
    OcrController *m_ocrController = nullptr;
    SystemAppearanceController *m_systemAppearanceController = nullptr;
    WindowDragController *m_windowDragController = nullptr;
    WindowModeController *m_windowModeController = nullptr;

    QAction *m_previousAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_openFolderAction = nullptr;
    QAction *m_pasteAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_saveAsAction = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_rotateLeftAction = nullptr;
    QAction *m_rotateRightAction = nullptr;
    QAction *m_flipHorizontalAction = nullptr;
    QAction *m_flipVerticalAction = nullptr;
    QAction *m_cropAction = nullptr;
    QAction *m_resizeAction = nullptr;
    QAction *m_adjustAction = nullptr;
    QAction *m_redEyeAction = nullptr;
    QAction *m_fitAction = nullptr;
    QAction *m_fitWidthAction = nullptr;
    QAction *m_fitHeightAction = nullptr;
    QAction *m_fillAction = nullptr;
    QAction *m_lockZoomAction = nullptr;
    QActionGroup *m_zoomModeGroup = nullptr;
    QAction *m_actualSizeAction = nullptr;
    QAction *m_zoomInAction = nullptr;
    QAction *m_zoomOutAction = nullptr;
    QAction *m_metadataAction = nullptr;
    QAction *m_handToolAction = nullptr;
    QAction *m_colorPickerAction = nullptr;
    QAction *m_textSelectionAction = nullptr;
    QActionGroup *m_pointerToolGroup = nullptr;
    QAction *m_ocrDebugAction = nullptr;
    QAction *m_checkerboardAction = nullptr;
    QAction *m_fullscreenAction = nullptr;
    QAction *m_fitWindowToImageAction = nullptr;
    QAction *m_borderlessAction = nullptr;
    QAction *m_alwaysOnTopAction = nullptr;
    QAction *m_fullscreenToolbarAction = nullptr;
    QAction *m_fullscreenFilmstripAction = nullptr;
    QAction *m_fullscreenStatusBarAction = nullptr;
    QAction *m_fullscreenInformationAction = nullptr;
    QAction *m_filmstripAction = nullptr;
    QAction *m_filmstripSourceAction = nullptr;
    QAction *m_browseAction = nullptr;
    QAction *m_slideshowAction = nullptr;
    QAction *m_trashAction = nullptr;
    QAction *m_renameAction = nullptr;
    QAction *m_copyFileAction = nullptr;
    QAction *m_moveFileAction = nullptr;
    QAction *m_revealAction = nullptr;
    QAction *m_openWithAction = nullptr;
    QAction *m_printAction = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_formatCapabilitiesAction = nullptr;
    QAction *m_ocrSupportAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QAction *m_layoutLockAction = nullptr;
    QAction *m_interfaceLayoutAction = nullptr;
    QAction *m_floatFilmstripAction = nullptr;
    QAction *m_floatMetadataAction = nullptr;
    QAction *m_floatColorPickerAction = nullptr;
    QAction *m_menuBarAction = nullptr;
    QAction *m_statusBarAction = nullptr;
    QAction *m_wallpaperAction = nullptr;
    QAction *m_frameFirstAction = nullptr;
    QAction *m_framePreviousAction = nullptr;
    QAction *m_framePlayAction = nullptr;
    QAction *m_frameNextAction = nullptr;
    QAction *m_frameLastAction = nullptr;
    QAction *m_exportFrameAction = nullptr;
    DirectoryMonitor *m_directoryMonitor = nullptr;
    DirectoryScanService *m_directoryScanService = nullptr;
    BrowserFileOperationsController *
        m_browserFileOperationsController = nullptr;
    quint64 m_directoryScanRequestId = 0;
    QString m_directoryScanPath;
    QString m_directoryScanPreferredImage;
    bool m_directoryScanOpensImage = false;
    ApplicationSettings m_settings;
    QString m_ocrLanguages;
    ActionRegistry *m_actionRegistry = nullptr;
    QString m_systemStyleName;
    QString m_systemColorSchemePath;
    QPalette m_systemPalette;
    bool m_systemColorSchemePathWasSet = false;
    bool m_applyingTheme = false;
    bool m_layoutLocked = true;
    bool m_ocrRecognitionPending = false;
};
