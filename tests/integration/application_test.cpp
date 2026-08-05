#include "applicationsettings.h"
#include "aboutdialog.h"
#include "actionregistry.h"
#include "comparewidget.h"
#include "browserwidget.h"
#include "browserfileoperationscontroller.h"
#include "breezetheme.h"
#include "canvasgesturecontroller.h"
#include "canvasappearancecontroller.h"
#include "colorpickercontroller.h"
#include "colorpickerpanel.h"
#include "desktopintegration.h"
#include "directorymonitor.h"
#include "directoryscanservice.h"
#include "displaycolor.h"
#include "displaycolorcontroller.h"
#include "documentworkflowcontroller.h"
#include "editdialogs.h"
#include "formatcapabilities.h"
#include "formatcapabilitiesdialog.h"
#include "filmstripcontroller.h"
#include "filmstripview.h"
#include "fileoperations.h"
#include "foldernavigationcontroller.h"
#include "imagedocument.h"
#include "imagesessioncontroller.h"
#include "imagesequence.h"
#include "framecontroller.h"
#include "imagecanvas.h"
#include "imagedecoder.h"
#include "imagesource.h"
#include "imageeditcontroller.h"
#include "imageexportservice.h"
#include "imageloadcontroller.h"
#include "mainwindow.h"
#include "metadatapanel.h"
#include "ocrengine.h"
#include "ocrfallbackdetector.h"
#include "ocrsupport.h"
#include "ocrsupportdialog.h"
#include "ocrtextselectionmodel.h"
#include "panellayoutcontroller.h"
#include "paneltitlebar.h"
#include "persistentthumbnailcache.h"
#include "singleinstance.h"
#include "slideshowcontroller.h"
#include "systemappearancecontroller.h"
#include "thumbnailmodel.h"
#include "tiledimageviewmodel.h"
#include "viewernavigationcontroller.h"
#include "vieweruistatecontroller.h"
#include "windowdragcontroller.h"
#include "windowmodecontroller.h"

#include <QDir>
#include <QDirIterator>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QClipboard>
#include <QColorSpace>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHeaderView>
#include <QImage>
#include <QImageWriter>
#include <QKeySequenceEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QElapsedTimer>
#include <QListView>
#include <QListWidget>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QPointingDevice>
#include <QPushButton>
#include <QPrinter>
#include <QScrollBar>
#include <QSet>
#include <QSettings>
#include <QScopeGuard>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <QTranslator>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtTest>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>

class CoreTest : public QObject
{
    Q_OBJECT

private slots:
    void actionRegistryNormalizesLayouts();
    void applicationSettingsValidateAndRoundTrip();
    void folderNavigationTracksHistoryAndFavorites();
    void imageSessionSeparatesOpenedAndDirectoryPreview();
    void filmstripControllerSwitchesModelsAndSynchronizesSelection();
    void imageLoaderKeepsLatestRequestAndReusesCache();
    void imageDecoderReportsMetricsAndCancellation();
    void largeImageDecoderUsesBoundedRegionSource();
    void tiledImageRequestsCenterFirstAndDropsStaleQueue();
    void displayColorPipelineConvertsWithoutChangingSource();
    void displayColorControllerKeepsLatestImage();
    void canvasAppearanceControllerScopesAndPersistsCheckerboardState();
    void systemAppearanceResolvesPortalColorScheme();
    void breezeThemesMatchKdeReferenceColors();
    void slideshowControllerOwnsPlaybackState();
    void colorPickerControllerOwnsSampleAndCopyState();
    void fileOperationsReturnStructuredResults();
    void browserFileOperationsAggregateBatchResults();
    void imageExportAndDesktopIntegrationReturnStructuredResults();
    void documentWorkflowOwnsLoadSaveReloadAndWatchState();
    void viewerNavigationOwnsPendingActivationAndPrefetch();
    void viewerUiStateOwnsActionsAndPresentation();
    void sequenceUsesNaturalSorting();
    void documentTransformsAndUndoes();
    void imageEditControllerValidatesAndReportsCommands();
    void documentSaves();
    void explicitSequencePreservesSelectionOrder();
    void explicitSequenceAppendsAndReplacesFiles();
    void documentCanBeCleared();
    void animatedImageExposesFrames();
    void thumbnailModelListsFoldersAndImagesAsynchronously();
    void thumbnailModelSortsAndUpdatesIncrementally();
    void directoryScanServiceCachesAndRefreshes();
    void directoryFilmstripRefreshesIncrementally();
    void folderOpenUsesViewerAndOverviewAvoidsDuplicateStrip();
    void largeDirectoryOpenReturnsBeforeScanCompletes();
    void persistentThumbnailCacheIsOptInAndInvalidates();
    void formatCapabilitiesReflectRuntimeAndExplainFailures();
    void formatCapabilitiesDialogExposesRuntimeReport();
    void ocrSupportExplainsDistributionPackages();
    void aboutDialogExposesVersionAndProjectInformation();
    void singleInstanceForwardsExactlyOnce();
    void documentCropResizeAndAdjustAreUndoable();
    void cropSelectionCanBeMovedAndResized();
    void colorPickerOffersCopyableFormats();
    void zoomLockFreezesCurrentPercentage();
    void folderSlideshowActivatesItsThumbnail();
    void compareAcceptsFewerThanFourImages();
    void redEyeCorrectionIsSelectiveAndUndoable();
    void metadataPanelShowsPresentEmbeddedMetadata();
    void browserRefreshesItsViewportPalette();
    void browserRestoresPerDirectoryState();
    void mainToolbarCompactsWithoutOverflow();
    void windowChromePopupAndDragRegionsAreUsable();
    void toolbarSettingsCanReorderToggleAndReset();
    void settingsDialogAppliesWithoutClosing();
    void preferencesApplyRecolorsMenuBar();
    void thumbnailStripSettingsControlCachesAndLayout();
    void shortcutSettingsCanEditAndReset();
    void mouseActionsEmitConfiguredCommands();
    void ocrTextSelectionPreservesReadingOrder();
    void ocrFallbackDetectorBudgetsSuspiciousTextRows();
    void ocrEngineRecognizesRenderedText();
    void ocrEngineRecognizesInstalledChineseModel();
    void ocrCanvasSupportsMouseSelection();
    void mainWindowOcrWorkflowCopiesRecognizedText();
    void canvasGesturesZoomAndPanIncrementally();
    void viewerStatusExposesAccessibleNames();
    void windowModesCanCombineAndFitImage();
    void fullscreenComponentsHideAndRestore();
    void windowModeControllerOwnsPresentationState();
    void interfaceLayoutPageCentralizesPanelChanges();
    void interfaceLayoutPreviewSupportsDragRearrangement();
    void panelLayoutControllerOwnsFloatingState();
    void colorPickerMigratesFromSideDockToCompactOverlay();
    void panelLayoutControllerAnchorsOverlayToMainWindow();
    void panelLayoutControllerSnapsOverlayPanelsTogether();
    void floatingPanelsPersistAndSurviveLayoutLock();
    void largeFolderFilmstripSwitchDoesNotRebuildThumbnails();
};

void CoreTest::actionRegistryNormalizesLayouts()
{
    ActionRegistry registry;
    QAction openAction(QStringLiteral("Open"));
    QAction zoomAction(QStringLiteral("Zoom"));
    registry.addAction(QStringLiteral("open"), &openAction,
                       QKeySequence::Open);
    registry.addAction(QStringLiteral("zoom"), &zoomAction,
                       QKeySequence(QStringLiteral("+")));
    registry.addAction(QStringLiteral("open"), &zoomAction,
                       QKeySequence(QStringLiteral("F1")));
    registry.addToolbarItem(QStringLiteral("open"), true);
    registry.addToolbarItem(QStringLiteral("separator"), true,
                            QStringLiteral("Separator"));
    registry.addToolbarItem(QStringLiteral("zoom"), false);

    QCOMPARE(registry.action(QStringLiteral("open")), &openAction);
    QCOMPARE(registry.defaultToolbarLayout(),
             QStringList({QStringLiteral("open"),
                          QStringLiteral("separator"),
                          QStringLiteral("!zoom")}));
    QCOMPARE(registry.normalizedToolbarLayout({
                 QStringLiteral("!zoom"),
                 QStringLiteral("unknown"),
                 QStringLiteral("open"),
                 QStringLiteral("open")}),
             QStringList({QStringLiteral("!zoom"),
                          QStringLiteral("open"),
                          QStringLiteral("!separator")}));

    const QString customZoom = ActionRegistry::shortcutEntry(
        QStringLiteral("zoom"),
        QKeySequence(QStringLiteral("Ctrl+9")));
    const QStringList shortcuts =
        registry.normalizedShortcutLayout({
            customZoom,
            ActionRegistry::shortcutEntry(
                QStringLiteral("unknown"),
                QKeySequence(QStringLiteral("F2"))),
            customZoom});
    QCOMPARE(shortcuts.size(), 2);
    QCOMPARE(ActionRegistry::shortcutEntryId(shortcuts.at(0)),
             QStringLiteral("zoom"));
    QCOMPARE(ActionRegistry::shortcutEntrySequence(shortcuts.at(0)),
             QKeySequence(QStringLiteral("Ctrl+9")));
    QCOMPARE(ActionRegistry::shortcutEntryId(shortcuts.at(1)),
             QStringLiteral("open"));
    QCOMPARE(ActionRegistry::shortcutEntrySequence(shortcuts.at(1)),
             QKeySequence::Open);

    const auto toolbarDefinitions =
        registry.toolbarItemDefinitions();
    QCOMPARE(toolbarDefinitions.at(0).label,
             QStringLiteral("Open"));
    QCOMPARE(toolbarDefinitions.at(1).label,
             QStringLiteral("Separator"));
    QCOMPARE(registry.shortcutItemDefinitions().size(), 2);
}

void CoreTest::applicationSettingsValidateAndRoundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path =
        directory.filePath(QStringLiteral("settings.ini"));

    {
        QSettings storage(path, QSettings::IniFormat);
        storage.setValue(QStringLiteral("appearance/theme"),
                         QStringLiteral("invalid-theme"));
        storage.setValue(QStringLiteral("ui/language"),
                         QStringLiteral("invalid-language"));
        storage.setValue(QStringLiteral("input/mouseWheelAction"),
                         QStringLiteral("invalid-action"));
        storage.setValue(QStringLiteral("input/ctrlMouseWheelAction"),
                         QStringLiteral("navigate"));
        storage.setValue(QStringLiteral("input/backButtonAction"),
                         QStringLiteral("fullscreen"));
        storage.setValue(
            QStringLiteral("thumbnails/persistentCacheMiB"), 1);
        storage.setValue(
            QStringLiteral("performance/imageMemoryCacheMiB"),
            99'999);
        storage.setValue(QStringLiteral("slideshow/intervalMs"),
                         500'000);
        storage.setValue(QStringLiteral("filmstrip/thumbnailExtent"),
                         10);
        storage.setValue(QStringLiteral("filmstrip/verticalColumns"),
                         99);
        storage.setValue(QStringLiteral("filmstrip/directorySortKey"),
                         QStringLiteral("invalid"));
        storage.setValue(QStringLiteral("filmstrip/floatingLayout"),
                         QStringLiteral("invalid"));
        storage.setValue(QStringLiteral("view/showMenuBar"), true);
        storage.setValue(
            QStringLiteral("appearance/showTransparencyCheckerboard"),
            false);
        storage.setValue(QStringLiteral("ui/toolbarLayout"),
                         QStringList{QStringLiteral("open"),
                                     QStringLiteral("separator")});
        storage.setValue(QStringLiteral("browser/recentFolders"),
                         QStringList{directory.path(), directory.path()});
        storage.setValue(QStringLiteral("browser/favoriteFolders"),
                         QStringList{directory.path()});
        storage.setValue(
            QStringLiteral("layout/panelOrder"),
            QStringList{QStringLiteral("colorPicker"),
                        QStringLiteral("invalid"),
                        QStringLiteral("colorPicker")});
        storage.sync();
    }

    ApplicationSettings loaded;
    {
        QSettings storage(path, QSettings::IniFormat);
        loaded = ApplicationSettings::load(storage);
    }
    QCOMPARE(loaded.theme, QStringLiteral("system"));
    QCOMPARE(loaded.language, QStringLiteral("system"));
    QCOMPARE(loaded.wheelAction, QStringLiteral("scroll"));
    QCOMPARE(loaded.ctrlWheelAction, QStringLiteral("navigate"));
    QCOMPARE(loaded.backButtonAction, QStringLiteral("fullscreen"));
    QCOMPARE(loaded.persistentThumbnailCacheMiB,
             ApplicationSettings::minimumThumbnailCacheMiB);
    QCOMPARE(loaded.imageMemoryCacheMiB,
             ApplicationSettings::maximumImageMemoryCacheMiB);
    QCOMPARE(loaded.slideshowIntervalMs,
             ApplicationSettings::maximumSlideshowIntervalMs);
    QCOMPARE(loaded.filmstripThumbnailExtent,
             ApplicationSettings::minimumFilmstripThumbnailExtent);
    QCOMPARE(loaded.filmstripVerticalColumns,
             ApplicationSettings::maximumFilmstripVerticalColumns);
    QCOMPARE(loaded.directoryThumbnailSortKey,
             QStringLiteral("name"));
    QCOMPARE(loaded.floatingThumbnailLayout,
             QStringLiteral("auto"));
    QVERIFY(loaded.showMenuBar);
    QVERIFY(!loaded.showTransparencyCheckerboard);
    QCOMPARE(loaded.toolbarLayout,
             QStringList({QStringLiteral("open"),
                          QStringLiteral("separator")}));
    QCOMPARE(loaded.recentFolders,
             QStringList({directory.path()}));
    QCOMPARE(loaded.favoriteFolders,
             QStringList({directory.path()}));
    QCOMPARE(loaded.panelOrder,
             QStringList({QStringLiteral("colorPicker"),
                          QStringLiteral("thumbnails"),
                          QStringLiteral("information")}));

    loaded.theme = QStringLiteral("dark");
    loaded.language = QStringLiteral("zh_CN");
    loaded.persistentThumbnailCacheEnabled = true;
    loaded.persistentThumbnailCacheMiB = 2048;
    loaded.imageMemoryCacheMiB = 384;
    loaded.slideshowIntervalMs = 5000;
    loaded.showToolbar = false;
    loaded.showFilmstrip = false;
    loaded.showInformation = true;
    loaded.showColorPicker = true;
    loaded.showFilmstripFileNames = false;
    loaded.filmstripThumbnailExtent = 144;
    loaded.filmstripVerticalColumns = 3;
    loaded.directoryThumbnailSortKey = QStringLiteral("modified");
    loaded.floatingThumbnailLayout = QStringLiteral("vertical");
    loaded.directoryThumbnailSortAscending = false;
    loaded.showTransparencyCheckerboard = true;
    loaded.windowGeometry = QByteArray("geometry-test");
    loaded.windowState = QByteArray("state-test");
    loaded.recentFolders = {
        directory.filePath(QStringLiteral("recent"))};
    loaded.favoriteFolders = {
        directory.filePath(QStringLiteral("favorite"))};
    loaded.panelOrder = {
        QStringLiteral("information"),
        QStringLiteral("colorPicker"),
        QStringLiteral("thumbnails")};
    {
        QSettings storage(path, QSettings::IniFormat);
        loaded.save(storage);
        storage.sync();
    }

    QSettings storage(path, QSettings::IniFormat);
    const ApplicationSettings restored =
        ApplicationSettings::load(storage);
    QCOMPARE(restored.theme, QStringLiteral("dark"));
    QCOMPARE(restored.language, QStringLiteral("zh_CN"));
    QVERIFY(restored.persistentThumbnailCacheEnabled);
    QCOMPARE(restored.persistentThumbnailCacheMiB, 2048);
    QCOMPARE(restored.imageMemoryCacheMiB, 384);
    QCOMPARE(restored.slideshowIntervalMs, 5000);
    QVERIFY(!restored.showToolbar);
    QVERIFY(!restored.showFilmstrip);
    QVERIFY(restored.showInformation);
    QVERIFY(restored.showColorPicker);
    QVERIFY(!restored.showFilmstripFileNames);
    QCOMPARE(restored.filmstripThumbnailExtent, 144);
    QCOMPARE(restored.filmstripVerticalColumns, 3);
    QCOMPARE(restored.directoryThumbnailSortKey,
             QStringLiteral("modified"));
    QCOMPARE(restored.floatingThumbnailLayout,
             QStringLiteral("vertical"));
    QVERIFY(!restored.directoryThumbnailSortAscending);
    QVERIFY(restored.showTransparencyCheckerboard);
    QCOMPARE(restored.windowGeometry, QByteArray("geometry-test"));
    QCOMPARE(restored.windowState, QByteArray("state-test"));
    QCOMPARE(restored.recentFolders, loaded.recentFolders);
    QCOMPARE(restored.favoriteFolders, loaded.favoriteFolders);
    QCOMPARE(restored.panelOrder, loaded.panelOrder);
}

void CoreTest::folderNavigationTracksHistoryAndFavorites()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString first = root.filePath(QStringLiteral("first"));
    const QString second = root.filePath(QStringLiteral("second"));
    const QString third = root.filePath(QStringLiteral("third"));
    QVERIFY(QDir().mkpath(first));
    QVERIFY(QDir().mkpath(second));
    QVERIFY(QDir().mkpath(third));

    FolderNavigationController navigation;
    navigation.setStoredLocations(
        {first, first, second}, {third, third});
    QCOMPARE(navigation.recentDirectories(),
             QStringList({first, second}));
    QCOMPARE(navigation.favoriteDirectories(),
             QStringList({third}));

    navigation.recordVisit(first);
    navigation.recordVisit(second);
    navigation.recordVisit(third);
    QCOMPARE(navigation.currentDirectory(), third);
    QVERIFY(navigation.canGoBack());
    QVERIFY(!navigation.canGoForward());
    QCOMPARE(navigation.recentDirectories(),
             QStringList({third, second, first}));

    QSignalSpy requested(
        &navigation,
        &FolderNavigationController::directoryRequested);
    navigation.goBack();
    QCOMPARE(navigation.currentDirectory(), third);
    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.constFirst().constFirst().toString(), second);
    QVERIFY(!navigation.canGoBack());
    navigation.recordVisit(second);
    QCOMPARE(navigation.currentDirectory(), second);
    QVERIFY(navigation.canGoForward());
    navigation.goBack();
    navigation.recordVisit(first);
    QCOMPARE(navigation.currentDirectory(), first);
    navigation.goForward();
    navigation.recordVisit(second);
    QCOMPARE(navigation.currentDirectory(), second);

    navigation.goForward();
    const QString failedTarget =
        requested.constLast().constFirst().toString();
    navigation.navigationFailed(failedTarget);
    QCOMPARE(navigation.currentDirectory(), second);
    QVERIFY(navigation.canGoForward());

    navigation.toggleFavorite(second);
    QCOMPARE(navigation.favoriteDirectories(),
             QStringList({third, second}));
    navigation.toggleFavorite(third);
    QCOMPARE(navigation.favoriteDirectories(),
             QStringList({second}));
    QVERIFY(navigation.isFavorite(second));
    navigation.clearRecentDirectories();
    QVERIFY(navigation.recentDirectories().isEmpty());
}

void CoreTest::imageSessionSeparatesOpenedAndDirectoryPreview()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QStringList paths;
    for (const QString &name : {
             QStringLiteral("image-1.png"),
             QStringLiteral("image-2.png"),
             QStringLiteral("image-3.png")}) {
        const QString path = directory.filePath(name);
        QImage image(8, 6, QImage::Format_RGB32);
        image.fill(Qt::cyan);
        QVERIFY(image.save(path));
        paths.append(path);
    }

    ImageSessionController session;
    session.appendOpenedFiles({paths.at(0), paths.at(2)});
    QCOMPARE(session.openedFiles(),
             QStringList({paths.at(0), paths.at(2)}));
    session.setLoadedPath(paths.at(0));
    QCOMPARE(session.currentOpenedIndex(), 0);
    session.setPendingOpenedIndex(1);
    QCOMPARE(session.effectiveOpenedIndex(), 1);
    session.clearPendingOpenedIndex();
    QCOMPARE(session.effectiveOpenedIndex(), 0);

    QVERIFY(session.refreshDirectoryForFile(paths.at(1)));
    QVERIFY(!session.refreshDirectoryForFile(paths.at(1)));
    QCOMPARE(session.directoryCount(), 3);
    QCOMPARE(session.directoryIndexOf(paths.at(1)), 1);

    session.setLoadedPath(paths.at(1));
    QCOMPARE(session.currentOpenedIndex(), -1);
    QCOMPARE(session.openedCount(), 2);
    session.appendOpenedFiles({paths.at(1)});
    session.setLoadedPath(paths.at(1));
    QCOMPARE(session.currentOpenedIndex(), 2);
    QCOMPARE(session.openedCount(), 3);

    const auto removedOther =
        session.removeOpenedAt(0, paths.at(1));
    QVERIFY(removedOther.removed);
    QVERIFY(!removedOther.removedDisplayedImage);
    QCOMPARE(session.currentOpenedIndex(), 1);
    QCOMPARE(session.openedCount(), 2);

    const auto removedCurrent =
        session.removeOpenedAt(1, paths.at(1));
    QVERIFY(removedCurrent.removed);
    QVERIFY(removedCurrent.removedDisplayedImage);
    QVERIFY(!removedCurrent.openedImagesEmpty);
    QCOMPARE(removedCurrent.nextIndex, 0);
    QCOMPARE(removedCurrent.nextPath, paths.at(2));
    QCOMPARE(session.currentOpenedIndex(), -1);

    session.invalidateDirectory();
    QVERIFY(session.directoryPath().isEmpty());
    QCOMPARE(session.directoryCount(), 0);
}

void CoreTest::filmstripControllerSwitchesModelsAndSynchronizesSelection()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QStringList paths;
    for (const QString &name : {
             QStringLiteral("opened-a.png"),
             QStringLiteral("opened-b.png"),
             QStringLiteral("directory-c.png")}) {
        const QString path = directory.filePath(name);
        QImage image(10, 8, QImage::Format_RGB32);
        image.fill(Qt::magenta);
        QVERIFY(image.save(path));
        paths.append(path);
    }

    QListView view;
    ThumbnailModel openedModel(&view);
    ThumbnailModel directoryModel(&view);
    openedModel.setFiles({paths.at(0), paths.at(1)});
    directoryModel.setFiles({paths.at(1), paths.at(2)});
    FilmstripController controller(
        &view, &openedModel, &directoryModel);
    QSignalSpy activationSpy(
        &controller, &FilmstripController::activationRequested);

    QCOMPARE(controller.source(),
             FilmstripController::Source::OpenedImages);
    QCOMPARE(controller.model(), &openedModel);
    QCOMPARE(controller.count(), 2);
    controller.syncSelection(paths.at(1));
    QCOMPARE(controller.currentRow(), 1);
    QCOMPARE(activationSpy.count(), 0);

    controller.selectRow(0);
    QCOMPARE(controller.currentRow(), 0);
    QCOMPARE(activationSpy.count(), 1);
    QCOMPARE(controller.pathAt(0), paths.at(0));

    controller.setSource(
        FilmstripController::Source::CurrentDirectory);
    QCOMPARE(controller.model(), &directoryModel);
    QCOMPARE(view.model(), &directoryModel);
    QCOMPARE(controller.count(), 2);
    controller.syncSelection(paths.at(2));
    QCOMPARE(controller.currentRow(), 1);
    QCOMPARE(controller.rowForPath(paths.at(1)), 0);

    controller.syncSelection(
        directory.filePath(QStringLiteral("missing.png")));
    QCOMPARE(controller.currentRow(), -1);
}

void CoreTest::imageLoaderKeepsLatestRequestAndReusesCache()
{
    class TestRegionSource final : public ImageSource
    {
    public:
        explicit TestRegionSource(QString path,
                                  std::atomic_int *releaseCount)
            : m_path(std::move(path))
            , m_releaseCount(releaseCount)
        {
            m_preview = QImage(8, 6, QImage::Format_RGB32);
            m_preview.fill(Qt::cyan);
        }

        QSize logicalSize() const override { return {8000, 6000}; }
        QImage preview() const override { return m_preview; }
        QString filePath() const override { return m_path; }
        bool isRegionBacked() const override { return true; }
        void releaseTransientResources() override
        {
            ++*m_releaseCount;
        }
        QImage readRegion(const QRect &, const QSize &outputSize,
                          std::stop_token,
                          QString *) override
        {
            QImage result(outputSize, QImage::Format_RGB32);
            result.fill(Qt::cyan);
            return result;
        }

    private:
        QString m_path;
        QImage m_preview;
        std::atomic_int *m_releaseCount;
    };

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString slowPath =
        directory.filePath(QStringLiteral("slow.png"));
    const QString fastPath =
        directory.filePath(QStringLiteral("fast.png"));
    const QString prefetchedPath =
        directory.filePath(QStringLiteral("prefetched.png"));
    QImage source(12, 9, QImage::Format_RGB32);
    source.fill(Qt::green);
    QVERIFY(source.save(slowPath));
    QVERIFY(source.save(fastPath));
    QVERIFY(source.save(prefetchedPath));

    std::atomic_int decodeCount{0};
    std::atomic_bool slowStarted{false};
    const auto decoder =
        [&decodeCount, &slowStarted](const QString &path) {
            ++decodeCount;
            if (QFileInfo(path).fileName()
                    == QStringLiteral("slow.png")) {
                slowStarted = true;
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(80));
            }
            QImage decoded(12, 9, QImage::Format_RGB32);
            decoded.fill(Qt::blue);
            return ImageLoadResult{
                QFileInfo(path).absoluteFilePath(), decoded, {},
                ImageDecodeError::None, {}, {}};
        };

    ImageLoadController loader(nullptr, decoder);
    QCOMPARE(loader.cacheLimitMiB(), 256);
    loader.setCacheLimitMiB(1);
    QCOMPARE(loader.cacheLimitMiB(), 16);
    loader.setCacheLimitMiB(512);
    QCOMPARE(loader.cacheLimitMiB(), 512);
    QSignalSpy finished(
        &loader, &ImageLoadController::loadFinished);
    loader.request(slowPath, 1);
    QTRY_VERIFY_WITH_TIMEOUT(slowStarted.load(), 1000);
    loader.request(fastPath, 2);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2000);
    QCOMPARE(finished.at(0).at(1).toInt(), 2);
    QVERIFY(!finished.at(0).at(2).toBool());
    const ImageLoadResult latest =
        qvariant_cast<ImageLoadResult>(finished.at(0).at(0));
    QCOMPARE(latest.filePath,
             QFileInfo(fastPath).absoluteFilePath());
    QCOMPARE(decodeCount.load(), 2);

    loader.request(fastPath, 3);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 1000);
    QCOMPARE(finished.at(1).at(1).toInt(), 3);
    QVERIFY(finished.at(1).at(2).toBool());
    QCOMPARE(decodeCount.load(), 2);

    loader.prefetch({prefetchedPath});
    QTRY_VERIFY_WITH_TIMEOUT(
        loader.isCached(prefetchedPath), 1000);
    const int afterPrefetch = decodeCount.load();
    loader.request(prefetchedPath, 4);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 3, 1000);
    QVERIFY(finished.at(2).at(2).toBool());
    QCOMPARE(decodeCount.load(), afterPrefetch);

    const QString regionPath =
        directory.filePath(QStringLiteral("region.png"));
    QVERIFY(source.save(regionPath));
    std::atomic_int regionDecodeCount{0};
    std::atomic_int releaseCount{0};
    const auto regionSource = std::make_shared<TestRegionSource>(
        regionPath, &releaseCount);
    ImageLoadController regionLoader(
        nullptr, [&regionDecodeCount, regionSource](const QString &path) {
            ++regionDecodeCount;
            ImageLoadResult result;
            result.filePath = QFileInfo(path).absoluteFilePath();
            result.image = regionSource->preview();
            result.source = regionSource;
            return result;
        });
    QSignalSpy regionFinished(
        &regionLoader, &ImageLoadController::loadFinished);
    regionLoader.request(regionPath, 6);
    QTRY_COMPARE_WITH_TIMEOUT(regionFinished.count(), 1, 1000);
    regionLoader.request(regionPath, 7);
    QTRY_COMPARE_WITH_TIMEOUT(regionFinished.count(), 2, 1000);
    QCOMPARE(regionDecodeCount.load(), 1);
    QVERIFY(regionFinished.at(1).at(2).toBool());
    const ImageLoadResult cachedRegion =
        qvariant_cast<ImageLoadResult>(regionFinished.at(1).at(0));
    QCOMPARE(cachedRegion.source, regionSource);
    QCOMPARE(cachedRegion.source->logicalSize(), QSize(8000, 6000));

    ImageDocument regionDocument;
    QVERIFY(regionDocument.loadDecoded(cachedRegion));
    QImage replacement(2, 2, QImage::Format_RGB32);
    replacement.fill(Qt::black);
    QVERIFY(regionDocument.loadImage(replacement));
    QCOMPARE(releaseCount.load(), 1);

    std::atomic_bool cancelStarted{false};
    ImageLoadController cancelLoader(
        nullptr, [&cancelStarted](const QString &path) {
            cancelStarted = true;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(80));
            QImage decoded(4, 4, QImage::Format_RGB32);
            decoded.fill(Qt::red);
            return ImageLoadResult{
                path, decoded, {}, ImageDecodeError::None, {}, {}};
        });
    QSignalSpy canceledFinished(
        &cancelLoader, &ImageLoadController::loadFinished);
    cancelLoader.request(slowPath, 5);
    QTRY_VERIFY_WITH_TIMEOUT(cancelStarted.load(), 1000);
    cancelLoader.cancel();
    QTest::qWait(120);
    QCOMPARE(canceledFinished.count(), 0);
    QVERIFY(!cancelLoader.isLoading());
}

void CoreTest::imageDecoderReportsMetricsAndCancellation()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path =
        directory.filePath(QStringLiteral("profiled.png"));
    QImage source(64, 48, QImage::Format_RGBA8888);
    source.fill(QColor(80, 140, 210, 230));
    source.setColorSpace(QColorSpace(QColorSpace::DisplayP3));
    QVERIFY(source.save(path));

    const ImageLoadResult decoded = ImageDecoder::decode(path);
    QVERIFY(decoded.succeeded());
    QCOMPARE(decoded.errorCode, ImageDecodeError::None);
    QCOMPARE(decoded.metrics.sourceSize, source.size());
    QCOMPARE(decoded.metrics.decodedSize, source.size());
    QCOMPARE(decoded.metrics.format, QByteArray("png"));
    QVERIFY(decoded.metrics.decodedBytes > 0);
    QVERIFY(decoded.metrics.elapsedNanoseconds > 0);
    QVERIFY(decoded.metrics.sourceColorSpacePresent);
    QVERIFY(decoded.metrics.convertedToSrgb);
    QCOMPARE(decoded.image.colorSpace(),
             QColorSpace(QColorSpace::SRgb));

    ImageDecodeOptions preserveProfile;
    preserveProfile.normalizeToSrgb = false;
    const ImageLoadResult preserved =
        ImageDecoder::decode(path, preserveProfile);
    QVERIFY(preserved.succeeded());
    QVERIFY(!preserved.metrics.convertedToSrgb);
    QCOMPARE(preserved.image.colorSpace(),
             QColorSpace(QColorSpace::DisplayP3));

    ImageDecodeOptions restricted;
    restricted.maximumPixels = 100;
    const ImageLoadResult tooLarge =
        ImageDecoder::decode(path, restricted);
    QVERIFY(!tooLarge.succeeded());
    QCOMPARE(tooLarge.errorCode, ImageDecodeError::TooLarge);
    QCOMPARE(tooLarge.metrics.sourceSize, source.size());

    std::stop_source alreadyCanceled;
    alreadyCanceled.request_stop();
    const ImageLoadResult canceled = ImageDecoder::decode(
        path, {}, alreadyCanceled.get_token());
    QVERIFY(!canceled.succeeded());
    QCOMPARE(canceled.errorCode, ImageDecodeError::Cancelled);

    const int configuredLimit = QImageReader::allocationLimit();
    const auto restoreAllocationLimit = qScopeGuard(
        [configuredLimit] {
        QImageReader::setAllocationLimit(configuredLimit);
    });
    const QString allocationPath = directory.filePath(
        QStringLiteral("allocation-limit.png"));
    QImage allocationImage(1024, 1024, QImage::Format_RGB32);
    allocationImage.fill(Qt::darkCyan);
    QVERIFY(allocationImage.save(allocationPath));
    QImageReader::setAllocationLimit(1);
    const ImageLoadResult allocationDecoded =
        ImageDecoder::decode(allocationPath);
    QVERIFY(allocationDecoded.succeeded());
    QVERIFY(QImageReader::allocationLimit() >= 512);

    std::atomic_bool decoderStarted{false};
    std::atomic_bool cancellationObserved{false};
    ImageLoadController loader(
        nullptr,
        ImageLoadController::Decoder(
            [&decoderStarted, &cancellationObserved](
                const QString &filePath, std::stop_token stopToken) {
                decoderStarted = true;
                while (!stopToken.stop_requested()) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1));
                }
                cancellationObserved = true;
                ImageLoadResult result;
                result.filePath = filePath;
                result.errorCode = ImageDecodeError::Cancelled;
                return result;
            }));
    loader.request(path, 1);
    QTRY_VERIFY_WITH_TIMEOUT(decoderStarted.load(), 1000);
    loader.cancel();
    QTRY_VERIFY_WITH_TIMEOUT(cancellationObserved.load(), 1000);
}

void CoreTest::largeImageDecoderUsesBoundedRegionSource()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(
        QStringLiteral("large-region-source.png"));

    QImage source(768, 512, QImage::Format_RGBA8888);
    source.fill(Qt::transparent);
    {
        QPainter painter(&source);
        painter.fillRect(QRect(0, 0, 384, 256), Qt::red);
        painter.fillRect(QRect(384, 0, 384, 256), Qt::green);
        painter.fillRect(QRect(0, 256, 384, 256), Qt::blue);
        painter.fillRect(QRect(384, 256, 384, 256), Qt::yellow);
    }
    QVERIFY(source.save(path));

    ImageDecodeOptions options;
    options.regionBackedThresholdBytes = 1;
    options.largeImagePreviewMaximumDimension = 512;
    const ImageLoadResult decoded = ImageDecoder::decode(path, options);

    QVERIFY2(decoded.succeeded(), qPrintable(decoded.error));
    QVERIFY(decoded.source);
    QVERIFY(decoded.source->isRegionBacked());
    QCOMPARE(decoded.metrics.sourceSize, source.size());
    QCOMPARE(decoded.source->logicalSize(), source.size());
    QVERIFY(decoded.image.width() <= 512);
    QVERIFY(decoded.image.height() <= 512);
    QVERIFY(decoded.image.size() != source.size());

    QString regionError;
    const QImage region = decoded.source->readRegion(
        QRect(430, 300, 100, 100), QSize(50, 50), {}, &regionError);
    QVERIFY2(!region.isNull(), qPrintable(regionError));
    QCOMPARE(region.size(), QSize(50, 50));
    const QColor sample = region.pixelColor(25, 25);
    QVERIFY(sample.red() > 245);
    QVERIFY(sample.green() > 245);
    QVERIFY(sample.blue() < 10);

    ImageCanvas canvas;
    canvas.resize(400, 300);
    canvas.setColorManagedImage(
        decoded.image, decoded.image, false,
        decoded.source->logicalSize(), decoded.source);
    QCOMPARE(canvas.logicalImageSize(), source.size());
    canvas.fitToWindow();
    QVERIFY(qAbs(canvas.zoom() - (368.0 / 768.0)) < 0.001);

    QSignalSpy exactPickSpy(&canvas, &ImageCanvas::colorPicked);
    canvas.setColorPickerEnabled(true);
    canvas.show();
    QTest::mouseClick(
        &canvas, Qt::LeftButton, Qt::NoModifier,
        QPoint(300, 220));
    QTRY_COMPARE_WITH_TIMEOUT(exactPickSpy.count(), 1, 3000);
    const QColor exactColor =
        exactPickSpy.constFirst().at(0).value<QColor>();
    QVERIFY(exactColor.red() > 245);
    QVERIFY(exactColor.green() > 245);
    QVERIFY(exactColor.blue() < 10);

    ImageDocument document;
    QVERIFY(document.loadDecoded(decoded));
    const QString exportedPath = directory.filePath(
        QStringLiteral("large-region-export.png"));
    const ImageExportService::Result exported =
        document.saveAsResult(exportedPath);
    QVERIFY2(exported.succeeded(), qPrintable(exported.detail));
    QImageReader exportedReader(exportedPath);
    QCOMPARE(exportedReader.size(), source.size());
}

void CoreTest::tiledImageRequestsCenterFirstAndDropsStaleQueue()
{
    class RecordingSource final : public ImageSource
    {
    public:
        RecordingSource()
        {
            m_preview = QImage(64, 64, QImage::Format_RGB32);
            m_preview.fill(Qt::darkGray);
        }

        QSize logicalSize() const override { return {4096, 4096}; }
        QImage preview() const override { return m_preview; }
        QString filePath() const override
        {
            return QStringLiteral("recording-source");
        }
        bool isRegionBacked() const override { return true; }
        QImage readRegion(const QRect &sourceRect,
                          const QSize &outputSize,
                          std::stop_token,
                          QString *) override
        {
            const int requestNumber = ++m_requestCount;
            {
                const std::lock_guard lock(m_mutex);
                m_regions.append(sourceRect);
            }
            if (requestNumber == 1) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(80));
            }
            QImage result(outputSize, QImage::Format_RGB32);
            result.fill(Qt::magenta);
            return result;
        }
        QList<QRect> regions() const
        {
            const std::lock_guard lock(m_mutex);
            return m_regions;
        }

    private:
        QImage m_preview;
        std::atomic_int m_requestCount{0};
        mutable std::mutex m_mutex;
        QList<QRect> m_regions;
    };

    const auto source = std::make_shared<RecordingSource>();
    TiledImageViewModel tiles;
    tiles.setSource(source);
    tiles.updateViewport(QRectF(0, 0, 1536, 1536), 1.0);
    QTRY_VERIFY_WITH_TIMEOUT(source->regions().size() >= 1, 1000);
    QCOMPARE(source->regions().constFirst().topLeft(),
             QPoint(512, 512));

    // Move while the center tile is still running. None of the eight queued
    // tiles from the old viewport should run before the new viewport.
    tiles.updateViewport(
        QRectF(2048, 2048, 1024, 1024), 1.0);
    QTRY_VERIFY_WITH_TIMEOUT(source->regions().size() >= 2, 2000);
    const QRect second = source->regions().at(1);
    QVERIFY(second.left() >= 2048);
    QVERIFY(second.top() >= 2048);
    QTRY_VERIFY_WITH_TIMEOUT(!tiles.visibleTiles().isEmpty(), 2000);
    for (const TiledImageViewModel::Tile &tile : tiles.visibleTiles()) {
        QVERIFY(tile.sourceRect.left() >= 2048);
        QVERIFY(tile.sourceRect.top() >= 2048);
    }
}

void CoreTest::displayColorPipelineConvertsWithoutChangingSource()
{
    QImage source(80, 50, QImage::Format_RGBA8888);
    source.fill(QColor(220, 80, 35, 210));
    source.setColorSpace(QColorSpace(QColorSpace::DisplayP3));

    const DisplayColorTransformResult converted =
        DisplayColor::transform(
            source, QColorSpace(QColorSpace::SRgb));
    QVERIFY(converted.succeeded());
    QVERIFY(converted.converted);
    QVERIFY(!converted.assumedSrgb);
    QCOMPARE(converted.image.colorSpace(),
             QColorSpace(QColorSpace::SRgb));
    QCOMPARE(source.colorSpace(),
             QColorSpace(QColorSpace::DisplayP3));
    QCOMPARE(converted.image.size(), source.size());
    QVERIFY(converted.elapsedNanoseconds > 0);

    QImage untagged(20, 12, QImage::Format_RGB32);
    untagged.fill(Qt::darkCyan);
    const DisplayColorTransformResult assumed =
        DisplayColor::transform(
            untagged, QColorSpace(QColorSpace::SRgb));
    QVERIFY(assumed.succeeded());
    QVERIFY(assumed.assumedSrgb);
    QVERIFY(!assumed.converted);
    QVERIFY(!untagged.colorSpace().isValid());
    QVERIFY(!assumed.image.colorSpace().isValid());
    QCOMPARE(assumed.image.cacheKey(), untagged.cacheKey());

    std::stop_source stopSource;
    stopSource.request_stop();
    const DisplayColorTransformResult canceled =
        DisplayColor::transform(
            source, QColorSpace(QColorSpace::SRgb),
            stopSource.get_token());
    QCOMPARE(canceled.error,
             DisplayColorTransformError::Cancelled);
    QVERIFY(canceled.image.isNull());

    const DisplayColorTarget waylandTarget =
        DisplayColor::resolveAutomaticTarget(
            QStringLiteral("DP-1"), QStringLiteral("wayland"));
    QCOMPARE(waylandTarget.source,
             DisplayColorTargetSource::CompositorSrgb);
    QCOMPARE(waylandTarget.colorSpace,
             QColorSpace(QColorSpace::SRgb));
    QCOMPARE(waylandTarget.outputName,
             QStringLiteral("DP-1"));
}

void CoreTest::displayColorControllerKeepsLatestImage()
{
    QWidget host;
    ImageCanvas canvas(&host);
    host.resize(500, 360);
    canvas.resize(host.size());
    host.show();

    DisplayColorController controller(
        &canvas, &host, nullptr,
        [](const QString &outputName, const QString &) {
            DisplayColorTarget target;
            target.colorSpace = QColorSpace(QColorSpace::SRgb);
            target.source = DisplayColorTargetSource::CompositorSrgb;
            target.outputName = outputName;
            return target;
        });
    QSignalSpy ready(&controller,
                     &DisplayColorController::imageReady);

    QImage first(900, 600, QImage::Format_RGBA8888);
    first.fill(Qt::red);
    first.setColorSpace(QColorSpace(QColorSpace::DisplayP3));
    QImage latest(32, 24, QImage::Format_RGBA8888);
    latest.fill(Qt::blue);
    latest.setColorSpace(QColorSpace(QColorSpace::SRgb));
    controller.setImage(first);
    controller.setImage(latest);

    QTRY_VERIFY_WITH_TIMEOUT(ready.count() >= 1, 2000);
    QCOMPARE(canvas.sourceImage().size(), latest.size());
    QCOMPARE(canvas.sourceImage().pixelColor(0, 0), QColor(Qt::blue));
    QCOMPARE(canvas.displayImage().size(), latest.size());
    QCOMPARE(canvas.displayImage().colorSpace(),
             QColorSpace(QColorSpace::SRgb));
    QVERIFY(!controller.isTransforming());
}

void CoreTest::canvasAppearanceControllerScopesAndPersistsCheckerboardState()
{
    ImageCanvas canvas;
    canvas.resize(120, 90);
    QPalette palette = canvas.palette();
    const QColor background(23, 31, 47);
    palette.setColor(QPalette::Window, background);
    canvas.setPalette(palette);

    QImage transparentImage(40, 30, QImage::Format_ARGB32);
    transparentImage.fill(Qt::transparent);
    canvas.setImage(transparentImage);
    canvas.actualSize();
    canvas.show();
    QCoreApplication::processEvents();

    QAction checkerboardAction;
    checkerboardAction.setCheckable(true);
    checkerboardAction.setChecked(true);
    CanvasAppearanceController controller(
        &canvas, &checkerboardAction);
    QSignalSpy changedSpy(
        &controller,
        &CanvasAppearanceController::
            transparencyCheckerboardVisibleChanged);

    const auto renderCanvas = [&canvas] {
        QImage snapshot(canvas.size(),
                        QImage::Format_ARGB32_Premultiplied);
        snapshot.fill(Qt::transparent);
        canvas.render(&snapshot);
        return snapshot;
    };

    const QImage withCheckerboard = renderCanvas();
    QCOMPARE(withCheckerboard.pixelColor(5, 5), background);
    const QPoint imageCenter = canvas.rect().center();
    QVERIFY(withCheckerboard.pixelColor(imageCenter) != background);

    checkerboardAction.setChecked(false);
    QCOMPARE(changedSpy.count(), 1);
    QVERIFY(!controller.transparencyCheckerboardVisible());
    const QImage withoutCheckerboard = renderCanvas();
    QCOMPARE(withoutCheckerboard.pixelColor(5, 5), background);
    QCOMPARE(withoutCheckerboard.pixelColor(imageCenter), background);

    controller.setTransparencyCheckerboardVisible(true);
    QVERIFY(checkerboardAction.isChecked());
    QVERIFY(controller.transparencyCheckerboardVisible());
    QCOMPARE(changedSpy.count(), 2);
}

void CoreTest::systemAppearanceResolvesPortalColorScheme()
{
    using ColorScheme = SystemAppearanceController::ColorScheme;
    QCOMPARE(
        SystemAppearanceController::colorSchemeFromPortalValue(
            QVariant::fromValue(uint(1))),
        ColorScheme::PreferDark);
    QCOMPARE(
        SystemAppearanceController::colorSchemeFromPortalValue(
            QVariant::fromValue(uint(2))),
        ColorScheme::PreferLight);
    QCOMPARE(
        SystemAppearanceController::colorSchemeFromPortalValue(
            QVariant::fromValue(uint(17))),
        ColorScheme::NoPreference);
    QCOMPARE(
        SystemAppearanceController::resolveTheme(
            QStringLiteral("system"), ColorScheme::PreferDark),
        QStringLiteral("system"));
    QCOMPARE(
        SystemAppearanceController::resolveTheme(
            QStringLiteral("system"), ColorScheme::PreferLight),
        QStringLiteral("system"));
    QCOMPARE(
        SystemAppearanceController::resolveTheme(
            QStringLiteral("system"), ColorScheme::NoPreference),
        QStringLiteral("system"));
    QCOMPARE(
        SystemAppearanceController::resolveTheme(
            QStringLiteral("light"), ColorScheme::PreferDark),
        QStringLiteral("light"));
}

void CoreTest::breezeThemesMatchKdeReferenceColors()
{
    const QPalette light =
        BreezeTheme::palette(BreezeTheme::Variant::Light);
    QCOMPARE(light.color(QPalette::Window), QColor(239, 240, 241));
    QCOMPARE(light.color(QPalette::WindowText), QColor(35, 38, 41));
    QCOMPARE(light.color(QPalette::Base), QColor(255, 255, 255));
    QCOMPARE(light.color(QPalette::AlternateBase), QColor(247, 247, 247));
    QCOMPARE(light.color(QPalette::Button), QColor(252, 252, 252));
    QCOMPARE(light.color(QPalette::Highlight), QColor(61, 174, 233));
    QCOMPARE(light.color(QPalette::HighlightedText), QColor(255, 255, 255));
    QCOMPARE(light.color(QPalette::Link), QColor(41, 128, 185));

    const QPalette dark =
        BreezeTheme::palette(BreezeTheme::Variant::Dark);
    QCOMPARE(dark.color(QPalette::Window), QColor(32, 35, 38));
    QCOMPARE(dark.color(QPalette::WindowText), QColor(252, 252, 252));
    QCOMPARE(dark.color(QPalette::Base), QColor(20, 22, 24));
    QCOMPARE(dark.color(QPalette::AlternateBase), QColor(29, 31, 34));
    QCOMPARE(dark.color(QPalette::Button), QColor(41, 44, 48));
    QCOMPARE(dark.color(QPalette::Highlight), QColor(61, 174, 233));
    QCOMPARE(dark.color(QPalette::HighlightedText), QColor(252, 252, 252));
    QCOMPARE(dark.color(QPalette::Link), QColor(29, 153, 243));

    QCOMPARE(BreezeTheme::preferredStyleName(
                 {QStringLiteral("Fusion"), QStringLiteral("Breeze")}),
             QStringLiteral("Breeze"));
    QCOMPARE(BreezeTheme::preferredStyleName(
                 {QStringLiteral("Windows"), QStringLiteral("Fusion")}),
             QStringLiteral("Fusion"));
    QCOMPARE(BreezeTheme::preferredStyleName(
                 {QStringLiteral("Fusion"), QStringLiteral("Breeze")},
                 false),
             QStringLiteral("Fusion"));
}

void CoreTest::slideshowControllerOwnsPlaybackState()
{
    SlideshowController controller(
        nullptr, [](int) { return 0; });
    QSignalSpy activationSpy(
        &controller,
        &SlideshowController::activateIndexRequested);
    QSignalSpy runningSpy(
        &controller, &SlideshowController::runningChanged);
    QSignalSpy fullscreenSpy(
        &controller, &SlideshowController::fullscreenRequested);

    controller.setIntervalMs(25);
    QCOMPARE(controller.intervalMs(), 25);
    controller.setIntervalMs(60'000);
    controller.setFullscreenEnabled(true);
    controller.setNavigationState(1, 0);
    QVERIFY(!controller.start(false));
    QVERIFY(!controller.isRunning());

    controller.setNavigationState(3, 0);
    QVERIFY(controller.start(false));
    QVERIFY(controller.isRunning());
    QCOMPARE(runningSpy.count(), 1);
    QCOMPARE(fullscreenSpy.count(), 1);
    QCOMPARE(fullscreenSpy.constFirst().constFirst().toBool(),
             true);

    controller.advance();
    controller.advance();
    controller.advance();
    QCOMPARE(activationSpy.count(), 3);
    QCOMPARE(activationSpy.at(0).constFirst().toInt(), 1);
    QCOMPARE(activationSpy.at(1).constFirst().toInt(), 2);
    QCOMPARE(activationSpy.at(2).constFirst().toInt(), 0);

    controller.stop();
    QVERIFY(!controller.isRunning());
    QCOMPARE(runningSpy.count(), 2);
    QCOMPARE(fullscreenSpy.count(), 2);
    QCOMPARE(fullscreenSpy.at(1).constFirst().toBool(), false);

    controller.setFullscreenEnabled(false);
    controller.setRandomOrder(true);
    controller.setNavigationState(3, 0);
    QVERIFY(controller.start(false));
    controller.advance();
    QCOMPARE(activationSpy.constLast().constFirst().toInt(), 1);
    controller.setNavigationState(1, 0);
    QVERIFY(!controller.isRunning());
}

void CoreTest::colorPickerControllerOwnsSampleAndCopyState()
{
    ImageCanvas canvas;
    ColorPickerPanel panel;
    ColorPickerController controller(&canvas, &panel);
    QSignalSpy copySpy(
        &controller,
        &ColorPickerController::copyTextRequested);

    QImage image(20, 20, QImage::Format_ARGB32);
    image.fill(QColor(10, 20, 30, 128));
    canvas.setImage(image);
    controller.setEnabled(true);
    QVERIFY(controller.isEnabled());

    QImage sample(11, 11, QImage::Format_ARGB32);
    sample.fill(QColor(10, 20, 30, 128));
    QVERIFY(QMetaObject::invokeMethod(
        &canvas, "colorHovered", Qt::DirectConnection,
        Q_ARG(QColor, QColor(10, 20, 30, 128)),
        Q_ARG(QPoint, QPoint(4, 7)),
        Q_ARG(QImage, sample)));
    QVERIFY(controller.hasSample());
    QCOMPARE(controller.color(), QColor(10, 20, 30, 128));
    QCOMPARE(controller.imagePosition(), QPoint(4, 7));
    QCOMPARE(controller.formattedColor(QStringLiteral("hex")),
             QStringLiteral("#0A141E"));
    QCOMPARE(controller.formattedColor(QStringLiteral("rgba")),
             QStringLiteral("rgba(10, 20, 30, 0.502)"));
    QVERIFY(controller.formattedColor(QStringLiteral("all"))
                .contains(QStringLiteral("Position: (4, 7)")));

    QVERIFY(QMetaObject::invokeMethod(
        &panel, "copyRequested", Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("rgb"))));
    QCOMPARE(copySpy.count(), 1);
    QCOMPARE(copySpy.constFirst().at(0).toString(),
             QStringLiteral("rgb(10, 20, 30)"));
    QCOMPARE(copySpy.constFirst().at(1).toString(),
             QStringLiteral("rgb"));

    controller.resetForImage();
    QVERIFY(!controller.hasSample());
    QVERIFY(controller.formattedColor(
                QStringLiteral("hex")).isEmpty());
    controller.setEnabled(false);
    QVERIFY(!controller.isEnabled());
}

void CoreTest::fileOperationsReturnStructuredResults()
{
    QTemporaryDir sourceDirectory;
    QTemporaryDir copyDirectory;
    QTemporaryDir moveDirectory;
    QVERIFY(sourceDirectory.isValid());
    QVERIFY(copyDirectory.isValid());
    QVERIFY(moveDirectory.isValid());

    const QString original = sourceDirectory.filePath(
        QStringLiteral("original.png"));
    QFile sourceFile(original);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    QCOMPARE(sourceFile.write("clearveil-test"), 14);
    sourceFile.close();

    auto result = FileOperations::renameFile(
        original, QStringLiteral("../invalid.png"));
    QCOMPARE(result.error,
             FileOperations::Error::InvalidFileName);
    QVERIFY(QFileInfo::exists(original));

    result = FileOperations::renameFile(
        original, QStringLiteral("renamed.png"));
    QVERIFY(result.succeeded());
    const QString renamed = result.targetPath;
    QVERIFY(QFileInfo::exists(renamed));
    QVERIFY(!QFileInfo::exists(original));

    result = FileOperations::renameFile(
        renamed, QStringLiteral("renamed.png"));
    QVERIFY(result.isNoChange());

    result = FileOperations::copyToDirectory(
        renamed, copyDirectory.path());
    QVERIFY(result.succeeded());
    const QString copied = result.targetPath;
    QVERIFY(QFileInfo::exists(copied));
    QFile copiedFile(copied);
    QVERIFY(copiedFile.open(QIODevice::ReadOnly));
    QCOMPARE(copiedFile.readAll(), QByteArray("clearveil-test"));

    result = FileOperations::copyToDirectory(
        renamed, copyDirectory.path());
    QCOMPARE(result.error,
             FileOperations::Error::TargetExists);

    ImageSessionController session;
    session.appendOpenedFiles({renamed});
    session.setLoadedPath(renamed);
    result = FileOperations::moveToDirectory(
        renamed, moveDirectory.path());
    QVERIFY(result.succeeded());
    const QString moved = result.targetPath;
    QVERIFY(QFileInfo::exists(moved));
    QVERIFY(!QFileInfo::exists(renamed));
    QVERIFY(session.replaceOpenedFile(renamed, moved));
    session.setLoadedPath(moved);
    QCOMPARE(session.openedPathAt(0), moved);
    QCOMPARE(session.currentOpenedIndex(), 0);

    result = FileOperations::moveToDirectory(
        moved, moveDirectory.path());
    QVERIFY(result.isNoChange());
    result = FileOperations::copyToDirectory(
        moved, sourceDirectory.filePath(
            QStringLiteral("missing-directory")));
    QCOMPARE(result.error,
             FileOperations::Error::DestinationMissing);
    result = FileOperations::moveToTrash(
        sourceDirectory.filePath(QStringLiteral("missing.png")));
    QCOMPARE(result.error,
             FileOperations::Error::SourceMissing);
    result = FileOperations::launchApplication(
        sourceDirectory.filePath(QStringLiteral("not-executable")),
        moved);
    QCOMPARE(result.error,
             FileOperations::Error::InvalidApplication);
}

void CoreTest::browserFileOperationsAggregateBatchResults()
{
    QTemporaryDir sourceDirectory;
    QTemporaryDir copyDirectory;
    QTemporaryDir moveDirectory;
    QVERIFY(sourceDirectory.isValid());
    QVERIFY(copyDirectory.isValid());
    QVERIFY(moveDirectory.isValid());

    QStringList sources;
    for (int index = 1; index <= 3; ++index) {
        const QString path = sourceDirectory.filePath(
            QStringLiteral("batch-%1.png").arg(index));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(QByteArray::number(index)) > 0);
        file.close();
        sources.append(path);
    }
    sources.append(sourceDirectory.filePath(
        QStringLiteral("missing.png")));

    const auto copied =
        BrowserFileOperationsController::copyFiles(
            sources, copyDirectory.path());
    QCOMPARE(copied.successes.size(), 3);
    QCOMPARE(copied.skipped.size(), 0);
    QCOMPARE(copied.failures.size(), 1);

    const auto copiedAgain =
        BrowserFileOperationsController::copyFiles(
            sources, copyDirectory.path());
    QCOMPARE(copiedAgain.successes.size(), 0);
    QCOMPARE(copiedAgain.skipped.size(), 3);
    QCOMPARE(copiedAgain.failures.size(), 1);

    const auto moved =
        BrowserFileOperationsController::moveFiles(
            sources.mid(0, 3), moveDirectory.path());
    QCOMPARE(moved.successes.size(), 3);
    QCOMPARE(moved.skipped.size(), 0);
    QCOMPARE(moved.failures.size(), 0);
    for (const FileOperations::Result &result : moved.successes) {
        QVERIFY(!QFileInfo::exists(result.sourcePath));
        QVERIFY(QFileInfo::exists(result.targetPath));
    }

    const auto missingTrash =
        BrowserFileOperationsController::trashFiles(
            {sourceDirectory.filePath(
                QStringLiteral("missing-trash.png"))});
    QCOMPARE(missingTrash.successes.size(), 0);
    QCOMPARE(missingTrash.failures.size(), 1);
}

void CoreTest::imageExportAndDesktopIntegrationReturnStructuredResults()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QCOMPARE(ImageExportService::formatForPath(
                 QStringLiteral("photo.JPG")),
             QByteArray("jpeg"));
    QCOMPARE(ImageExportService::formatForPath(
                 QStringLiteral("scan.tif")),
             QByteArray("tiff"));

    auto exportResult = ImageExportService::writeAtomically(
        {}, directory.filePath(QStringLiteral("empty.png")));
    QCOMPARE(exportResult.error,
             ImageExportService::Error::EmptyImage);

    QImage image(80, 50, QImage::Format_ARGB32);
    image.fill(QColor(71, 118, 230, 180));
    const QString pngPath =
        directory.filePath(QStringLiteral("export.png"));
    exportResult = ImageExportService::writeAtomically(
        image, pngPath);
    QVERIFY(exportResult.succeeded());
    QCOMPARE(exportResult.filePath,
             QFileInfo(pngPath).absoluteFilePath());
    QCOMPARE(QImage(pngPath).size(), image.size());

    exportResult = ImageExportService::writeAtomically(
        image,
        directory.filePath(
            QStringLiteral("missing/export.png")));
    QCOMPARE(exportResult.error,
             ImageExportService::Error::OpenFailed);

    const QString unsupportedPath =
        directory.filePath(QStringLiteral("preserved.unknown"));
    QFile preserved(unsupportedPath);
    QVERIFY(preserved.open(QIODevice::WriteOnly));
    QCOMPARE(preserved.write("keep"), 4);
    preserved.close();
    exportResult = ImageExportService::writeAtomically(
        image, unsupportedPath);
    QCOMPARE(exportResult.error,
             ImageExportService::Error::EncodeFailed);
    QVERIFY(preserved.open(QIODevice::ReadOnly));
    QCOMPARE(preserved.readAll(), QByteArray("keep"));
    preserved.close();

    const QString pdfPath =
        directory.filePath(QStringLiteral("printed.pdf"));
    DesktopIntegration::Result printResult;
    {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(pdfPath);
        printResult = DesktopIntegration::printImage(
            printer, image);
    }
    QVERIFY(printResult.succeeded());
    QVERIFY(QFileInfo(pdfPath).size() > 0);

    const auto wallpaperResult =
        DesktopIntegration::requestWallpaper(
            directory.filePath(QStringLiteral("missing.png")));
    QCOMPARE(wallpaperResult.error,
             DesktopIntegration::Error::ImageFileOpenFailed);
}

void CoreTest::documentWorkflowOwnsLoadSaveReloadAndWatchState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath =
        directory.filePath(QStringLiteral("source.png"));
    QImage source(18, 12, QImage::Format_ARGB32);
    source.fill(QColor(42, 93, 171, 220));
    QVERIFY(source.save(sourcePath));

    ImageDocument document;
    FrameController frames;
    DocumentWorkflowController controller(
        &document, &frames);
    QSignalSpy pathSpy(
        &controller,
        &DocumentWorkflowController::currentFileChanged);

    QString error;
    QVERIFY2(controller.loadPath(sourcePath, &error),
             qPrintable(error));
    QCOMPARE(document.filePath(),
             QFileInfo(sourcePath).absoluteFilePath());
    QCOMPARE(controller.displayedImage().size(), source.size());
    QCOMPARE(controller.watchedPath(),
             QFileInfo(sourcePath).absoluteFilePath());
    QCOMPARE(pathSpy.count(), 1);

    QVERIFY(document.rotateClockwise());
    QCOMPARE(controller.displayedImage().size(), QSize(12, 18));
    const QString savedPath =
        directory.filePath(QStringLiteral("saved.png"));
    const ImageExportService::Result saveResult =
        controller.saveAs(savedPath);
    QVERIFY2(saveResult.succeeded(),
             qPrintable(saveResult.detail));
    QVERIFY(!document.isModified());
    QCOMPARE(document.filePath(),
             QFileInfo(savedPath).absoluteFilePath());
    QCOMPARE(controller.watchedPath(),
             QFileInfo(savedPath).absoluteFilePath());

    QImage replacement(7, 5, QImage::Format_RGB32);
    replacement.fill(Qt::darkGreen);
    QVERIFY(replacement.save(savedPath));
    QVERIFY2(controller.reload(&error), qPrintable(error));
    QCOMPARE(controller.displayedImage().size(), QSize(7, 5));

    QImage clipboard(4, 3, QImage::Format_RGB32);
    clipboard.fill(Qt::yellow);
    QVERIFY(controller.loadClipboardImage(clipboard, &error));
    QVERIFY(document.isModified());
    QVERIFY(document.filePath().isEmpty());
    QVERIFY(controller.watchedPath().isEmpty());
    QCOMPARE(controller.displayedImage().size(), QSize(4, 3));

    controller.clear();
    QVERIFY(controller.displayedImage().isNull());
    QVERIFY(controller.watchedPath().isEmpty());
}

void CoreTest::viewerNavigationOwnsPendingActivationAndPrefetch()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QStringList paths;
    for (int index = 0; index < 3; ++index) {
        const QString path = directory.filePath(
            QStringLiteral("image-%1.png").arg(index));
        QImage image(24 + index, 16 + index,
                     QImage::Format_RGB32);
        image.fill(QColor(50 + index * 30, 80, 150));
        QVERIFY(image.save(path));
        paths.append(QFileInfo(path).absoluteFilePath());
    }

    ImageDocument document;
    FrameController frames;
    DocumentWorkflowController workflow(
        &document, &frames);
    ImageSessionController session;
    session.appendOpenedFiles(paths);
    ImageLoadController loader;
    ViewerNavigationController navigation(
        &session, &loader, &workflow, nullptr);
    QSignalSpy successSpy(
        &navigation,
        &ViewerNavigationController::activationSucceeded);
    QSignalSpy failureSpy(
        &navigation,
        &ViewerNavigationController::activationFailed);

    navigation.requestPath(paths.at(1), 1);
    QCOMPARE(session.pendingOpenedIndex(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(successSpy.count(), 1, 2000);
    QCOMPARE(session.pendingOpenedIndex(), -1);
    QCOMPARE(session.currentOpenedIndex(), 1);
    QCOMPARE(document.filePath(), paths.at(1));
    QCOMPARE(successSpy.constFirst().at(1).toInt(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(loader.isCached(paths.at(0)), 2000);
    QTRY_VERIFY_WITH_TIMEOUT(loader.isCached(paths.at(2)), 2000);

    const QString missing =
        directory.filePath(QStringLiteral("missing.png"));
    navigation.requestPath(missing, 0);
    QTRY_COMPARE_WITH_TIMEOUT(failureSpy.count(), 1, 2000);
    QCOMPARE(session.pendingOpenedIndex(), -1);
    QCOMPARE(session.currentOpenedIndex(), 1);
    QCOMPARE(document.filePath(), paths.at(1));

    navigation.requestPath(paths.at(0), 0);
    QVERIFY(navigation.cancel());
    QCOMPARE(session.pendingOpenedIndex(), -1);
    QVERIFY(!navigation.isLoading());
}

void CoreTest::viewerUiStateOwnsActionsAndPresentation()
{
    QAction previous;
    QAction next;
    QAction slideshow;
    QAction saveAs;
    QAction copyImage;
    QAction printImage;
    QAction fileAction;
    QAction undo;
    QAction redo;
    QAction editAction;
    QAction viewAction;
    QAction exportFrame;
    QAction filmstripSource;
    QWidget window;
    QLabel fileLabel;
    QLabel detailLabel;
    QLabel zoomLabel;

    ViewerUiStateController controller(
        {
            &previous, &next, &slideshow, &saveAs,
            &copyImage, nullptr, {&fileAction},
            &printImage, nullptr, &undo, &redo,
            {&editAction}, {&viewAction},
            &exportFrame, nullptr, nullptr,
            &filmstripSource
        },
        {&window, &fileLabel, &detailLabel, &zoomLabel},
        {
            QStringLiteral("Ready"),
            QStringLiteral("Clearveil"),
            QStringLiteral("%1 — Clearveil"),
            QStringLiteral("%1 image(s)"),
            QStringLiteral("Clipboard image"),
            QStringLiteral("Not saved"),
            QStringLiteral("%1 × %2 · %3 · %4/%5"),
            QStringLiteral(" · Locked"),
            QStringLiteral("%1%2 — Clearveil")
        });

    controller.applyDocumentActions({});
    QVERIFY(!saveAs.isEnabled());
    QVERIFY(!fileAction.isEnabled());
    QVERIFY(!editAction.isEnabled());
    QVERIFY(!viewAction.isEnabled());
    QVERIFY(!exportFrame.isEnabled());
    QVERIFY(!filmstripSource.isEnabled());

    controller.applyDocumentActions({
        true, false, true, true, false,
        true, true, true
    });
    QVERIFY(saveAs.isEnabled());
    QVERIFY(copyImage.isEnabled());
    QVERIFY(printImage.isEnabled());
    QVERIFY(fileAction.isEnabled());
    QVERIFY(undo.isEnabled());
    QVERIFY(!redo.isEnabled());
    QVERIFY(editAction.isEnabled());
    QVERIFY(viewAction.isEnabled());
    QVERIFY(exportFrame.isEnabled());
    QVERIFY(filmstripSource.isEnabled());

    controller.applyDocumentActions({
        true, true, true, true, true,
        true, true, true
    });
    QVERIFY(!saveAs.isEnabled());
    QVERIFY(!fileAction.isEnabled());
    QVERIFY(!editAction.isEnabled());
    QVERIFY(!viewAction.isEnabled());
    QVERIFY(undo.isEnabled());
    QVERIFY(redo.isEnabled());

    controller.applyDocumentActions({
        true, false, false, false, false,
        true, false, false, false
    });
    QVERIFY(saveAs.isEnabled());
    QVERIFY(!copyImage.isEnabled());
    QVERIFY(!printImage.isEnabled());

    controller.applyNavigationActions(1, 3);
    QVERIFY(previous.isEnabled());
    QVERIFY(next.isEnabled());
    QVERIFY(slideshow.isEnabled());
    controller.applyNavigationActions(2, 3);
    QVERIFY(previous.isEnabled());
    QVERIFY(!next.isEnabled());

    controller.showReady();
    QCOMPARE(fileLabel.text(), QStringLiteral("Ready"));
    QCOMPARE(fileLabel.accessibleDescription(),
             QStringLiteral("Ready"));
    QVERIFY(detailLabel.text().isEmpty());
    QVERIFY(detailLabel.accessibleDescription().isEmpty());
    QCOMPARE(window.windowTitle(), QStringLiteral("Clearveil"));

    controller.showCollection(QStringLiteral("Pictures"), 12);
    QCOMPARE(fileLabel.text(), QStringLiteral("Pictures"));
    QCOMPARE(detailLabel.text(), QStringLiteral("12 image(s)"));
    QCOMPARE(fileLabel.accessibleDescription(),
             QStringLiteral("Pictures"));
    QCOMPARE(detailLabel.accessibleDescription(),
             QStringLiteral("12 image(s)"));
    QCOMPARE(window.windowTitle(),
             QStringLiteral("Pictures — Clearveil"));

    controller.showImage({
        QStringLiteral("photo.png"), true,
        QSize(640, 480), true, 2048,
        QStringLiteral("2"), 5, 1.5, true
    });
    QCOMPARE(fileLabel.text(), QStringLiteral("● photo.png"));
    QCOMPARE(detailLabel.text(),
             QStringLiteral("640 × 480 · 2.0 KiB · 2/5"));
    QCOMPARE(zoomLabel.text(), QStringLiteral("150% · Locked"));
    QCOMPARE(fileLabel.accessibleDescription(),
             QStringLiteral("● photo.png"));
    QCOMPARE(detailLabel.accessibleDescription(),
             QStringLiteral("640 × 480 · 2.0 KiB · 2/5"));
    QCOMPARE(zoomLabel.accessibleDescription(),
             QStringLiteral("150% · Locked"));
    QCOMPARE(window.windowTitle(),
             QStringLiteral("● photo.png — Clearveil"));
    QCOMPARE(ViewerUiStateController::humanFileSize(512),
             QStringLiteral("512 B"));
}

void CoreTest::sequenceUsesNaturalSorting()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    for (const QString &name : {QStringLiteral("photo10.png"),
                                QStringLiteral("photo2.png"),
                                QStringLiteral("photo1.png")}) {
        QImage image(2, 2, QImage::Format_ARGB32);
        image.fill(Qt::red);
        QVERIFY(image.save(directory.filePath(name)));
    }

    ImageSequence sequence;
    sequence.loadDirectory(directory.path());
    QCOMPARE(sequence.size(), 3);
    QCOMPARE(QFileInfo(sequence.at(0)).fileName(), QStringLiteral("photo1.png"));
    QCOMPARE(QFileInfo(sequence.at(1)).fileName(), QStringLiteral("photo2.png"));
    QCOMPARE(QFileInfo(sequence.at(2)).fileName(), QStringLiteral("photo10.png"));
}

void CoreTest::documentTransformsAndUndoes()
{
    QImage source(3, 2, QImage::Format_ARGB32);
    source.fill(Qt::transparent);
    source.setPixelColor(0, 0, Qt::red);

    ImageDocument document;
    QVERIFY(document.loadImage(source));
    QCOMPARE(document.image().size(), QSize(3, 2));

    QVERIFY(document.rotateClockwise());
    QCOMPARE(document.image().size(), QSize(2, 3));
    QVERIFY(document.canUndo());

    QVERIFY(document.undo());
    QCOMPARE(document.image().size(), QSize(3, 2));
    QVERIFY(document.canRedo());

    QVERIFY(document.redo());
    QCOMPARE(document.image().size(), QSize(2, 3));
}

void CoreTest::imageEditControllerValidatesAndReportsCommands()
{
    ImageDocument document;
    FrameController frames;
    ImageEditController controller(&document, &frames);
    QSignalSpy finishedSpy(
        &controller, &ImageEditController::commandFinished);

    auto result = controller.rotateClockwise();
    QCOMPARE(result.error, ImageEditController::Error::NoImage);
    QVERIFY(!result.changed);

    QImage image(6, 4, QImage::Format_ARGB32);
    image.fill(QColor(30, 60, 90));
    image.setPixelColor(1, 1, QColor(250, 20, 20));
    QVERIFY(document.loadImage(image));
    QVERIFY(controller.canEdit());

    result = controller.crop(image.rect());
    QCOMPARE(result.error, ImageEditController::Error::NoChange);
    result = controller.crop(QRect(20, 20, 5, 5));
    QCOMPARE(result.error,
             ImageEditController::Error::InvalidArgument);
    result = controller.resize({});
    QCOMPARE(result.error,
             ImageEditController::Error::InvalidArgument);
    result = controller.resize(image.size());
    QCOMPARE(result.error, ImageEditController::Error::NoChange);
    result = controller.adjustColors(0, 0, 1.0);
    QCOMPARE(result.error, ImageEditController::Error::NoChange);
    result = controller.adjustColors(101, 0, 1.0);
    QCOMPARE(result.error,
             ImageEditController::Error::InvalidArgument);

    result = controller.rotateClockwise();
    QVERIFY(result.succeeded());
    QVERIFY(result.changed);
    QCOMPARE(result.sizeBefore, QSize(6, 4));
    QCOMPARE(result.sizeAfter, QSize(4, 6));
    QVERIFY(controller.canUndo());
    result = controller.undo();
    QVERIFY(result.succeeded());
    QCOMPARE(document.image().size(), QSize(6, 4));
    QVERIFY(controller.canRedo());
    result = controller.redo();
    QVERIFY(result.succeeded());
    QCOMPARE(document.image().size(), QSize(4, 6));

    QVERIFY(document.loadImage(image));
    result = controller.reduceRedEye(QRect(0, 0, 3, 3));
    QVERIFY(result.succeeded());
    QVERIFY(result.changed);
    QCOMPARE(document.image().pixelColor(1, 1).red(), 21);

    static const QByteArray gifData = QByteArray::fromBase64(
        "R0lGODlhAwACAPAAAP8AAAAAACH/C05FVFNDQVBFMi4wAwEAAAAh+QQAAAAAACwA"
        "AAAAAwACAAACAoRfACH5BAAKAAAALAAAAAADAAIAgAAA/wAAAAIChF8AOw==");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString gifPath =
        directory.filePath(QStringLiteral("active.gif"));
    QFile gifFile(gifPath);
    QVERIFY(gifFile.open(QIODevice::WriteOnly));
    QCOMPARE(gifFile.write(gifData), gifData.size());
    gifFile.close();
    QString error;
    QVERIFY2(frames.open(gifPath, &error), qPrintable(error));
    result = controller.flipHorizontal();
    QCOMPARE(result.error,
             ImageEditController::Error::FrameSequenceActive);
    QVERIFY(!result.changed);
    frames.close();

    QVERIFY(finishedSpy.count() >= 12);
}

void CoreTest::documentSaves()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QImage source(8, 5, QImage::Format_RGB32);
    source.fill(QColor(QStringLiteral("#4776e6")));
    ImageDocument document;
    QVERIFY(document.loadImage(source));
    QVERIFY(document.isModified());

    const QString output = directory.filePath(QStringLiteral("saved.png"));
    QString error;
    QVERIFY2(document.saveAs(output, &error), qPrintable(error));
    QVERIFY(!document.isModified());
    QCOMPARE(QImage(output).size(), QSize(8, 5));
}

void CoreTest::explicitSequencePreservesSelectionOrder()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString first = directory.filePath(QStringLiteral("z.png"));
    const QString second = directory.filePath(QStringLiteral("a.png"));
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::green);
    QVERIFY(image.save(first));
    QVERIFY(image.save(second));

    ImageSequence sequence;
    sequence.loadFiles({first, second, first,
                        directory.filePath(QStringLiteral("note.txt"))});
    QCOMPARE(sequence.size(), 2);
    QCOMPARE(sequence.at(0), first);
    QCOMPARE(sequence.at(1), second);
}

void CoreTest::explicitSequenceAppendsAndReplacesFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::magenta);
    const QString first = directory.filePath(QStringLiteral("first.png"));
    const QString second = directory.filePath(QStringLiteral("second.png"));
    const QString renamed = directory.filePath(QStringLiteral("renamed.png"));
    QVERIFY(image.save(first));
    QVERIFY(image.save(second));
    QVERIFY(image.save(renamed));

    ImageSequence sequence;
    sequence.appendFiles({first});
    sequence.appendFiles({second, first});
    QCOMPARE(sequence.files(), QStringList({first, second}));
    QVERIFY(sequence.replaceFile(second, renamed));
    QCOMPARE(sequence.files(), QStringList({first, renamed}));
}

void CoreTest::documentCanBeCleared()
{
    ImageDocument document;
    QImage image(4, 3, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QVERIFY(document.loadImage(image));
    document.clear();
    QVERIFY(document.image().isNull());
    QVERIFY(document.filePath().isEmpty());
    QVERIFY(!document.isModified());
    QVERIFY(!document.canUndo());
}

void CoreTest::animatedImageExposesFrames()
{
    static const QByteArray gifData = QByteArray::fromBase64(
        "R0lGODlhAwACAPAAAP8AAAAAACH/C05FVFNDQVBFMi4wAwEAAAAh+QQAAAAAACwA"
        "AAAAAwACAAACAoRfACH5BAAKAAAALAAAAAADAAIAgAAA/wAAAAIChF8AOw==");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("animated.gif"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(gifData), gifData.size());
    file.close();

    FrameController frames;
    QSignalSpy frameSpy(&frames, &FrameController::frameChanged);
    QString error;
    QVERIFY2(frames.open(path, &error), qPrintable(error));
    QVERIFY(frames.isAnimated());
    QTRY_VERIFY_WITH_TIMEOUT(frameSpy.count() > 0, 1000);
    QCOMPARE(frames.frameCount(), 2);
    QCOMPARE(frames.currentImage().size(), QSize(3, 2));

    frames.setPlaying(false);
    frames.setCurrentFrame(1);
    QCOMPARE(frames.currentFrame(), 1);
    QCOMPARE(frames.currentImage().pixelColor(0, 0), QColor(Qt::blue));
}

void CoreTest::thumbnailModelListsFoldersAndImagesAsynchronously()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir().mkdir(directory.filePath(QStringLiteral("album"))));

    QImage image(80, 40, QImage::Format_RGB32);
    image.fill(Qt::yellow);
    QVERIFY(image.save(directory.filePath(QStringLiteral("photo2.png"))));
    QVERIFY(image.save(directory.filePath(QStringLiteral("photo10.png"))));
    QFile ignored(directory.filePath(QStringLiteral("notes.txt")));
    QVERIFY(ignored.open(QIODevice::WriteOnly));
    ignored.write("not an image");
    ignored.close();

    ThumbnailModel model;
    model.setDirectory(directory.path());
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0), ThumbnailModel::IsDirectoryRole).toBool(), true);
    QCOMPARE(model.data(model.index(1), Qt::DisplayRole).toString(),
             QStringLiteral("photo2.png"));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
    QVERIFY(model.data(model.index(1), Qt::DecorationRole).isValid());
    QTRY_VERIFY_WITH_TIMEOUT(changedSpy.count() > 0, 2000);
    QCOMPARE(model.thumbnailSize(), QSize(128, 96));
    const QString tooltip = model.data(
        model.index(1), Qt::ToolTipRole).toString();
    QVERIFY(tooltip.contains(QStringLiteral("80 × 40")));
    std::unique_ptr<QMimeData> mimeData(
        model.mimeData({model.index(1), model.index(2)}));
    QCOMPARE(mimeData->urls().size(), 2);
    QVERIFY(mimeData->urls().constFirst().isLocalFile());
}

void CoreTest::thumbnailModelSortsAndUpdatesIncrementally()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto makeImage = [&directory](const QString &name,
                                         const QSize &size) {
        const QString path = directory.filePath(name);
        QImage image(size, QImage::Format_RGB32);
        image.fill(QColor(70, 130, 210));
        return image.save(path) ? path : QString();
    };
    const QString image10 = makeImage(
        QStringLiteral("image10.png"), QSize(40, 30));
    const QString image2 = makeImage(
        QStringLiteral("image2.png"), QSize(80, 60));
    const QString other = makeImage(
        QStringLiteral("other.bmp"), QSize(20, 15));
    QVERIFY(!image10.isEmpty());
    QVERIFY(!image2.isEmpty());
    QVERIFY(!other.isEmpty());

    ThumbnailModel model;
    model.setFiles({image10, image2, other});
    QCOMPARE(model.filePath(model.index(0)), image10);
    QCOMPARE(model.filePath(model.index(1)), image2);
    QCOMPARE(model.filePath(model.index(2)), other);

    model.setSort(ThumbnailModel::SortKey::Name,
                  Qt::AscendingOrder);
    QCOMPARE(model.filePath(model.index(0)), image2);
    QCOMPARE(model.filePath(model.index(1)), image10);
    QCOMPARE(model.filePath(model.index(2)), other);
    QPersistentModelIndex persistentImage10(model.index(1));

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy insertedSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removedSpy(&model, &QAbstractItemModel::rowsRemoved);
    const QString image3 = makeImage(
        QStringLiteral("image3.png"), QSize(60, 45));
    QVERIFY(!image3.isEmpty());
    model.setFiles({image10, image2, other, image3});
    QCOMPARE(resetSpy.count(), 0);
    QVERIFY(insertedSpy.count() > 0);
    QCOMPARE(model.filePath(model.index(0)), image2);
    QCOMPARE(model.filePath(model.index(1)), image3);
    QCOMPARE(model.filePath(model.index(2)), image10);
    QVERIFY(persistentImage10.isValid());
    QCOMPARE(model.filePath(persistentImage10), image10);

    model.setFiles({image10, other, image3});
    QCOMPARE(resetSpy.count(), 0);
    QVERIFY(removedSpy.count() > 0);
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.filePath(model.index(0)), image3);
    QCOMPARE(model.filePath(model.index(1)), image10);
    QCOMPARE(model.filePath(model.index(2)), other);

    model.setSort(ThumbnailModel::SortKey::FileType,
                  Qt::DescendingOrder);
    QCOMPARE(model.filePath(model.index(0)), image10);
    QCOMPARE(model.filePath(model.index(1)), image3);
    QCOMPARE(model.filePath(model.index(2)), other);
}

void CoreTest::directoryScanServiceCachesAndRefreshes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir().mkdir(
        directory.filePath(QStringLiteral("subfolder"))));
    QImage image(32, 24, QImage::Format_RGB32);
    image.fill(Qt::darkCyan);
    for (int index = 1; index <= 120; ++index) {
        QVERIFY(image.save(directory.filePath(
            QStringLiteral("scan-%1.png").arg(
                index, 3, 10, QLatin1Char('0')))));
    }

    DirectoryScanService service;
    QSignalSpy finishedSpy(
        &service, &DirectoryScanService::scanFinished);
    const quint64 firstRequest =
        service.requestScan(directory.path());
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3000);
    QCOMPARE(finishedSpy.constFirst().at(0).toULongLong(),
             firstRequest);
    const DirectoryScanResult firstResult =
        qvariant_cast<DirectoryScanResult>(
            finishedSpy.constFirst().at(1));
    QVERIFY(firstResult.succeeded());
    QCOMPARE(firstResult.imageCount(), 120);
    QCOMPARE(firstResult.entries.constFirst().directory, true);

    DirectoryScanResult cached;
    QVERIFY(service.cachedResult(directory.path(), &cached));
    QCOMPARE(cached.imageCount(), 120);
    const quint64 cachedRequest =
        service.requestScan(directory.path());
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 2, 1000);
    QCOMPARE(finishedSpy.at(1).at(0).toULongLong(),
             cachedRequest);

    QVERIFY(image.save(directory.filePath(
        QStringLiteral("scan-121.png"))));
    const quint64 refreshedRequest =
        service.requestScan(directory.path(), true);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 3, 3000);
    QCOMPARE(finishedSpy.at(2).at(0).toULongLong(),
             refreshedRequest);
    const DirectoryScanResult refreshed =
        qvariant_cast<DirectoryScanResult>(
            finishedSpy.at(2).at(1));
    QCOMPARE(refreshed.imageCount(), 121);
}

void CoreTest::directoryFilmstripRefreshesIncrementally()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString firstPath = directory.filePath(
        QStringLiteral("first.png"));
    QImage image(48, 32, QImage::Format_RGB32);
    image.fill(QColor(80, 150, 220));
    QVERIFY(image.save(firstPath));

    MainWindow window;
    QVERIFY(window.openPath(firstPath));
    auto *sourceAction = window.findChild<QAction *>(
        QStringLiteral("filmstripSourceAction"));
    auto *view = window.findChild<QListView *>(
        QStringLiteral("filmstrip"));
    QVERIFY(sourceAction);
    QVERIFY(view);
    sourceAction->setChecked(true);
    QCoreApplication::processEvents();
    auto *model = qobject_cast<ThumbnailModel *>(view->model());
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 1);

    QSignalSpy resetSpy(model, &QAbstractItemModel::modelReset);
    QSignalSpy insertedSpy(model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removedSpy(model, &QAbstractItemModel::rowsRemoved);
    const QString secondPath = directory.filePath(
        QStringLiteral("second.png"));
    QVERIFY(image.save(secondPath));
    QTRY_COMPARE_WITH_TIMEOUT(model->rowCount(), 2, 2500);
    QCOMPARE(resetSpy.count(), 0);
    QVERIFY(insertedSpy.count() > 0);

    QVERIFY(QFile::remove(secondPath));
    QTRY_COMPARE_WITH_TIMEOUT(model->rowCount(), 1, 2500);
    QCOMPARE(resetSpy.count(), 0);
    QVERIFY(removedSpy.count() > 0);
}

void CoreTest::folderOpenUsesViewerAndOverviewAvoidsDuplicateStrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    for (int index = 1; index <= 3; ++index) {
        QImage image(64, 48, QImage::Format_RGB32);
        image.fill(QColor::fromHsv(index * 70, 180, 220));
        QVERIFY(image.save(directory.filePath(
            QStringLiteral("folder-%1.png").arg(index))));
    }

    MainWindow window;
    QVERIFY(window.openPath(directory.path()));
    window.show();

    auto *sourceAction = window.findChild<QAction *>(
        QStringLiteral("filmstripSourceAction"));
    auto *overviewAction = window.findChild<QAction *>(
        QStringLiteral("folderOverviewAction"));
    auto *filmstrip = window.findChild<QListView *>(
        QStringLiteral("filmstrip"));
    auto *filmstripDock = window.findChild<QDockWidget *>(
        QStringLiteral("filmstripDock"));
    auto *browser = window.findChild<BrowserWidget *>();
    QVERIFY(sourceAction);
    QVERIFY(overviewAction);
    QVERIFY(filmstrip);
    QVERIFY(filmstripDock);
    QVERIFY(browser);
    QVERIFY(sourceAction->isChecked());
    QTRY_COMPARE_WITH_TIMEOUT(
        filmstrip->model()->rowCount(), 3, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(
        window.windowTitle().contains(QStringLiteral("folder-1.png")),
        3000);
    QVERIFY(!browser->isVisible());

    overviewAction->setChecked(true);
    QCoreApplication::processEvents();
    QVERIFY(browser->isVisible());
    QVERIFY(!filmstripDock->isVisible());
    auto *showFilmstripAction = window.findChild<QAction *>(
        QStringLiteral("filmstrip"));
    if (!showFilmstripAction) {
        for (QAction *action : window.findChildren<QAction *>()) {
            if (action->shortcut() == QKeySequence(QStringLiteral("T"))) {
                showFilmstripAction = action;
                break;
            }
        }
    }
    QVERIFY(showFilmstripAction);
    showFilmstripAction->setChecked(false);
    showFilmstripAction->setChecked(true);
    QCoreApplication::processEvents();
    QVERIFY(!filmstripDock->isVisible());
    bool foundCount = false;
    for (QLabel *label : window.findChildren<QLabel *>()) {
        if (label->text().contains(QStringLiteral("3 image(s)"))) {
            foundCount = true;
            break;
        }
    }
    QVERIFY(foundCount);

    const QString overviewPath = directory.filePath(
        QStringLiteral("folder-3.png"));
    QVERIFY(QMetaObject::invokeMethod(
        browser, "imageActivated", Qt::DirectConnection,
        Q_ARG(QString, overviewPath)));
    QTRY_VERIFY_WITH_TIMEOUT(
        window.windowTitle().contains(
            QStringLiteral("folder-3.png")),
        3000);
    QVERIFY(sourceAction->isChecked());
    sourceAction->setChecked(false);
    QCOMPARE(filmstrip->model()->rowCount(), 0);
    sourceAction->setChecked(true);

    overviewAction->setChecked(false);
    QCoreApplication::processEvents();
    QVERIFY(!browser->isVisible());
    QVERIFY(filmstripDock->isVisible());

    QVERIFY(window.openPath(directory.filePath(
        QStringLiteral("folder-2.png"))));
    QVERIFY(!sourceAction->isChecked());
    QCOMPARE(filmstrip->model()->rowCount(), 1);
}

void CoreTest::largeDirectoryOpenReturnsBeforeScanCompletes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString seedPath = directory.filePath(
        QStringLiteral("large-0000.png"));
    QImage seed(96, 64, QImage::Format_RGB32);
    seed.fill(QColor(50, 120, 200));
    QVERIFY(seed.save(seedPath));
    constexpr int imageCount = 1500;
    for (int index = 1; index < imageCount; ++index) {
        QVERIFY(QFile::link(seedPath, directory.filePath(
            QStringLiteral("large-%1.png").arg(
                index, 4, 10, QLatin1Char('0')))));
    }

    MainWindow window;
    QElapsedTimer timer;
    timer.start();
    QVERIFY(window.openPath(directory.path()));
    QVERIFY2(timer.elapsed() < 250,
             qPrintable(QStringLiteral(
                 "Opening a folder blocked for %1 ms")
                 .arg(timer.elapsed())));

    auto *filmstrip = window.findChild<QListView *>(
        QStringLiteral("filmstrip"));
    QVERIFY(filmstrip);
    QTRY_COMPARE_WITH_TIMEOUT(
        filmstrip->model()->rowCount(), imageCount, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(
        window.windowTitle().contains(
            QStringLiteral("large-0000.png")),
        5000);
}

void CoreTest::persistentThumbnailCacheIsOptInAndInvalidates()
{
    QTemporaryDir sourceDirectory;
    QTemporaryDir cacheParent;
    QVERIFY(sourceDirectory.isValid());
    QVERIFY(cacheParent.isValid());
    const QString cacheDirectory =
        cacheParent.filePath(QStringLiteral("thumbnails"));
    const QString firstPath =
        sourceDirectory.filePath(QStringLiteral("first.png"));
    const QString secondPath =
        sourceDirectory.filePath(QStringLiteral("second.png"));

    QImage first(320, 200, QImage::Format_RGB32);
    first.fill(QColor(220, 80, 40));
    QVERIFY(first.save(firstPath));
    QImage second(420, 260, QImage::Format_RGB32);
    second.fill(QColor(40, 120, 220));
    QVERIFY(second.save(secondPath));

    PersistentThumbnailCache::configure(
        false, 8LL * 1024LL * 1024LL, cacheDirectory);
    {
        ThumbnailModel model;
        model.setFiles({firstPath});
        QSignalSpy changed(
            &model, &QAbstractItemModel::dataChanged);
        QVERIFY(model.data(model.index(0),
                           Qt::DecorationRole).isValid());
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() > 0, 2000);
    }
    QVERIFY(!QDir(cacheDirectory).exists());

    PersistentThumbnailCache::configure(
        true, 8LL * 1024LL * 1024LL, cacheDirectory);
    const QSize cacheBucket =
        PersistentThumbnailCache::bucketSize(QSize(128, 96));
    PersistentThumbnailCache::store(
        secondPath, cacheBucket,
        second.scaled(cacheBucket, Qt::KeepAspectRatio,
                      Qt::SmoothTransformation));
    QVERIFY(PersistentThumbnailCache::sizeBytes() > 0);
    QVERIFY(PersistentThumbnailCache::clear());
    {
        ThumbnailModel model;
        model.setFiles({secondPath});
        QSignalSpy changed(
            &model, &QAbstractItemModel::dataChanged);
        QVERIFY(model.data(model.index(0),
                           Qt::DecorationRole).isValid());
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() > 0, 2000);
    }
    QTRY_VERIFY_WITH_TIMEOUT(
        PersistentThumbnailCache::sizeBytes() > 0, 2000);
    QDirIterator initialFiles(
        cacheDirectory, {QStringLiteral("*.png")},
        QDir::Files, QDirIterator::Subdirectories);
    int initialCount = 0;
    while (initialFiles.hasNext()) {
        initialFiles.next();
        ++initialCount;
    }
    QVERIFY(initialCount > 0);

    QImage changedImage(280, 180, QImage::Format_RGB32);
    changedImage.fill(QColor(30, 190, 90));
    QVERIFY(changedImage.save(secondPath));
    {
        ThumbnailModel model;
        model.setFiles({secondPath});
        QSignalSpy changed(
            &model, &QAbstractItemModel::dataChanged);
        QVERIFY(model.data(model.index(0),
                           Qt::DecorationRole).isValid());
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() > 0, 2000);
    }
    QDirIterator invalidatedFiles(
        cacheDirectory, {QStringLiteral("*.png")},
        QDir::Files, QDirIterator::Subdirectories);
    int invalidatedCount = 0;
    while (invalidatedFiles.hasNext()) {
        invalidatedFiles.next();
        ++invalidatedCount;
    }
    QVERIFY(invalidatedCount > initialCount);

    QVERIFY(PersistentThumbnailCache::clear());
    QCOMPARE(PersistentThumbnailCache::sizeBytes(), 0);
    PersistentThumbnailCache::configure(
        true, 1, cacheDirectory);
    PersistentThumbnailCache::store(
        secondPath, cacheBucket,
        second.scaled(cacheBucket, Qt::KeepAspectRatio,
                      Qt::SmoothTransformation));
    QCOMPARE(PersistentThumbnailCache::sizeBytes(), 0);
    PersistentThumbnailCache::configure(
        false,
        PersistentThumbnailCache::defaultMaximumBytes);
}

void CoreTest::formatCapabilitiesReflectRuntimeAndExplainFailures()
{
    const QSet<QString> readable =
        FormatCapabilities::readableExtensions();
    QVERIFY(readable.contains(QStringLiteral("png")));
    QVERIFY(FormatCapabilities::canReadExtension(
        QStringLiteral(".PNG")));
    QVERIFY(FormatCapabilities::imageDialogPatterns().contains(
        QStringLiteral("*.avif")));

    const QList<ImageFormatCapability> capabilities =
        FormatCapabilities::capabilities();
    QVERIFY(!capabilities.isEmpty());
    const auto png = std::find_if(
        capabilities.cbegin(), capabilities.cend(),
        [](const ImageFormatCapability &capability) {
            return capability.name == QStringLiteral("PNG");
        });
    QVERIFY(png != capabilities.cend());
    QVERIFY(png->readable);

    const ImageFormatInstallationAdvice archHeic =
        FormatCapabilities::installationAdvice(
            QStringLiteral(".HEIC"), QStringLiteral("arch"));
    QCOMPARE(archHeic.packages, QStringList({
        QStringLiteral("kimageformats"),
        QStringLiteral("libheif"),
    }));
    QCOMPARE(
        archHeic.command,
        QStringLiteral(
            "sudo pacman -S kimageformats libheif"));

    const ImageFormatInstallationAdvice debianWebp =
        FormatCapabilities::installationAdvice(
            QStringLiteral("webp"), QStringLiteral("debian"));
    QCOMPARE(
        debianWebp.command,
        QStringLiteral(
            "sudo apt install qt6-image-formats-plugins"));

    const ImageFormatInstallationAdvice fedoraJxl =
        FormatCapabilities::installationAdvice(
            QStringLiteral("jxl"), QStringLiteral("fedora"));
    QCOMPARE(
        fedoraJxl.command,
        QStringLiteral(
            "sudo dnf install kf6-kimageformats"));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString invalidPng =
        directory.filePath(QStringLiteral("damaged.png"));
    QFile invalidFile(invalidPng);
    QVERIFY(invalidFile.open(QIODevice::WriteOnly));
    QCOMPARE(invalidFile.write("not a png"), 9);
    invalidFile.close();
    const ImageLoadResult invalidResult =
        ImageDocument::decodeFile(invalidPng);
    QVERIFY(!invalidResult.succeeded());
    QVERIFY(invalidResult.error.contains(
        QStringLiteral("damaged"), Qt::CaseInsensitive));
    QVERIFY(invalidResult.error.contains(
        QStringLiteral("Decoder details")));

    const QString unknown =
        directory.filePath(QStringLiteral("sample.clearveil_unknown"));
    QFile unknownFile(unknown);
    QVERIFY(unknownFile.open(QIODevice::WriteOnly));
    QCOMPARE(unknownFile.write("unknown"), 7);
    unknownFile.close();
    const QString unsupported =
        FormatCapabilities::friendlyDecodeError(
            unknown, QImageReader::UnsupportedFormatError,
            QStringLiteral("Unsupported image format"));
    QVERIFY(unsupported.contains(
        QStringLiteral(".CLEARVEIL_UNKNOWN")));
    QVERIFY(unsupported.contains(
        QStringLiteral("Qt image plugin")));

    const QString missing =
        directory.filePath(QStringLiteral("missing.png"));
    QVERIFY(FormatCapabilities::friendlyDecodeError(
                missing, QImageReader::FileNotFoundError, {})
                .contains(QStringLiteral("no longer exists")));
}

void CoreTest::formatCapabilitiesDialogExposesRuntimeReport()
{
    FormatCapabilitiesDialog dialog;
    auto *tree = dialog.findChild<QTreeWidget *>(
        QStringLiteral("formatCapabilitiesTree"));
    auto *summary = dialog.findChild<QLabel *>(
        QStringLiteral("formatCapabilitiesSummary"));
    auto *copy = dialog.findChild<QPushButton *>(
        QStringLiteral("copyFormatReportButton"));
    auto *installAdvice = dialog.findChild<QLabel *>(
        QStringLiteral("formatInstallAdvice"));
    auto *copyInstall = dialog.findChild<QPushButton *>(
        QStringLiteral("copyFormatInstallCommandButton"));
    QVERIFY(tree);
    QVERIFY(summary);
    QVERIFY(copy);
    QVERIFY(installAdvice);
    QVERIFY(copyInstall);
    QVERIFY(tree->topLevelItemCount() >= 4);
    QCOMPARE(tree->columnCount(), 5);
    QVERIFY(summary->text().contains(QStringLiteral("Qt ")));
    QVERIFY(!installAdvice->text().isEmpty());
    const QList<ImageFormatCapability> capabilities =
        FormatCapabilities::capabilities();
    int checkedCapabilities = 0;
    for (int categoryIndex = 0;
         categoryIndex < tree->topLevelItemCount();
         ++categoryIndex) {
        QTreeWidgetItem *category =
            tree->topLevelItem(categoryIndex);
        for (int itemIndex = 0;
             itemIndex < category->childCount();
             ++itemIndex) {
            QTreeWidgetItem *item =
                category->child(itemIndex);
            const auto capability = std::find_if(
                capabilities.cbegin(),
                capabilities.cend(),
                [item](
                    const ImageFormatCapability &value) {
                    return value.name == item->text(0);
                });
            QVERIFY(capability
                    != capabilities.cend());
            QCOMPARE(
                item->text(2),
                capability->readable
                    ? QStringLiteral("✓")
                    : QStringLiteral("✕"));
            QCOMPARE(
                item->text(3),
                capability->writable
                    ? QStringLiteral("✓")
                    : QStringLiteral("✕"));
            ++checkedCapabilities;
        }
    }
    QCOMPARE(checkedCapabilities,
             capabilities.size());

    copy->click();
    QVERIFY(QApplication::clipboard()->text().startsWith(
        QStringLiteral("Clearveil image format support")));
}

void CoreTest::ocrSupportExplainsDistributionPackages()
{
    const OcrInstallationAdvice arch =
        OcrSupport::installationAdvice(
            QStringLiteral("arch"), false);
    QCOMPARE(arch.command,
             QStringLiteral(
                 "sudo pacman -S tesseract tesseract-data-eng tesseract-data-chi_sim"));

    const OcrInstallationAdvice ubuntu =
        OcrSupport::installationAdvice(
            QStringLiteral("ubuntu"), false);
    QCOMPARE(ubuntu.packages, QStringList({
        QStringLiteral("tesseract-ocr"),
        QStringLiteral("tesseract-ocr-eng"),
        QStringLiteral("tesseract-ocr-chi-sim"),
    }));
    QVERIFY(!ubuntu.packages.contains(
        QStringLiteral("libtesseract-dev")));

    const OcrInstallationAdvice ubuntuBuild =
        OcrSupport::installationAdvice(
            QStringLiteral("ubuntu"), true);
    QVERIFY(ubuntuBuild.packages.contains(
        QStringLiteral("libtesseract-dev")));
    QVERIFY(ubuntuBuild.note.contains(
        QStringLiteral("rebuild"), Qt::CaseInsensitive));

    OcrSupportDialog dialog;
    QVERIFY(dialog.findChild<QLabel *>(
        QStringLiteral("ocrEngineStatus")));
    QVERIFY(dialog.findChild<QLabel *>(
        QStringLiteral("ocrInstalledLanguages")));
    QVERIFY(dialog.findChild<QLabel *>(
        QStringLiteral("ocrPackageAdvice")));
    auto *command = dialog.findChild<QPlainTextEdit *>(
        QStringLiteral("ocrInstallCommand"));
    QVERIFY(command);
    QVERIFY(command->toPlainText().contains(
        QStringLiteral("tesseract")));

    MainWindow window;
    auto *action = window.findChild<QAction *>(
        QStringLiteral("ocrSupportAction"));
    QVERIFY(action);
    bool dialogOpened = false;
    QTimer::singleShot(0, &window, [&dialogOpened] {
        auto *opened = qobject_cast<OcrSupportDialog *>(
            QApplication::activeModalWidget());
        QVERIFY(opened);
        dialogOpened = true;
        opened->reject();
    });
    action->trigger();
    QVERIFY(dialogOpened);
}

void CoreTest::aboutDialogExposesVersionAndProjectInformation()
{
    const QString originalVersion =
        QCoreApplication::applicationVersion();
    QCoreApplication::setApplicationVersion(
        QStringLiteral("9.8.7-test"));
    const auto restoreVersion = qScopeGuard(
        [originalVersion] {
        QCoreApplication::setApplicationVersion(originalVersion);
    });

    AboutDialog dialog;
    auto *version = dialog.findChild<QLabel *>(
        QStringLiteral("aboutVersionLabel"));
    auto *description = dialog.findChild<QLabel *>(
        QStringLiteral("aboutDescriptionLabel"));
    auto *runtime = dialog.findChild<QLabel *>(
        QStringLiteral("aboutRuntimeLabel"));
    auto *license = dialog.findChild<QLabel *>(
        QStringLiteral("aboutLicenseLabel"));
    auto *project = dialog.findChild<QLabel *>(
        QStringLiteral("aboutProjectLink"));
    auto *buttons = dialog.findChild<QDialogButtonBox *>(
        QStringLiteral("aboutButtonBox"));
    QVERIFY(version);
    QVERIFY(description);
    QVERIFY(runtime);
    QVERIFY(license);
    QVERIFY(project);
    QVERIFY(buttons);
    QCOMPARE(version->text(), QStringLiteral("Version 9.8.7-test"));
    QVERIFY(!description->text().isEmpty());
    QVERIFY(runtime->text().contains(QStringLiteral("Qt ")));
    QVERIFY(license->text().contains(QStringLiteral("GPL")));
    QVERIFY(project->text().contains(AboutDialog::projectUrl()));
    QVERIFY(project->openExternalLinks());

    MainWindow window;
    auto *aboutAction = window.findChild<QAction *>(
        QStringLiteral("aboutAction"));
    QVERIFY(aboutAction);
    bool mainWindowDialogOpened = false;
    QTimer::singleShot(0, &window, [&mainWindowDialogOpened] {
        auto *opened = qobject_cast<AboutDialog *>(
            QApplication::activeModalWidget());
        QVERIFY(opened);
        mainWindowDialogOpened = true;
        opened->reject();
    });
    aboutAction->trigger();
    QVERIFY(mainWindowDialogOpened);
}

void CoreTest::singleInstanceForwardsExactlyOnce()
{
    QTemporaryDir runtimeDirectory;
    QVERIFY(runtimeDirectory.isValid());
    const QString instanceId = QStringLiteral(
        "clearveil-single-instance-test-%1")
        .arg(QCoreApplication::applicationPid());
    SingleInstance primary(
        instanceId, runtimeDirectory.path());
    QVERIFY2(primary.startOrForward({}),
             "The isolated local IPC server did not start.");
    QSignalSpy received(
        &primary, &SingleInstance::pathsReceived);

    SingleInstance secondary(
        instanceId, runtimeDirectory.path());
    QVERIFY(!secondary.startOrForward({
        QStringLiteral("/tmp/clearveil-single-instance-test.png"),
    }));
    QTRY_COMPARE_WITH_TIMEOUT(received.count(), 1, 2000);
    const QStringList forwarded =
        received.constFirst().constFirst().toStringList();
    QCOMPARE(forwarded.size(), 1);
    QCOMPARE(forwarded.constFirst(),
             QStringLiteral(
                 "/tmp/clearveil-single-instance-test.png"));
    QTest::qWait(50);
    QCOMPARE(received.count(), 1);
}

void CoreTest::documentCropResizeAndAdjustAreUndoable()
{
    QImage source(10, 8, QImage::Format_ARGB32);
    source.fill(QColor(40, 60, 80, 170));
    ImageDocument document;
    QVERIFY(document.loadImage(source));

    document.crop(QRect(2, 1, 5, 4));
    QCOMPARE(document.image().size(), QSize(5, 4));
    document.resizeImage(QSize(20, 12));
    QCOMPARE(document.image().size(), QSize(20, 12));

    const QColor before = document.image().pixelColor(0, 0);
    document.adjustImage(20, 10, 1.2);
    const QColor after = document.image().pixelColor(0, 0);
    QVERIFY(after.red() > before.red());
    QCOMPARE(after.alpha(), 170);

    document.undo();
    QCOMPARE(document.image().pixelColor(0, 0), before);
    document.undo();
    QCOMPARE(document.image().size(), QSize(5, 4));
    document.undo();
    QCOMPARE(document.image().size(), QSize(10, 8));
}

void CoreTest::cropSelectionCanBeMovedAndResized()
{
    QImage image(100, 100, QImage::Format_ARGB32);
    image.fill(Qt::white);
    CropDialog dialog(image);
    dialog.show();
    QTest::qWait(20);

    QWidget *preview = dialog.findChild<QWidget *>(
        QStringLiteral("cropPreview"));
    QVERIFY(preview);
    constexpr qreal controlMargin = 7.0;
    const qreal side = std::min(
        preview->width() - controlMargin * 2.0,
        preview->height() - controlMargin * 2.0);
    const QPointF topLeft(
        (preview->width() - side) / 2.0,
        (preview->height() - side) / 2.0);
    const auto widgetPoint =
        [topLeft, side](const QPoint &imagePoint) {
        return QPoint(
            qRound(topLeft.x()
                   + imagePoint.x() * side / 100.0),
            qRound(topLeft.y()
                   + imagePoint.y() * side / 100.0));
    };

    QTest::mouseMove(
        preview, widgetPoint(QPoint(0, 0)), 5);
    QCOMPARE(preview->cursor().shape(), Qt::SizeFDiagCursor);
    QTest::mouseMove(
        preview, widgetPoint(QPoint(50, 0)), 5);
    QCOMPARE(preview->cursor().shape(), Qt::SizeVerCursor);
    QTest::mouseMove(
        preview, widgetPoint(QPoint(0, 50)), 5);
    QCOMPARE(preview->cursor().shape(), Qt::SizeHorCursor);
    QTest::mouseMove(
        preview, widgetPoint(QPoint(50, 50)), 5);
    QCOMPARE(preview->cursor().shape(), Qt::OpenHandCursor);

    const QPoint outsideTopLeft =
        widgetPoint(QPoint(0, 0)) - QPoint(3, 3);
    QTest::mousePress(
        preview, Qt::LeftButton, Qt::NoModifier,
        outsideTopLeft);
    QTest::mouseMove(
        preview, widgetPoint(QPoint(8, 8)), 5);
    QTest::mouseRelease(
        preview, Qt::LeftButton, Qt::NoModifier,
        widgetPoint(QPoint(8, 8)));
    QVERIFY(dialog.cropRectangle().topLeft().x() > 0);
    QVERIFY(dialog.cropRectangle().topLeft().y() > 0);

    QTest::mousePress(
        preview, Qt::LeftButton, Qt::NoModifier,
        widgetPoint(QPoint(2, 2)));
    QTest::mouseMove(
        preview, widgetPoint(QPoint(60, 60)), 5);
    QTest::mouseRelease(
        preview, Qt::LeftButton, Qt::NoModifier,
        widgetPoint(QPoint(60, 60)));
    const QRect created = dialog.cropRectangle();
    QVERIFY(created.width() < image.width());
    QVERIFY(created.height() < image.height());

    QTest::mousePress(
        preview, Qt::LeftButton, Qt::NoModifier,
        widgetPoint(created.center()));
    QTest::mouseMove(
        preview,
        widgetPoint(created.center() + QPoint(10, 8)), 5);
    QTest::mouseRelease(
        preview, Qt::LeftButton, Qt::NoModifier,
        widgetPoint(created.center() + QPoint(10, 8)));
    const QRect moved = dialog.cropRectangle();
    QCOMPARE(moved.size(), created.size());
    QVERIFY(moved.topLeft() != created.topLeft());

    QTest::mousePress(
        preview, Qt::LeftButton, Qt::NoModifier,
        widgetPoint(moved.bottomRight()));
    QTest::mouseMove(
        preview,
        widgetPoint(moved.bottomRight() + QPoint(10, 10)), 5);
    QTest::mouseRelease(
        preview, Qt::LeftButton, Qt::NoModifier,
        widgetPoint(moved.bottomRight() + QPoint(10, 10)));
    const QRect resized = dialog.cropRectangle();
    QVERIFY(resized.width() > moved.width());
    QVERIFY(resized.height() > moved.height());

    auto *xEditor = dialog.findChild<QSpinBox *>(
        QStringLiteral("cropX"));
    auto *yEditor = dialog.findChild<QSpinBox *>(
        QStringLiteral("cropY"));
    auto *widthEditor = dialog.findChild<QSpinBox *>(
        QStringLiteral("cropWidth"));
    auto *heightEditor = dialog.findChild<QSpinBox *>(
        QStringLiteral("cropHeight"));
    QVERIFY(xEditor);
    QVERIFY(yEditor);
    QVERIFY(widthEditor);
    QVERIFY(heightEditor);

    const auto resetSelection = [&] {
        xEditor->setValue(0);
        yEditor->setValue(0);
        widthEditor->setValue(60);
        heightEditor->setValue(60);
        xEditor->setValue(20);
        yEditor->setValue(20);
        QCOMPARE(dialog.cropRectangle(), QRect(20, 20, 60, 60));
    };
    struct HandleDrag {
        QPoint handle;
        QPoint delta;
        Qt::CursorShape cursor;
    };
    const QList<HandleDrag> handleDrags{
        {{20, 20}, {6, 7}, Qt::SizeFDiagCursor},
        {{50, 20}, {0, 7}, Qt::SizeVerCursor},
        {{80, 20}, {-6, 7}, Qt::SizeBDiagCursor},
        {{80, 50}, {-6, 0}, Qt::SizeHorCursor},
        {{80, 80}, {-6, -7}, Qt::SizeFDiagCursor},
        {{50, 80}, {0, -7}, Qt::SizeVerCursor},
        {{20, 80}, {6, -7}, Qt::SizeBDiagCursor},
        {{20, 50}, {6, 0}, Qt::SizeHorCursor},
    };
    for (const HandleDrag &drag : handleDrags) {
        resetSelection();
        const QPoint start = widgetPoint(drag.handle);
        QTest::mouseMove(preview, start, 5);
        QCOMPARE(preview->cursor().shape(), drag.cursor);
        QTest::mousePress(
            preview, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(
            preview, widgetPoint(drag.handle + drag.delta), 5);
        QTest::mouseRelease(
            preview, Qt::LeftButton, Qt::NoModifier,
            widgetPoint(drag.handle + drag.delta));
        QVERIFY2(
            dialog.cropRectangle() != QRect(20, 20, 60, 60),
            qPrintable(QStringLiteral(
                "Crop handle at %1,%2 did not resize")
                .arg(drag.handle.x())
                .arg(drag.handle.y())));
    }
}

void CoreTest::colorPickerOffersCopyableFormats()
{
    const QString originalOrganization =
        QCoreApplication::organizationName();
    const QString originalApplication =
        QCoreApplication::applicationName();
    const QSettings::Format settingsFormat =
        QSettings::defaultFormat();
    const QString originalSettingsPath =
        QStandardPaths::writableLocation(
            QStandardPaths::GenericConfigLocation);
    QTemporaryDir settingsDirectory;
    QVERIFY(settingsDirectory.isValid());
    QSettings::setPath(settingsFormat, QSettings::UserScope,
                       settingsDirectory.path());
    const auto restoreSettings = qScopeGuard(
        [originalOrganization, originalApplication,
         originalSettingsPath, settingsFormat] {
        QSettings().clear();
        QCoreApplication::setOrganizationName(originalOrganization);
        QCoreApplication::setApplicationName(originalApplication);
        QSettings::setPath(settingsFormat, QSettings::UserScope,
                           originalSettingsPath);
    });
    QCoreApplication::setOrganizationName(
        QStringLiteral("ClearveilTest"));
    QCoreApplication::setApplicationName(
        QStringLiteral("clearveil-color-picker-test"));
    QSettings().clear();

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage image(320, 320, QImage::Format_RGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixelColor(
                x, y, QColor(x % 256, y % 256, 90));
        }
    }
    const QString path =
        directory.filePath(QStringLiteral("picker.png"));
    QVERIFY(image.save(path));

    MainWindow window;
    window.resize(920, 620);
    window.show();
    QVERIFY(window.openPath(path));
    auto *canvas = window.findChild<ImageCanvas *>();
    auto *pickerAction = window.findChild<QAction *>(
        QStringLiteral("colorPickerAction"));
    auto *pickerDock = window.findChild<QDockWidget *>(
        QStringLiteral("colorPickerDock"));
    auto *pickerPanel = window.findChild<ColorPickerPanel *>(
        QStringLiteral("colorPickerPanel"));
    auto *pickerController = window.findChild<ColorPickerController *>(
        QStringLiteral("colorPickerController"));
    auto *magnifier = window.findChild<QWidget *>(
        QStringLiteral("pixelMagnifier"));
    auto *hexField = window.findChild<QLineEdit *>(
        QStringLiteral("pickedColorHEX"));
    auto *rgbField = window.findChild<QLineEdit *>(
        QStringLiteral("pickedColorRGB"));
    auto *rgbCopyButton = window.findChild<QToolButton *>(
        QStringLiteral("copyRGBColorButton"));
    auto *resumeButton = window.findChild<QToolButton *>(
        QStringLiteral("resumeColorSamplingButton"));
    auto *historyList = window.findChild<QListWidget *>(
        QStringLiteral("colorHistoryList"));
    auto *copyButton = window.findChild<QToolButton *>(
        QStringLiteral("copyPickedColorButton"));
    auto *positionX = window.findChild<QLabel *>(
        QStringLiteral("pickedColorPositionX"));
    auto *positionY = window.findChild<QLabel *>(
        QStringLiteral("pickedColorPositionY"));
    auto *axisLegend = window.findChild<QWidget *>(
        QStringLiteral("coordinateAxisLegend"));
    QVERIFY(canvas);
    QVERIFY(pickerAction);
    QVERIFY(pickerDock);
    QVERIFY(pickerPanel);
    QVERIFY(pickerController);
    QVERIFY(magnifier);
    QVERIFY(hexField);
    QVERIFY(rgbField);
    QVERIFY(rgbCopyButton);
    QVERIFY(resumeButton);
    QVERIFY(historyList);
    QVERIFY(copyButton);
    QVERIFY(positionX);
    QVERIFY(positionY);
    QVERIFY(axisLegend);

    pickerAction->setChecked(true);
    QVERIFY(pickerAction->isChecked());
    QVERIFY(pickerController->isEnabled());
    QTRY_VERIFY(pickerDock->isVisible());
    QTRY_VERIFY(!canvas->displayImage().isNull());
    QVERIFY(magnifier->isVisible());
    QVERIFY(pickerPanel->preferredHeight() < window.height());
    QTRY_VERIFY(pickerDock->height()
                <= pickerPanel->preferredHeight() + 64);
    QVERIFY(pickerAction->isChecked());
    QVERIFY(pickerController->isEnabled());

    QSignalSpy hovered(canvas, &ImageCanvas::colorHovered);
    const QPoint samplePoint = canvas->rect().center();
    QMouseEvent moveEvent(
        QEvent::MouseMove, QPointF(samplePoint),
        QPointF(canvas->mapToGlobal(samplePoint)),
        Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(canvas, &moveEvent);
    QTRY_VERIFY(!hovered.isEmpty());
    const QList<QVariant> hoveredArguments =
        hovered.constLast();
    const QColor hoveredColor =
        hoveredArguments.at(0).value<QColor>();
    const QImage hoveredSample =
        hoveredArguments.at(2).value<QImage>();
    QVERIFY(hoveredColor.isValid());
    QCOMPARE(hoveredSample.size(), QSize(11, 11));
    QCOMPARE(
        hoveredSample.pixelColor(5, 5),
        hoveredColor);
    QImage magnifierRendering(
        magnifier->size(),
        QImage::Format_ARGB32_Premultiplied);
    magnifierRendering.fill(Qt::transparent);
    magnifier->render(&magnifierRendering);
    QCOMPARE(
        magnifierRendering.pixelColor(
            magnifier->rect().center()),
        hoveredColor);
    QVERIFY(hexField->text() != QStringLiteral("—"));
    QVERIFY(positionX->text().startsWith(QStringLiteral("X")));
    QVERIFY(positionY->text().startsWith(QStringLiteral("Y")));
    QVERIFY(!positionX->text().contains(QLatin1Char(',')));
    QVERIFY(!positionY->text().contains(QLatin1Char(',')));

    const QPoint firstPoint = canvas->rect().center();
    QTest::mouseClick(
        canvas, Qt::LeftButton, Qt::NoModifier,
        firstPoint);
    QVERIFY(canvas->isColorSamplePinned());
    QVERIFY(resumeButton->isEnabled());
    QCOMPARE(historyList->count(), 1);
    const QPoint pinnedPosition =
        hovered.constLast().at(1).toPoint();
    QSignalSpy adjusted(
        canvas, &ImageCanvas::colorSampleAdjusted);
    QTest::keyClick(canvas, Qt::Key_Right);
    QTRY_COMPARE(adjusted.size(), 1);
    QCOMPARE(
        adjusted.constLast().at(1).toPoint(),
        pinnedPosition + QPoint(1, 0));
    QCOMPARE(historyList->count(), 1);
    QTest::keyClick(
        canvas, Qt::Key_Down, Qt::ShiftModifier);
    QTRY_COMPARE(adjusted.size(), 2);
    QCOMPARE(
        adjusted.constLast().at(1).toPoint(),
        pinnedPosition + QPoint(1, 5));
    QCOMPARE(historyList->count(), 1);
    const QString pinnedHex = hexField->text();
    const qsizetype samplesAfterPin = hovered.size();
    const QPoint secondPoint =
        firstPoint + QPoint(80, 80);
    QTest::mouseMove(canvas, secondPoint, 5);
    QCoreApplication::processEvents();
    QCOMPARE(hovered.size(), samplesAfterPin);
    QCOMPARE(hexField->text(), pinnedHex);

    QTest::keyClick(&window, Qt::Key_Escape);
    QVERIFY(!canvas->isColorSamplePinned());
    QVERIFY(!resumeButton->isEnabled());
    const QPoint resumedPoint =
        firstPoint + QPoint(20, 0);
    QTest::mouseMove(canvas, resumedPoint, 5);
    QTRY_VERIFY(hovered.size()
                > samplesAfterPin);

    QTest::mouseClick(
        canvas, Qt::LeftButton, Qt::NoModifier,
        resumedPoint);
    QVERIFY(canvas->isColorSamplePinned());
    QVERIFY(hexField->text() != pinnedHex);
    const QString secondHex = hexField->text();
    QCOMPARE(historyList->count(), 2);
    QListWidgetItem *newestHistory =
        historyList->item(0);
    QListWidgetItem *firstHistory =
        historyList->item(1);
    const QString newestHistoryText =
        newestHistory->text();
    const QString firstHistoryText =
        firstHistory->text();

    const QRect firstHistoryRect =
        historyList->visualItemRect(
            firstHistory);
    QVERIFY(firstHistoryRect.isValid());
    QTest::mouseClick(
        historyList->viewport(), Qt::LeftButton,
        Qt::NoModifier, firstHistoryRect.center());
    QCOMPARE(hexField->text(), pinnedHex);
    QVERIFY(canvas->isColorSamplePinned());
    QCOMPARE(historyList->item(0), newestHistory);
    QCOMPARE(historyList->item(1), firstHistory);
    QCOMPARE(historyList->item(0)->text(),
             newestHistoryText);
    QCOMPARE(historyList->item(1)->text(),
             firstHistoryText);

    QTest::keyClick(historyList, Qt::Key_Up);
    QCOMPARE(historyList->currentRow(), 0);
    QCOMPARE(hexField->text(), secondHex);
    QCOMPARE(historyList->item(0), newestHistory);
    QCOMPARE(historyList->item(1), firstHistory);
    QTest::keyClick(historyList, Qt::Key_Down);
    QCOMPARE(historyList->currentRow(), 1);
    QCOMPARE(hexField->text(), pinnedHex);

    resumeButton->click();
    QVERIFY(!canvas->isColorSamplePinned());
    QVERIFY(!resumeButton->isEnabled());
    const qsizetype samplesBeforeResumeMove =
        hovered.size();
    QTest::mouseMove(
        canvas, firstPoint - QPoint(20, 20), 5);
    QTRY_VERIFY(hovered.size()
                > samplesBeforeResumeMove);

    rgbCopyButton->click();
    QCOMPARE(
        QApplication::clipboard()->text(),
        rgbField->text());

    QVERIFY(QMetaObject::invokeMethod(
        canvas, "colorPicked", Qt::DirectConnection,
        Q_ARG(QColor, QColor(10, 20, 30, 128)),
        Q_ARG(QPoint, QPoint(4, 7))));
    QVERIFY(!copyButton->isHidden());
    copyButton->click();
    QCOMPARE(
        QApplication::clipboard()->text(),
        QStringLiteral("#0A141E"));

    QAction *rgbaAction = nullptr;
    for (QAction *action : copyButton->menu()->actions()) {
        if (action->text() == QStringLiteral("Copy RGBA")) {
            rgbaAction = action;
            break;
        }
    }
    QVERIFY(rgbaAction);
    rgbaAction->trigger();
    QCOMPARE(
        QApplication::clipboard()->text(),
        QStringLiteral("rgba(10, 20, 30, 0.502)"));

    pickerAction->setChecked(false);
    QTRY_VERIFY(!pickerDock->isVisible());
}

void CoreTest::zoomLockFreezesCurrentPercentage()
{
    ImageCanvas canvas;
    canvas.resize(800, 600);
    QImage first(1600, 1200, QImage::Format_RGB32);
    first.fill(Qt::red);
    canvas.setImage(first);
    canvas.fitToWindow();
    const qreal lockedZoom = canvas.zoom();

    canvas.setZoomLocked(true);
    QCOMPARE(canvas.zoomMode(), ImageCanvas::ZoomMode::Custom);
    QImage second(3200, 900, QImage::Format_RGB32);
    second.fill(Qt::blue);
    canvas.setImage(second);
    QVERIFY(qAbs(canvas.zoom() - lockedZoom) < 0.0001);
    canvas.resize(1000, 500);
    QVERIFY(qAbs(canvas.zoom() - lockedZoom) < 0.0001);

    canvas.setZoomLocked(false);
    canvas.setImage(first);
    QCOMPARE(canvas.zoomMode(), ImageCanvas::ZoomMode::Fit);
}

void CoreTest::folderSlideshowActivatesItsThumbnail()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QStringList paths;
    for (int index = 1; index <= 3; ++index) {
        QImage image(80, 60, QImage::Format_RGB32);
        image.fill(QColor::fromHsv(index * 70, 255, 220));
        const QString path = directory.filePath(
            QStringLiteral("slide-%1.png").arg(index));
        QVERIFY(image.save(path));
        paths.append(path);
    }

    MainWindow window;
    QVERIFY(window.openPath(paths.constFirst()));
    window.show();
    auto *sourceAction = window.findChild<QAction *>(
        QStringLiteral("filmstripSourceAction"));
    auto *filmstrip = window.findChild<QListView *>(
        QStringLiteral("filmstrip"));
    auto *slideshow = window.findChild<QAction *>(
        QStringLiteral("slideshowAction"));
    auto *timer = window.findChild<QTimer *>(
        QStringLiteral("slideshowTimer"));
    auto *fileLabel = window.findChild<QLabel *>(
        QStringLiteral("currentFileLabel"));
    QVERIFY(sourceAction);
    QVERIFY(filmstrip);
    QVERIFY(slideshow);
    QVERIFY(timer);
    QVERIFY(fileLabel);

    sourceAction->setChecked(true);
    QCoreApplication::processEvents();
    QCOMPARE(filmstrip->model()->rowCount(), 3);
    QVERIFY(slideshow->isEnabled());
    const int initialRow = filmstrip->currentIndex().row();
    timer->setInterval(30);
    slideshow->setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(
        filmstrip->currentIndex().row() != initialRow,
        1000);
    slideshow->setChecked(false);

    const QModelIndex current = filmstrip->currentIndex();
    QVERIFY(current.isValid());
    QVERIFY(filmstrip->selectionModel()->isSelected(current));
    auto *model =
        qobject_cast<ThumbnailModel *>(filmstrip->model());
    QVERIFY(model);
    const QString activeFile =
        QFileInfo(model->filePath(current)).fileName();
    QTRY_COMPARE_WITH_TIMEOUT(
        fileLabel->text(), activeFile, 2000);
}

void CoreTest::compareAcceptsFewerThanFourImages()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage image(8, 6, QImage::Format_RGB32);
    image.fill(Qt::cyan);
    const QString first = directory.filePath(QStringLiteral("first.png"));
    const QString second = directory.filePath(QStringLiteral("second.png"));
    QVERIFY(image.save(first));
    QVERIFY(image.save(second));

    CompareWidget compare;
    compare.setFiles({first, second});
    QCOMPARE(compare.findChildren<ImageCanvas *>().size(), 2);
}

void CoreTest::redEyeCorrectionIsSelectiveAndUndoable()
{
    QImage image(9, 9, QImage::Format_RGBA8888);
    image.fill(QColor(45, 50, 55));
    image.setPixelColor(4, 4, QColor(240, 35, 30));
    image.setPixelColor(0, 0, QColor(240, 35, 30));

    ImageDocument document;
    QVERIFY(document.loadImage(image));
    document.reduceRedEye(QRect(2, 2, 5, 5));
    QVERIFY(document.image().pixelColor(4, 4).red() < 100);
    QCOMPARE(document.image().pixelColor(0, 0), QColor(240, 35, 30));
    document.undo();
    QCOMPARE(document.image().pixelColor(4, 4), QColor(240, 35, 30));
}

void CoreTest::metadataPanelShowsPresentEmbeddedMetadata()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("metadata.png"));
    QImage image(7, 5, QImage::Format_RGB32);
    image.fill(Qt::yellow);
    QImageWriter writer(path, "png");
    writer.setText(QStringLiteral("Author"), QStringLiteral("Clearveil Test"));
    writer.setText(QStringLiteral("Empty field"), QString());
    QVERIFY2(writer.write(image), qPrintable(writer.errorString()));

    MetadataPanel panel;
    panel.setImage(path, image);
    auto *tree = panel.findChild<QTreeWidget *>();
    QVERIFY(tree);
    QCOMPARE(tree->indentation(), 0);
    QCOMPARE(tree->header()->sectionResizeMode(0),
             QHeaderView::Stretch);
    QCOMPARE(tree->header()->sectionResizeMode(1),
             QHeaderView::Stretch);
    QCOMPARE(tree->selectionMode(), QAbstractItemView::NoSelection);
    QVERIFY(!tree->wordWrap());
    bool foundAuthor = false;
    bool foundEmbeddedTextGroup = false;
    QSet<QString> rows;
    for (int groupIndex = 0; groupIndex < tree->topLevelItemCount();
         ++groupIndex) {
        QTreeWidgetItem *group = tree->topLevelItem(groupIndex);
        QVERIFY(group);
        // QTreeWidgetItem retains only structure/data. SelectableLabel is the
        // sole visual text layer, preventing transparent double painting.
        QVERIFY(group->text(0).isEmpty());
        const QString groupText =
            group->data(0, Qt::UserRole).toString();
        QVERIFY(!groupText.trimmed().isEmpty());
        QVERIFY(group->isExpanded());
        group->setExpanded(false);
        QVERIFY(!group->isExpanded());
        group->setExpanded(true);
        QVERIFY(group->isExpanded());
        if (groupText.contains(QStringLiteral("Embedded text")))
            foundEmbeddedTextGroup = true;
        for (int childIndex = 0; childIndex < group->childCount();
             ++childIndex) {
            QTreeWidgetItem *property = group->child(childIndex);
            QVERIFY(property);
            QVERIFY(property->text(0).isEmpty());
            QVERIFY(property->text(1).isEmpty());
            const QString propertyText =
                property->data(0, Qt::UserRole).toString();
            const QString valueText =
                property->data(1, Qt::UserRole).toString();
            QVERIFY(!propertyText.trimmed().isEmpty());
            QVERIFY(!valueText.trimmed().isEmpty());
            QVERIFY(!propertyText.contains(QStringLiteral(" · ")));
            const QString fingerprint = groupText + QChar(0x1e)
                + propertyText + QChar(0x1f) + valueText;
            QVERIFY2(!rows.contains(fingerprint),
                     qPrintable(QStringLiteral("Duplicate metadata row: %1")
                                    .arg(fingerprint)));
            rows.insert(fingerprint);
            if (propertyText.contains(QStringLiteral("Author"))
                && valueText == QStringLiteral("Clearveil Test")) {
                foundAuthor = true;
            }
        }
    }
    QVERIFY(foundEmbeddedTextGroup);
    QVERIFY(foundAuthor);

    const auto groupLabels = tree->findChildren<QLabel *>(
        QStringLiteral("metadataGroupText"));
    const auto propertyLabels = tree->findChildren<QLabel *>(
        QStringLiteral("metadataPropertyText"));
    const auto valueLabels = tree->findChildren<QLabel *>(
        QStringLiteral("metadataValueText"));
    QVERIFY(!groupLabels.isEmpty());
    QVERIFY(!propertyLabels.isEmpty());
    QVERIFY(!valueLabels.isEmpty());

    QLabel *authorValueLabel = nullptr;
    for (QLabel *label : valueLabels) {
        if (label->text() == QStringLiteral("Clearveil Test")) {
            authorValueLabel = label;
            break;
        }
    }
    QVERIFY(authorValueLabel);
    QVERIFY(authorValueLabel->textInteractionFlags()
            & Qt::TextSelectableByMouse);
    QVERIFY(authorValueLabel->textInteractionFlags()
            & Qt::TextSelectableByKeyboard);
    authorValueLabel->setSelection(0, authorValueLabel->text().size());
    QApplication::clipboard()->clear();
    QTest::keyClick(authorValueLabel, Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(QApplication::clipboard()->text(),
             QStringLiteral("Clearveil Test"));
}

void CoreTest::browserRefreshesItsViewportPalette()
{
    BrowserWidget browser;
    QPalette palette = browser.palette();
    const QColor darkBase(QStringLiteral("#191b1e"));
    palette.setColor(QPalette::Base, darkBase);
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#232529")));
    browser.setPalette(palette);
    browser.refreshAppearance();

    auto *view = browser.findChild<QListView *>();
    QVERIFY(view);
    QCOMPARE(view->viewport()->palette().color(QPalette::Base), darkBase);
}

void CoreTest::browserRestoresPerDirectoryState()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString firstDirectory =
        root.filePath(QStringLiteral("first"));
    const QString secondDirectory =
        root.filePath(QStringLiteral("second"));
    QVERIFY(QDir().mkdir(firstDirectory));
    QVERIFY(QDir().mkdir(secondDirectory));
    QImage image(48, 32, QImage::Format_RGB32);
    image.fill(Qt::darkYellow);
    const QString seed = QDir(firstDirectory).filePath(
        QStringLiteral("state-000.png"));
    QVERIFY(image.save(seed));
    for (int index = 1; index < 80; ++index) {
        QVERIFY(QFile::link(seed, QDir(firstDirectory).filePath(
            QStringLiteral("state-%1.png").arg(
                index, 3, 10, QLatin1Char('0')))));
    }
    for (int index = 0; index < 4; ++index) {
        QVERIFY(image.save(QDir(secondDirectory).filePath(
            QStringLiteral("other-%1.png").arg(index))));
    }

    BrowserWidget browser;
    browser.resize(420, 230);
    browser.show();
    auto *view = browser.findChild<QListView *>(
        QStringLiteral("folderBrowserView"));
    QVERIFY(view);
    browser.setDirectory(firstDirectory);
    QTRY_COMPARE_WITH_TIMEOUT(view->model()->rowCount(), 80, 3000);
    const QModelIndex remembered = view->model()->index(60, 0);
    const QString rememberedPath = remembered.data(
        ThumbnailModel::FilePathRole).toString();
    view->setCurrentIndex(remembered);
    view->scrollTo(remembered,
                   QAbstractItemView::PositionAtCenter);
    QCoreApplication::processEvents();
    const int rememberedScroll =
        view->verticalScrollBar()->value();
    QVERIFY(rememberedScroll > 0);

    browser.setDirectory(secondDirectory);
    QTRY_COMPARE_WITH_TIMEOUT(view->model()->rowCount(), 4, 3000);
    browser.setDirectory(firstDirectory);
    QTRY_COMPARE_WITH_TIMEOUT(view->model()->rowCount(), 80, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(
        view->currentIndex().data(
            ThumbnailModel::FilePathRole).toString(),
        rememberedPath, 1000);
    QTRY_COMPARE_WITH_TIMEOUT(
        view->verticalScrollBar()->value(),
        rememberedScroll, 1000);

    QSignalSpy operationSpy(
        &browser, &BrowserWidget::fileOperationRequested);
    view->setFocus();
    QTest::keyClick(view, Qt::Key_F2);
    QCOMPARE(operationSpy.count(), 1);
    QCOMPARE(operationSpy.constFirst().at(0).toString(),
             QStringLiteral("rename"));
    QCOMPARE(operationSpy.constFirst().at(1).toStringList(),
             QStringList({rememberedPath}));
    QTest::keyClick(view, Qt::Key_Delete);
    QCOMPARE(operationSpy.count(), 2);
    QCOMPARE(operationSpy.at(1).at(0).toString(),
             QStringLiteral("trash"));

    auto *historyBack = browser.findChild<QToolButton *>(
        QStringLiteral("folderHistoryBackButton"));
    auto *historyForward = browser.findChild<QToolButton *>(
        QStringLiteral("folderHistoryForwardButton"));
    auto *locations = browser.findChild<QToolButton *>(
        QStringLiteral("folderLocationsButton"));
    QVERIFY(historyBack);
    QVERIFY(historyForward);
    QVERIFY(locations);
    QVERIFY(historyBack->isEnabled());
    QVERIFY(!historyForward->isEnabled());
    historyBack->click();
    QTRY_COMPARE_WITH_TIMEOUT(view->model()->rowCount(), 4, 3000);
    QVERIFY(historyForward->isEnabled());
    view->setFocus();
    QTest::keyClick(
        view, Qt::Key_Right, Qt::AltModifier);
    QTRY_COMPARE_WITH_TIMEOUT(view->model()->rowCount(), 80, 3000);
    QVERIFY(browser.recentDirectories().contains(firstDirectory));
    QVERIFY(browser.recentDirectories().contains(secondDirectory));

    QMenu *locationsMenu = locations->menu();
    QVERIFY(locationsMenu);
    QTimer::singleShot(0, locationsMenu, &QMenu::close);
    locations->showMenu();
    QAction *addFavorite = nullptr;
    for (QAction *action : locationsMenu->actions()) {
        if (action->text()
            == QStringLiteral("Add current folder to favorites")) {
            addFavorite = action;
            break;
        }
    }
    QVERIFY(addFavorite);
    addFavorite->trigger();
    QCOMPARE(browser.favoriteDirectories(),
             QStringList({firstDirectory}));
}

void CoreTest::mainToolbarCompactsWithoutOverflow()
{
    QSettings settings;
    const QVariant savedToolbarLayout = settings.value(
        QStringLiteral("ui/toolbarLayout"));
    settings.remove(QStringLiteral("ui/toolbarLayout"));

    const auto iconShapeStable = [](const QIcon &icon) {
        const auto mask = [&icon](int extent) {
            const QImage source = icon.pixmap(
                QSize(extent, extent), QIcon::Normal,
                QIcon::Off).toImage();
            return source.scaled(
                QSize(44, 44), Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation);
        };
        const QImage compact = mask(18);
        const QImage regular = mask(22);
        int intersection = 0;
        int unionCount = 0;
        for (int y = 0; y < compact.height(); ++y) {
            for (int x = 0; x < compact.width(); ++x) {
                const bool compactPixel =
                    compact.pixelColor(x, y).alpha() >= 48;
                const bool regularPixel =
                    regular.pixelColor(x, y).alpha() >= 48;
                intersection += compactPixel && regularPixel;
                unionCount += compactPixel || regularPixel;
            }
        }
        return unionCount > 0
            && static_cast<qreal>(intersection) / unionCount >= 0.68;
    };
    const auto iconContrastsWith = [](const QIcon &icon,
                                      const QSize &size,
                                      const QColor &background,
                                      QIcon::Mode mode,
                                      QIcon::State state) {
        const QImage image = icon.pixmap(size, mode, state).toImage();
        int greatestDifference = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.alpha() < 48)
                    continue;
                const int difference =
                    std::abs(pixel.red() - background.red())
                    + std::abs(pixel.green() - background.green())
                    + std::abs(pixel.blue() - background.blue());
                greatestDifference = std::max(
                    greatestDifference, difference);
            }
        }
        return greatestDifference >= 45;
    };
    const auto iconHasReasonableDensity = [](const QIcon &icon) {
        const QImage image = icon.pixmap(
            QSize(32, 32), QIcon::Normal,
            QIcon::Off).toImage();
        int painted = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x)
                painted += image.pixelColor(x, y).alpha() >= 48;
        }
        const qreal ratio = static_cast<qreal>(painted)
            / (image.width() * image.height());
        return painted > 0 && ratio <= 0.85;
    };

    MainWindow window;
    window.resize(window.minimumWidth(), 600);
    window.show();
    QCoreApplication::processEvents();

    auto *toolbar = window.findChild<QToolBar *>(
        QStringLiteral("mainToolbar"));
    QVERIFY(toolbar);
    QVERIFY2(toolbar->actions().size() >= 20,
             "The first-run toolbar must expose the common viewer actions");
    QCOMPARE(window.minimumWidth(), 820);
    QVERIFY(toolbar->property("compact").toBool());
    QCOMPARE(toolbar->iconSize(), QSize(18, 18));
    for (QAction *action : toolbar->actions()) {
        auto *button = qobject_cast<QToolButton *>(
            toolbar->widgetForAction(action));
        if (button) {
            QVERIFY2(!button->icon().isNull(),
                     qPrintable(button->toolTip()));
            if (!action->icon().isNull()) {
                QVERIFY2(!action->isIconVisibleInMenu(),
                         qPrintable(action->text()));
            }
            QVERIFY2(iconShapeStable(button->icon()),
                     qPrintable(button->toolTip()));
            if (button->objectName()
                != QStringLiteral("mainMenuButton")) {
                QVERIFY2(iconHasReasonableDensity(button->icon()),
                         qPrintable(button->toolTip()));
            }
            const QIcon::Mode mode = button->isEnabled()
                ? QIcon::Normal : QIcon::Disabled;
            const QIcon::State state = button->isChecked()
                ? QIcon::On : QIcon::Off;
            QVERIFY2(iconContrastsWith(
                         button->icon(), toolbar->iconSize(),
                         toolbar->palette().color(QPalette::Window),
                         mode, state),
                     qPrintable(button->toolTip()));
        }
    }

    auto *sourceAction = window.findChild<QAction *>(
        QStringLiteral("filmstripSourceAction"));
    QVERIFY(sourceAction);
    const qint64 sourceIconKey = sourceAction->icon().cacheKey();
    sourceAction->setChecked(true);
    QCOMPARE(sourceAction->icon().cacheKey(), sourceIconKey);

    auto *extension = toolbar->findChild<QToolButton *>(
        QStringLiteral("qt_toolbar_ext_button"));
    QVERIFY(!extension || !extension->isVisible());
    for (QAction *action : toolbar->actions()) {
        QWidget *widget = toolbar->widgetForAction(action);
        QVERIFY2(widget && widget->isVisible(),
                 qPrintable(action->text()));
    }
    auto *emptyFilmstripOverlay = window.findChild<QScrollBar *>(
        QStringLiteral("filmstripHorizontalScrollBar"));
    QVERIFY(emptyFilmstripOverlay);
    QVERIFY(!emptyFilmstripOverlay->isVisible());

    window.resize(1120, 760);
    QCoreApplication::processEvents();
    QVERIFY(!toolbar->property("compact").toBool());
    QCOMPARE(toolbar->iconSize(), QSize(22, 22));

    bool inspectedSettingsIcons = false;
    bool redundantInformationActionAbsent = false;
    QString settingsIconFailure;
    QTimer::singleShot(0, &window,
                       [&inspectedSettingsIcons,
                        &redundantInformationActionAbsent,
                        &settingsIconFailure,
                        &iconShapeStable,
                        &iconHasReasonableDensity] {
        auto *dialog = qobject_cast<SettingsDialog *>(
            QApplication::activeModalWidget());
        if (!dialog)
            return;
        auto *items = dialog->findChild<QListWidget *>(
            QStringLiteral("toolbarItems"));
        if (items) {
            inspectedSettingsIcons = items->count() > 0;
            redundantInformationActionAbsent = true;
            for (int row = 0; row < items->count(); ++row) {
                if (items->item(row)->data(Qt::UserRole).toString()
                    == QStringLiteral("properties")) {
                    redundantInformationActionAbsent = false;
                }
                const QIcon icon = items->item(row)->icon();
                const bool nullIcon = icon.isNull();
                const bool stableIcon = !nullIcon
                    && iconShapeStable(icon);
                const bool reasonableDensity = stableIcon
                    && iconHasReasonableDensity(icon);
                if (!reasonableDensity) {
                    inspectedSettingsIcons = false;
                    settingsIconFailure = QStringLiteral(
                        "%1 (%2): null=%3 stable=%4 density=%5")
                        .arg(items->item(row)->text(),
                             items->item(row)->data(
                                 Qt::UserRole).toString())
                        .arg(nullIcon)
                        .arg(stableIcon)
                        .arg(reasonableDensity);
                    break;
                }
            }
        }
        dialog->reject();
    });
    QVERIFY(QMetaObject::invokeMethod(
        &window, "showPreferences", Qt::DirectConnection));
    QVERIFY2(inspectedSettingsIcons,
             qPrintable(settingsIconFailure));
    QVERIFY(redundantInformationActionAbsent);

    if (savedToolbarLayout.isValid()) {
        settings.setValue(QStringLiteral("ui/toolbarLayout"),
                          savedToolbarLayout);
    } else {
        settings.remove(QStringLiteral("ui/toolbarLayout"));
    }
}

void CoreTest::windowChromePopupAndDragRegionsAreUsable()
{
    class PopupMainWindow final : public MainWindow
    {
    public:
        using MainWindow::createPopupMenu;
    };

    PopupMainWindow mainWindow;
    std::unique_ptr<QMenu> popup(mainWindow.createPopupMenu());
    QVERIFY(popup);
    QCOMPARE(popup->objectName(),
             QStringLiteral("windowLayoutPopupMenu"));
    int visibleEntryCount = 0;
    for (QAction *action : popup->actions()) {
        if (action->isSeparator())
            continue;
        ++visibleEntryCount;
        QVERIFY2(!action->text().trimmed().isEmpty(),
                 "Window layout popup contains a blank entry");
    }
    QCOMPARE(visibleEntryCount, 1);
    QCOMPARE(popup->actions().constFirst()->objectName(),
             QStringLiteral("interfaceLayoutAction"));

    QMainWindow window;
    window.resize(640, 320);
    QMenuBar *menuBar = window.menuBar();
    menuBar->addMenu(QStringLiteral("File"));
    auto *toolBar = window.addToolBar(QStringLiteral("Toolbar"));
    QAction *buttonAction = toolBar->addAction(
        QStringLiteral("Button"));
    auto *spacer = new QWidget(toolBar);
    spacer->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);

    WindowDragController controller(&window);
    controller.addDragSurface(menuBar);
    controller.addDragSurface(toolBar);
    QSignalSpy moveSpy(
        &controller, &WindowDragController::systemMoveRequested);
    QSignalSpy stateSpy(
        &controller,
        &WindowDragController::windowStateToggleRequested);
    window.show();
    QCoreApplication::processEvents();

    QToolButton *button = qobject_cast<QToolButton *>(
        toolBar->widgetForAction(buttonAction));
    QVERIFY(button);
    QTest::mousePress(
        toolBar, Qt::LeftButton, Qt::NoModifier,
        button->geometry().center());
    QCOMPARE(moveSpy.count(), 0);

    QVERIFY(spacer->width() > 0);
    QTest::mousePress(
        toolBar, Qt::LeftButton, Qt::NoModifier,
        spacer->geometry().center());
    QCOMPARE(moveSpy.count(), 1);

    QAction *fileMenuAction = menuBar->actions().constFirst();
    QTest::mousePress(
        menuBar, Qt::LeftButton, Qt::NoModifier,
        menuBar->actionGeometry(fileMenuAction).center());
    QCOMPARE(moveSpy.count(), 1);

    const QPoint emptyMenuPosition(
        menuBar->width() - 8, menuBar->height() / 2);
    QVERIFY(menuBar->actionAt(emptyMenuPosition) == nullptr);
    QTest::mousePress(
        menuBar, Qt::LeftButton, Qt::NoModifier,
        emptyMenuPosition);
    QCOMPARE(moveSpy.count(), 2);

    QMenu *popupMenu = menuBar->addMenu(
        QStringLiteral("Popup"));
    popupMenu->addAction(QStringLiteral("Entry"));
    popupMenu->popup(menuBar->mapToGlobal(emptyMenuPosition));
    QTRY_VERIFY(popupMenu->isVisible());
    QTest::mouseClick(
        popupMenu, Qt::LeftButton, Qt::NoModifier,
        QPoint(-8, -8));
    QCOMPARE(moveSpy.count(), 2);
    QTRY_VERIFY(!popupMenu->isVisible());

    QTest::mouseDClick(
        button, Qt::LeftButton, Qt::NoModifier,
        button->rect().center());
    QCOMPARE(stateSpy.count(), 0);
    QVERIFY(!window.isMaximized());

    QTest::mouseDClick(
        spacer, Qt::LeftButton, Qt::NoModifier,
        spacer->rect().center());
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.constFirst().constFirst().toBool(), true);
    QTRY_VERIFY(window.isMaximized());

    const QPoint maximizedEmptyMenuPosition(
        menuBar->width() - 8, menuBar->height() / 2);
    QVERIFY(menuBar->actionAt(maximizedEmptyMenuPosition) == nullptr);
    QTest::mouseDClick(
        menuBar, Qt::LeftButton, Qt::NoModifier,
        maximizedEmptyMenuPosition);
    QCOMPARE(stateSpy.count(), 2);
    QCOMPARE(stateSpy.constLast().constFirst().toBool(), false);
    QTRY_VERIFY(!window.isMaximized());
}

void CoreTest::toolbarSettingsCanReorderToggleAndReset()
{
    QPixmap toolbarItemPixmap(24, 24);
    toolbarItemPixmap.fill(Qt::blue);
    const QIcon toolbarItemIcon(toolbarItemPixmap);
    const QList<ActionRegistry::ToolbarItemDefinition> definitions{
        {QStringLiteral("alpha"), QStringLiteral("Alpha"), toolbarItemIcon},
        {QStringLiteral("beta"), QStringLiteral("Beta"), toolbarItemIcon},
        {QStringLiteral("gamma"), QStringLiteral("Gamma"), toolbarItemIcon}
    };
    const QStringList defaults{
        QStringLiteral("alpha"),
        QStringLiteral("beta"),
        QStringLiteral("!gamma")
    };
    SettingsDialog dialog(
        QStringLiteral("system"), QStringLiteral("system"),
        QStringLiteral("top"), QStringLiteral("bottom"), 3,
        true, true, 256, 1, QStringLiteral("name"), true,
        false, false, 256, false, 512, 0, definitions,
        {QStringLiteral("beta"), QStringLiteral("!alpha"),
         QStringLiteral("gamma")},
        defaults, {}, {}, {},
        QStringLiteral("zoom"),
        QStringLiteral("zoom"),
        QStringLiteral("toggle_zoom"),
        QStringLiteral("none"),
        QStringLiteral("previous"),
        QStringLiteral("next"));

    auto *items = dialog.findChild<QListWidget *>(
        QStringLiteral("toolbarItems"));
    QVERIFY(items);
    QCOMPARE(items->count(), 3);
    for (int row = 0; row < items->count(); ++row)
        QVERIFY(!items->item(row)->icon().isNull());
    QCOMPARE(items->item(0)->data(Qt::UserRole).toString(),
             QStringLiteral("beta"));
    QCOMPARE(items->item(1)->checkState(), Qt::Unchecked);

    items->setCurrentRow(1);
    auto *moveUp = dialog.findChild<QPushButton *>(
        QStringLiteral("toolbarMoveUpButton"));
    QVERIFY(moveUp);
    moveUp->click();
    QCOMPARE(dialog.toolbarLayout(),
             QStringList({QStringLiteral("!alpha"),
                          QStringLiteral("beta"),
                          QStringLiteral("gamma")}));

    items->item(1)->setCheckState(Qt::Unchecked);
    QCOMPARE(dialog.toolbarLayout().at(1),
             QStringLiteral("!beta"));

    auto *reset = dialog.findChild<QPushButton *>(
        QStringLiteral("toolbarResetButton"));
    QVERIFY(reset);
    reset->click();
    QCOMPARE(dialog.toolbarLayout(), defaults);
}

void CoreTest::settingsDialogAppliesWithoutClosing()
{
    SettingsDialog dialog(
        QStringLiteral("system"), QStringLiteral("system"),
        QStringLiteral("top"), QStringLiteral("bottom"), 3,
        true, true, 256, 1, QStringLiteral("name"), true,
        false, false, 256, false, 512, 0,
        {}, {}, {}, {}, {}, {},
        QStringLiteral("scroll"),
        QStringLiteral("zoom"),
        QStringLiteral("toggle_zoom"),
        QStringLiteral("none"),
        QStringLiteral("previous"),
        QStringLiteral("next"));
    QSignalSpy applySpy(&dialog, &SettingsDialog::applyRequested);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    auto *applyButton = dialog.findChild<QPushButton *>(
        QStringLiteral("settingsApplyButton"));
    QVERIFY(applyButton);
    QTest::mouseClick(applyButton, Qt::LeftButton);
    QCOMPARE(applySpy.count(), 1);
    QVERIFY(dialog.isVisible());

    auto *buttonBox = dialog.findChild<QDialogButtonBox *>(
        QStringLiteral("settingsButtonBox"));
    QVERIFY(buttonBox);
    auto *okButton = buttonBox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);
    QTest::mouseClick(okButton, Qt::LeftButton);
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
}

void CoreTest::preferencesApplyRecolorsMenuBar()
{
    const QPalette originalSystemPalette = qApp->palette();
    const QString originalSystemStyle = qApp->style()->objectName();
    const QString originalOrganization =
        QCoreApplication::organizationName();
    const QString originalApplication =
        QCoreApplication::applicationName();
    const QSettings::Format settingsFormat =
        QSettings::defaultFormat();
    const QString originalSettingsPath =
        QStandardPaths::writableLocation(
            QStandardPaths::GenericConfigLocation);
    QTemporaryDir settingsDirectory;
    QVERIFY(settingsDirectory.isValid());
    QSettings::setPath(settingsFormat, QSettings::UserScope,
                       settingsDirectory.path());
    const auto restoreSettings = qScopeGuard(
        [originalOrganization, originalApplication,
         originalSettingsPath, settingsFormat] {
        QSettings().clear();
        QCoreApplication::setOrganizationName(originalOrganization);
        QCoreApplication::setApplicationName(originalApplication);
        QSettings::setPath(settingsFormat, QSettings::UserScope,
                           originalSettingsPath);
    });
    QCoreApplication::setOrganizationName(
        QStringLiteral("ClearveilTest"));
    QCoreApplication::setApplicationName(
        QStringLiteral("clearveil-theme-apply-test"));
    QSettings().clear();

    MainWindow window;
    window.show();
    bool darkApplied = false;
    bool lightApplied = false;
    bool systemApplied = false;
    bool liveSystemPaletteApplied = false;
    QTimer::singleShot(0, &window,
                       [&window, &darkApplied, &lightApplied,
                        &systemApplied, &liveSystemPaletteApplied,
                        originalSystemPalette,
                        originalSystemStyle] {
        auto *dialog = qobject_cast<SettingsDialog *>(
            QApplication::activeModalWidget());
        if (!dialog)
            return;
        auto *theme = dialog->findChild<QComboBox *>(
            QStringLiteral("settingsTheme"));
        auto *apply = dialog->findChild<QPushButton *>(
            QStringLiteral("settingsApplyButton"));
        if (!theme || !apply) {
            dialog->reject();
            return;
        }
        const auto selectTheme = [theme, apply](const QString &name) {
            const int index = theme->findData(name);
            if (index < 0)
                return false;
            theme->setCurrentIndex(index);
            apply->click();
            QCoreApplication::processEvents();
            return true;
        };
        if (selectTheme(QStringLiteral("dark"))) {
            const QPalette expected = BreezeTheme::palette(
                BreezeTheme::Variant::Dark);
            const QMenu *mainMenu = window.findChild<QMenu *>(
                QStringLiteral("mainMenu"));
            const QMenu *fileMenu = window.menuBar()->actions()
                .constFirst()->menu();
            darkApplied = window.menuBar()->palette()
                    .color(QPalette::Window)
                    == expected.color(QPalette::Window)
                && window.menuBar()->palette()
                    .color(QPalette::WindowText)
                    == expected.color(QPalette::WindowText)
                && mainMenu && fileMenu
                && mainMenu->palette().color(QPalette::WindowText)
                    == expected.color(QPalette::WindowText)
                && fileMenu->palette().color(QPalette::WindowText)
                    == expected.color(QPalette::WindowText);
        }
        if (selectTheme(QStringLiteral("light"))) {
            const QPalette expected = BreezeTheme::palette(
                BreezeTheme::Variant::Light);
            const QMenu *mainMenu = window.findChild<QMenu *>(
                QStringLiteral("mainMenu"));
            const QMenu *fileMenu = window.menuBar()->actions()
                .constFirst()->menu();
            lightApplied = window.menuBar()->palette()
                    .color(QPalette::Window)
                    == expected.color(QPalette::Window)
                && window.menuBar()->palette()
                    .color(QPalette::WindowText)
                    == expected.color(QPalette::WindowText)
                && mainMenu && fileMenu
                && mainMenu->palette().color(QPalette::WindowText)
                    == expected.color(QPalette::WindowText)
                && fileMenu->palette().color(QPalette::WindowText)
                    == expected.color(QPalette::WindowText);
        }
        if (selectTheme(QStringLiteral("system"))) {
            systemApplied = qApp->palette().color(QPalette::Window)
                    == originalSystemPalette.color(QPalette::Window)
                && qApp->palette().color(QPalette::Highlight)
                    == originalSystemPalette.color(QPalette::Highlight)
                && qApp->style()->objectName() == originalSystemStyle;
        }
        QPalette changedSystemPalette = originalSystemPalette;
        const QColor changedHighlight(43, 137, 255);
        changedSystemPalette.setColor(
            QPalette::Highlight, changedHighlight);
        qApp->setPalette(changedSystemPalette);
        QCoreApplication::processEvents();
        if (selectTheme(QStringLiteral("dark"))
            && selectTheme(QStringLiteral("system"))) {
            liveSystemPaletteApplied =
                qApp->palette().color(QPalette::Highlight)
                == changedHighlight;
        }
        qApp->setPalette(originalSystemPalette);
        QCoreApplication::processEvents();
        dialog->reject();
    });
    QVERIFY(QMetaObject::invokeMethod(
        &window, "showPreferences", Qt::DirectConnection));
    QVERIFY(darkApplied);
    QVERIFY(lightApplied);
    QVERIFY(systemApplied);
    QVERIFY(liveSystemPaletteApplied);
    QVERIFY(!window.styleSheet().contains(QStringLiteral("QMenuBar")));
    QVERIFY(!window.styleSheet().contains(
        QStringLiteral("QCheckBox::indicator")));
}

void CoreTest::thumbnailStripSettingsControlCachesAndLayout()
{
    SettingsDialog dialog(
        QStringLiteral("system"), QStringLiteral("system"),
        QStringLiteral("top"), QStringLiteral("bottom"), 3,
        true, false, 120, 3, QStringLiteral("size"), false,
        false, false, 320, false, 512, 3 * 1024 * 1024,
        {}, {}, {}, {}, {}, {},
        QStringLiteral("zoom"),
        QStringLiteral("zoom"),
        QStringLiteral("toggle_zoom"),
        QStringLiteral("none"),
        QStringLiteral("previous"),
        QStringLiteral("next"));

    auto *enabled = dialog.findChild<QCheckBox *>(
        QStringLiteral("persistentThumbnailCacheEnabled"));
    auto *memoryLimit = dialog.findChild<QSpinBox *>(
        QStringLiteral("imageMemoryCacheLimitMiB"));
    auto *limit = dialog.findChild<QSpinBox *>(
        QStringLiteral("persistentThumbnailCacheLimitMiB"));
    auto *usage = dialog.findChild<QLabel *>(
        QStringLiteral("persistentThumbnailCacheUsage"));
    auto *clear = dialog.findChild<QPushButton *>(
        QStringLiteral("clearPersistentThumbnailCacheButton"));
    auto *showNames = dialog.findChild<QCheckBox *>(
        QStringLiteral("filmstripShowFileNames"));
    auto *thumbnailExtent = dialog.findChild<QSpinBox *>(
        QStringLiteral("filmstripThumbnailExtent"));
    auto *verticalColumns = dialog.findChild<QSpinBox *>(
        QStringLiteral("filmstripVerticalColumns"));
    auto *sortKey = dialog.findChild<QComboBox *>(
        QStringLiteral("directoryThumbnailSortKey"));
    auto *sortDirection = dialog.findChild<QComboBox *>(
        QStringLiteral("directoryThumbnailSortDirection"));
    QVERIFY(enabled);
    QVERIFY(memoryLimit);
    QVERIFY(limit);
    QVERIFY(usage);
    QVERIFY(clear);
    QVERIFY(showNames);
    QVERIFY(thumbnailExtent);
    QVERIFY(verticalColumns);
    QVERIFY(sortKey);
    QVERIFY(sortDirection);
    QVERIFY(!dialog.showFilmstripFileNames());
    QCOMPARE(dialog.filmstripThumbnailExtent(), 120);
    QCOMPARE(dialog.filmstripVerticalColumns(), 3);
    QCOMPARE(dialog.directoryThumbnailSortKey(),
             QStringLiteral("size"));
    QVERIFY(!dialog.directoryThumbnailSortAscending());
    QVERIFY(!enabled->isChecked());
    QCOMPARE(memoryLimit->value(), 320);
    memoryLimit->setValue(640);
    QCOMPARE(dialog.imageMemoryCacheMiB(), 640);
    QVERIFY(!limit->isEnabled());
    QCOMPARE(limit->value(), 512);
    QVERIFY(usage->text().contains(QStringLiteral("3.00")));
    QVERIFY(!dialog.persistentThumbnailCacheEnabled());

    enabled->setChecked(true);
    QVERIFY(limit->isEnabled());
    limit->setValue(1024);
    QVERIFY(dialog.persistentThumbnailCacheEnabled());
    QCOMPARE(dialog.persistentThumbnailCacheMiB(), 1024);
}

void CoreTest::shortcutSettingsCanEditAndReset()
{
    const QList<QPair<QString, QString>> definitions{
        {QStringLiteral("alpha"), QStringLiteral("Alpha")},
        {QStringLiteral("beta"), QStringLiteral("Beta")}
    };
    const QStringList defaults{
        QStringLiteral("alpha\tCtrl+A"),
        QStringLiteral("beta\tCtrl+B")
    };
    SettingsDialog dialog(
        QStringLiteral("system"), QStringLiteral("system"),
        QStringLiteral("top"), QStringLiteral("bottom"), 3,
        true, true, 256, 1, QStringLiteral("name"), true,
        false, false, 256, false, 512, 0, {}, {}, {},
        definitions,
        {QStringLiteral("alpha\tCtrl+1"),
         QStringLiteral("beta\t")},
        defaults, QStringLiteral("zoom"),
        QStringLiteral("zoom"),
        QStringLiteral("toggle_zoom"),
        QStringLiteral("none"),
        QStringLiteral("previous"),
        QStringLiteral("next"));

    auto *table = dialog.findChild<QTableWidget *>(
        QStringLiteral("shortcutItems"));
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 2);

    auto *alpha = dialog.findChild<QKeySequenceEdit *>(
        QStringLiteral("shortcutEditor_alpha"));
    auto *beta = dialog.findChild<QKeySequenceEdit *>(
        QStringLiteral("shortcutEditor_beta"));
    QVERIFY(alpha);
    QVERIFY(beta);
    QCOMPARE(alpha->keySequence(),
             QKeySequence(QStringLiteral("Ctrl+1")));
    QVERIFY(beta->keySequence().isEmpty());

    alpha->setKeySequence(
        QKeySequence(QStringLiteral("Ctrl+Shift+A")));
    QCOMPARE(dialog.shortcutLayout().at(0),
             QStringLiteral("alpha\tCtrl+Shift+A"));

    auto *reset = dialog.findChild<QPushButton *>(
        QStringLiteral("shortcutResetButton"));
    QVERIFY(reset);
    reset->click();
    QCOMPARE(dialog.shortcutLayout(), defaults);

    auto *wheel = dialog.findChild<QComboBox *>(
        QStringLiteral("mouseWheelAction"));
    auto *doubleClick = dialog.findChild<QComboBox *>(
        QStringLiteral("mouseDoubleClickAction"));
    auto *ctrlWheel = dialog.findChild<QComboBox *>(
        QStringLiteral("mouseCtrlWheelAction"));
    QVERIFY(wheel);
    QVERIFY(ctrlWheel);
    QVERIFY(doubleClick);
    wheel->setCurrentIndex(
        wheel->findData(QStringLiteral("navigate")));
    doubleClick->setCurrentIndex(
        doubleClick->findData(QStringLiteral("fullscreen")));
    QCOMPARE(dialog.wheelAction(),
             QStringLiteral("navigate"));
    ctrlWheel->setCurrentIndex(
        ctrlWheel->findData(QStringLiteral("none")));
    QCOMPARE(dialog.ctrlWheelAction(),
             QStringLiteral("none"));
    QCOMPARE(dialog.doubleClickAction(),
             QStringLiteral("fullscreen"));

    auto *resetMouse = dialog.findChild<QPushButton *>(
        QStringLiteral("mouseResetButton"));
    QVERIFY(resetMouse);
    resetMouse->click();
    QCOMPARE(dialog.wheelAction(),
             QStringLiteral("scroll"));
    QCOMPARE(dialog.ctrlWheelAction(),
             QStringLiteral("zoom"));
    QCOMPARE(dialog.doubleClickAction(),
             QStringLiteral("toggle_zoom"));
    QCOMPARE(dialog.middleButtonAction(),
             QStringLiteral("none"));
    QCOMPARE(dialog.backButtonAction(),
             QStringLiteral("previous"));
    QCOMPARE(dialog.forwardButtonAction(),
             QStringLiteral("next"));
}

void CoreTest::mouseActionsEmitConfiguredCommands()
{
    ImageCanvas canvas;
    canvas.resize(500, 360);
    QImage image(1000, 800, QImage::Format_RGB32);
    image.fill(Qt::white);
    canvas.setImage(image);
    canvas.actualSize();
    canvas.setMouseActions(
        QStringLiteral("scroll"),
        QStringLiteral("zoom"),
        QStringLiteral("fullscreen"),
        QStringLiteral("next"),
        QStringLiteral("previous"),
        QStringLiteral("next"));
    QSignalSpy actions(
        &canvas, &ImageCanvas::mouseActionRequested);

    QWheelEvent verticalScroll(
        QPointF(canvas.rect().center()),
        QPointF(canvas.mapToGlobal(canvas.rect().center())),
        QPoint(), QPoint(0, -120), Qt::NoButton,
        Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(
        &canvas, &verticalScroll);
    QVERIFY(canvas.viewOffset().y() < 0);
    QCOMPARE(canvas.viewOffset().x(), 0.0);
    const qreal verticalOffset =
        canvas.viewOffset().y();

    QWheelEvent horizontalScroll(
        QPointF(canvas.rect().center()),
        QPointF(canvas.mapToGlobal(canvas.rect().center())),
        QPoint(), QPoint(0, -120), Qt::NoButton,
        Qt::ShiftModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(
        &canvas, &horizontalScroll);
    QVERIFY(canvas.viewOffset().x() < 0);
    QCOMPARE(canvas.viewOffset().y(),
             verticalOffset);

    const qreal horizontalOffset =
        canvas.viewOffset().x();
    QWheelEvent horizontalPixelScroll(
        QPointF(canvas.rect().center()),
        QPointF(canvas.mapToGlobal(canvas.rect().center())),
        QPoint(-18, 0), QPoint(), Qt::NoButton,
        Qt::ShiftModifier, Qt::ScrollUpdate, false);
    QCoreApplication::sendEvent(
        &canvas, &horizontalPixelScroll);
    QVERIFY(canvas.viewOffset().x() < horizontalOffset);
    QCOMPARE(canvas.viewOffset().y(), verticalOffset);

    canvas.fitToWindow();
    QCOMPARE(canvas.zoomMode(), ImageCanvas::ZoomMode::Fit);
    QCOMPARE(canvas.viewOffset(), QPointF());
    QWheelEvent fittedScroll(
        QPointF(canvas.rect().center()),
        QPointF(canvas.mapToGlobal(canvas.rect().center())),
        QPoint(), QPoint(0, -120), Qt::NoButton,
        Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(&canvas, &fittedScroll);
    QCOMPARE(canvas.viewOffset(), QPointF());
    QTest::mousePress(
        &canvas, Qt::LeftButton, Qt::NoModifier,
        canvas.rect().center());
    QTest::mouseMove(
        &canvas, canvas.rect().center() + QPoint(80, 60));
    QTest::mouseRelease(
        &canvas, Qt::LeftButton, Qt::NoModifier,
        canvas.rect().center() + QPoint(80, 60));
    QCOMPARE(canvas.viewOffset(), QPointF());

    const qreal zoomBeforeControlWheel =
        canvas.zoom();
    QWheelEvent controlZoom(
        QPointF(canvas.rect().center()),
        QPointF(canvas.mapToGlobal(canvas.rect().center())),
        QPoint(), QPoint(0, 120), Qt::NoButton,
        Qt::ControlModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(
        &canvas, &controlZoom);
    QVERIFY(canvas.zoom() > zoomBeforeControlWheel);

    canvas.setMouseActions(
        QStringLiteral("navigate"),
        QStringLiteral("zoom"),
        QStringLiteral("fullscreen"),
        QStringLiteral("next"),
        QStringLiteral("previous"),
        QStringLiteral("next"));
    QWheelEvent navigateWheel(
        QPointF(canvas.rect().center()),
        QPointF(canvas.mapToGlobal(canvas.rect().center())),
        QPoint(), QPoint(0, 120), Qt::NoButton,
        Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(
        &canvas, &navigateWheel);
    QCOMPARE(actions.count(), 1);
    QCOMPARE(actions.takeFirst().at(0).toString(),
             QStringLiteral("previous"));

    for (int eventIndex = 0; eventIndex < 3; ++eventIndex) {
        QWheelEvent preciseNavigate(
            QPointF(canvas.rect().center()),
            QPointF(canvas.mapToGlobal(canvas.rect().center())),
            QPoint(0, -15), QPoint(), Qt::NoButton,
            Qt::NoModifier, Qt::ScrollUpdate, false);
        QCoreApplication::sendEvent(
            &canvas, &preciseNavigate);
    }
    QCOMPARE(actions.count(), 0);
    QWheelEvent preciseNavigateThreshold(
        QPointF(canvas.rect().center()),
        QPointF(canvas.mapToGlobal(canvas.rect().center())),
        QPoint(0, -15), QPoint(), Qt::NoButton,
        Qt::NoModifier, Qt::ScrollEnd, false);
    QCoreApplication::sendEvent(
        &canvas, &preciseNavigateThreshold);
    QCOMPARE(actions.count(), 1);
    QCOMPARE(actions.takeFirst().at(0).toString(),
             QStringLiteral("next"));

    QTest::mouseDClick(&canvas, Qt::LeftButton,
                       Qt::NoModifier,
                       canvas.rect().center());
    QVERIFY(!actions.isEmpty());
    QCOMPARE(actions.takeLast().at(0).toString(),
             QStringLiteral("fullscreen"));

    QTest::mouseClick(&canvas, Qt::MiddleButton,
                      Qt::NoModifier,
                      canvas.rect().center());
    QVERIFY(!actions.isEmpty());
    QCOMPARE(actions.takeLast().at(0).toString(),
             QStringLiteral("next"));
}

void CoreTest::ocrTextSelectionPreservesReadingOrder()
{
    OcrResult result;
    result.imageSize = QSize(300, 120);
    result.symbols = {
        {QStringLiteral("示"), QRectF(10, 10, 20, 24), 0, 0, 98.0F},
        {QStringLiteral("例"), QRectF(32, 10, 20, 24), 0, 1, 98.0F},
        {QStringLiteral("O"), QRectF(64, 10, 14, 24), 0, 2, 97.0F},
        {QStringLiteral("C"), QRectF(80, 10, 14, 24), 0, 2, 97.0F},
        {QStringLiteral("R"), QRectF(96, 10, 14, 24), 0, 2, 97.0F},
        {QStringLiteral("4"), QRectF(10, 48, 14, 24), 1, 3, 96.0F},
        {QStringLiteral("2"), QRectF(26, 48, 14, 24), 1, 3, 96.0F}
    };

    OcrTextSelectionModel selection;
    QSignalSpy changed(
        &selection, &OcrTextSelectionModel::selectionChanged);
    selection.setResult(result);
    QVERIFY(selection.beginSelection(QPointF(34, 18)));
    QVERIFY(selection.updateSelection(QPointF(39, 56)));
    QCOMPARE(selection.selectedText(),
             QStringLiteral("例 OCR\n42"));
    const QVector<QRectF> selectedRows = selection.selectedBounds();
    QCOMPARE(selectedRows.size(), 2);
    QCOMPARE(selectedRows.at(0).top(), 10.0);
    QCOMPARE(selectedRows.at(0).height(), 24.0);
    QCOMPARE(selectedRows.at(1).top(), 48.0);
    QCOMPARE(selectedRows.at(1).height(), 24.0);
    QVERIFY(!changed.isEmpty());

    OcrResult unevenResult;
    unevenResult.imageSize = QSize(100, 40);
    unevenResult.symbols = {
        {QStringLiteral("A"), QRectF(10, 12, 10, 12), 0, 0, 99.0F},
        {QStringLiteral("B"), QRectF(30, 8, 10, 26), 0, 1, 99.0F},
        {QStringLiteral("C"), QRectF(50, 14, 10, 10), 0, 2, 99.0F}
    };
    selection.setResult(unevenResult);
    // OCR selection includes both characters touched by the drag endpoints;
    // unlike a text editor, an endpoint inside C must not silently omit C.
    QVERIFY(selection.beginSelection(QPointF(19, 18)));
    QVERIFY(selection.updateSelection(QPointF(51, 18)));
    QCOMPARE(selection.selectedText(), QStringLiteral("A B C"));
    const QVector<QRectF> unifiedRow = selection.selectedBounds();
    QCOMPARE(unifiedRow.size(), 1);
    QCOMPARE(unifiedRow.constFirst(), QRectF(10, 8, 50, 26));

    OcrResult overlappingResult;
    overlappingResult.imageSize = QSize(80, 40);
    overlappingResult.symbols = {
        {QStringLiteral("M"), QRectF(10, 8, 10, 24), 0, 0, 99.0F},
        {QStringLiteral("i"), QRectF(20, 8, 10, 24), 0, 0, 99.0F},
        // Some Tesseract builds return a word-wide box for one symbol.
        {QStringLiteral("f"), QRectF(10, 8, 40, 24), 0, 0, 99.0F},
        {QStringLiteral("t"), QRectF(40, 8, 10, 24), 0, 0, 99.0F}
    };
    selection.setResult(overlappingResult);
    QVERIFY(selection.beginSelection(QPointF(31, 18)));
    QVERIFY(selection.updateSelection(QPointF(39, 18)));
    QCOMPARE(selection.selectedText(), QStringLiteral("f"));
    const QVector<QRectF> repairedSelection = selection.selectedBounds();
    QCOMPARE(repairedSelection.size(), 1);
    QCOMPARE(repairedSelection.constFirst(), QRectF(30, 8, 10, 24));

    OcrResult overlappingLines;
    overlappingLines.imageSize = QSize(120, 80);
    overlappingLines.symbols = {
        {QStringLiteral("-"), QRectF(0, 12, 100, 3), 0, 0, 15.0F},
        {QStringLiteral("应"), QRectF(20, 14, 24, 30), 1, 1, 96.0F},
        {QStringLiteral("用"), QRectF(48, 14, 24, 30), 1, 2, 96.0F}
    };
    selection.setResult(overlappingLines);
    QVERIFY(selection.beginSelection(QPointF(32, 28)));
    QVERIFY(selection.updateSelection(QPointF(60, 28)));
    QCOMPARE(selection.selectedText(), QStringLiteral("应用"));

    OcrResult overlappingCjkWords;
    overlappingCjkWords.imageSize = QSize(360, 140);
    overlappingCjkWords.symbols = {
        {QStringLiteral("应"), QRectF(150, 47, 36, 80), 0, 5, 96.0F},
        {QStringLiteral("用"), QRectF(214, 47, 6, 80), 0, 6, 96.0F},
        {QStringLiteral("程"), QRectF(229, 47, 73, 80), 0, 7, 96.0F},
        {QStringLiteral("序"), QRectF(283, 47, 24, 80), 0, 8, 96.0F}
    };
    selection.setResult(overlappingCjkWords);
    QVERIFY(selection.beginSelection(QPointF(168, 80)));
    QVERIFY(selection.updateSelection(QPointF(295, 80)));
    QCOMPARE(selection.selectedText(), QStringLiteral("应用程序"));
    QVERIFY(selection.beginSelection(QPointF(250, 80)));
    QVERIFY(selection.updateSelection(QPointF(295, 80)));
    QCOMPARE(selection.selectedText(), QStringLiteral("程序"));
    QVERIFY(selection.beginSelection(QPointF(250, 80)));
    QVERIFY(selection.updateSelection(QPointF(250, 80)));
    QCOMPARE(selection.selectedText(), QStringLiteral("程"));
    QVERIFY(selection.beginSelection(QPointF(295, 80)));
    QVERIFY(selection.updateSelection(QPointF(295, 80)));
    QCOMPARE(selection.selectedText(), QStringLiteral("序"));

    selection.setResult(result);
    QVERIFY(selection.selectWordAt(QPointF(82, 18)));
    QCOMPARE(selection.selectedText(), QStringLiteral("OCR"));
    selection.selectAll();
    QCOMPARE(selection.selectedText(),
             QStringLiteral("示例 OCR\n42"));
    selection.clearSelection();
    QVERIFY(!selection.hasSelection());
}

void CoreTest::ocrFallbackDetectorBudgetsSuspiciousTextRows()
{
    QImage image(1200, 600, QImage::Format_RGB32);
    image.fill(QColor(202, 241, 255));
    QPainter painter(&image);
    QFont font(QStringLiteral("Noto Sans CJK SC"));
    font.setPixelSize(38);
    painter.setFont(font);
    painter.setPen(QColor(40, 65, 75));
    painter.drawText(QRect(80, 40, 500, 70),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("示例 Clearveil"));
    painter.setPen(QColor(149, 178, 185));
    painter.drawText(QRect(80, 220, 680, 70),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("工具  效率与财务  社交"));
    painter.end();

    const QVector<OcrSymbol> primarySymbols{
        {QStringLiteral("示例 Clearveil"), QRectF(80, 40, 500, 70),
         0, 0, 98.0F}
    };
    const OcrFallbackPlan plan = OcrFallbackDetector::plan(
        image, image.size(), primarySymbols);
    QVERIFY(plan.analysisSize.width()
            <= OcrFallbackDetector::maximumAnalysisDimension);
    QVERIFY(plan.analysisSize.height()
            <= OcrFallbackDetector::maximumAnalysisDimension);
    QVERIFY(plan.inspectedPixels
            <= static_cast<qsizetype>(
                OcrFallbackDetector::maximumAnalysisDimension)
                * OcrFallbackDetector::maximumAnalysisDimension
                * OcrFallbackDetector::maximumAnalysisPasses);
    QVERIFY(plan.regions.size()
            <= OcrFallbackDetector::maximumRegionCount);
    QVERIFY(plan.selectedPixels
            <= static_cast<qsizetype>(image.width()) * image.height()
                * OcrFallbackDetector::maximumRegionAreaRatio);
    QStringList plannedRegions;
    for (const QRect &region : plan.regions) {
        plannedRegions.append(QStringLiteral("%1,%2 %3x%4")
                                  .arg(region.x()).arg(region.y())
                                  .arg(region.width()).arg(region.height()));
    }
    QVERIFY2(std::any_of(
                 plan.regions.cbegin(), plan.regions.cend(),
                 [](const QRect &region) {
                     return region.intersects(QRect(80, 220, 680, 70));
                 }),
             qPrintable(plannedRegions.join(QStringLiteral("; "))));
    QVERIFY(std::none_of(
        plan.regions.cbegin(), plan.regions.cend(),
        [](const QRect &region) {
            return region.contains(QPoint(250, 75));
        }));
}

void CoreTest::ocrEngineRecognizesRenderedText()
{
    QCOMPARE(OcrEngine::recognitionLanguages(
                 {QStringLiteral("eng"), QStringLiteral("deu")}),
             QStringLiteral("deu+eng"));
    QCOMPARE(OcrEngine::recognitionLanguages(
                 {QStringLiteral("eng"), QStringLiteral("chi_sim")}),
             QStringLiteral("chi_sim+eng"));
    QCOMPARE(OcrEngine::recognitionLanguages(
                 {QStringLiteral("eng"), QStringLiteral("chi_sim"),
                  QStringLiteral("chi_tra")}),
             QStringLiteral("chi_sim+chi_tra+eng"));
    QCOMPARE(OcrEngine::recognitionLanguages(
                 {QStringLiteral("osd"), QStringLiteral("eng"),
                  QStringLiteral("jpn")}),
             QStringLiteral("eng+jpn"));
    if (!OcrEngine::isAvailable())
        QSKIP("Tesseract was not available at build time");
    const QStringList available = OcrEngine::availableLanguages();
    if (!available.contains(QStringLiteral("eng")))
        QSKIP("English Tesseract language data is not installed");

    QImage image(1200, 300, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QFont font(QStringLiteral("DejaVu Sans"));
    font.setPixelSize(120);
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(image.rect().adjusted(40, 20, -40, -20),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     QStringLiteral("CLEARVEIL 42"));
    painter.end();

    const OcrResult result = OcrEngine::recognize(
        image, image.size(), QStringLiteral("eng"));
    QVERIFY2(result.succeeded(),
             qPrintable(result.error));
    QVERIFY(!result.symbols.isEmpty());
    QString recognized;
    for (const OcrSymbol &symbol : result.symbols)
        recognized.append(symbol.text);
    QVERIFY(recognized.contains(QStringLiteral("CLEARVEIL"),
                                Qt::CaseInsensitive));
    QVERIFY(recognized.contains(QStringLiteral("42")));
    QCOMPARE(result.fallbackRegionCount, 0);
    QVERIFY(std::none_of(
        result.symbols.cbegin(), result.symbols.cend(),
        [](const OcrSymbol &symbol) {
            return symbol.supplemental;
        }));
    for (int index = 0; index < result.symbols.size(); ++index) {
        const OcrSymbol &symbol = result.symbols.at(index);
        QVERIFY(QRectF(image.rect()).contains(symbol.bounds));
        if (index > 0
            && result.symbols.at(index - 1).lineIndex
                == symbol.lineIndex) {
            QCOMPARE(symbol.bounds.top(),
                     result.symbols.at(index - 1).bounds.top());
            QCOMPARE(symbol.bounds.height(),
                     result.symbols.at(index - 1).bounds.height());
        }
    }
}

void CoreTest::ocrEngineRecognizesInstalledChineseModel()
{
    if (!OcrEngine::isAvailable())
        QSKIP("Tesseract was not available at build time");
    const QStringList available = OcrEngine::availableLanguages();
    if (!available.contains(QStringLiteral("chi_sim")))
        QSKIP("Simplified Chinese Tesseract data is not installed");

    QFont font(QStringLiteral("Noto Sans CJK SC"));
    font.setPixelSize(150);
    font.setWeight(QFont::Bold);
    const QFontMetrics metrics(font);
    if (!metrics.inFontUcs4(0x793A))
        QSKIP("No font capable of rendering the Chinese fixture is installed");

    QImage image(1400, 360, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(image.rect().adjusted(40, 20, -40, -20),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     QStringLiteral("示例中文"));
    painter.end();

    const QString languages = OcrEngine::recognitionLanguages(available);
    QVERIFY(languages.split(QLatin1Char('+')).contains(
        QStringLiteral("chi_sim")));
    const OcrResult result = OcrEngine::recognize(
        image, image.size(), languages);
    QVERIFY2(result.succeeded(), qPrintable(result.error));
    bool foundHanCharacter = false;
    for (const OcrSymbol &symbol : result.symbols) {
        for (const QChar character : symbol.text) {
            const ushort value = character.unicode();
            if (value >= 0x4E00 && value <= 0x9FFF) {
                foundHanCharacter = true;
                break;
            }
        }
    }
    QVERIFY2(foundHanCharacter,
             "The installed Chinese model produced no selectable Han symbols");
}

void CoreTest::ocrCanvasSupportsMouseSelection()
{
    ImageCanvas canvas;
    canvas.resize(400, 300);
    QImage image(200, 100, QImage::Format_RGB32);
    image.fill(Qt::white);
    canvas.setImage(image);
    canvas.fitToWindow();

    OcrResult result;
    result.imageSize = image.size();
    result.symbols = {
        {QStringLiteral("A"), QRectF(10, 10, 20, 20), 0, 0, 99.0F},
        {QStringLiteral("B"), QRectF(35, 10, 20, 20), 0, 0, 99.0F}
    };
    canvas.setOcrResult(result);
    canvas.setOcrTextSelectionEnabled(true);
    canvas.setOcrDebugOverlayEnabled(true);
    QVERIFY(canvas.ocrDebugOverlayEnabled());
    canvas.setOcrDebugOverlayEnabled(false);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    const QPoint first(121, 120);
    const QPoint second(153, 120);
    QMouseEvent hoverEvent(
        QEvent::MouseMove, QPointF(first),
        QPointF(canvas.mapToGlobal(first)), Qt::NoButton,
        Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&canvas, &hoverEvent);
    QCOMPARE(canvas.cursor().shape(), Qt::IBeamCursor);
    QTest::mousePress(&canvas, Qt::LeftButton,
                      Qt::NoModifier, first);
    QTest::mouseMove(&canvas, second);
    QTest::mouseRelease(&canvas, Qt::LeftButton,
                        Qt::NoModifier, second);
    QVERIFY(canvas.hasSelectedText());
    QCOMPARE(canvas.selectedText(), QStringLiteral("AB"));
    QCOMPARE(canvas.viewOffset(), QPointF());

    QTest::keyClick(&canvas, Qt::Key_Escape);
    QVERIFY(!canvas.hasSelectedText());

    canvas.setZoom(3.0);
    const QPointF textModeOffset = canvas.viewOffset();
    QTest::mousePress(&canvas, Qt::LeftButton,
                      Qt::NoModifier, QPoint(320, 250));
    QTest::mouseMove(&canvas, QPoint(220, 250));
    QTest::mouseRelease(&canvas, Qt::LeftButton,
                        Qt::NoModifier, QPoint(220, 250));
    QCOMPARE(canvas.viewOffset(), textModeOffset);

    canvas.setOcrTextSelectionEnabled(false);
    QTest::mousePress(&canvas, Qt::LeftButton,
                      Qt::NoModifier, QPoint(320, 250));
    QTest::mouseMove(&canvas, QPoint(220, 250));
    QTest::mouseRelease(&canvas, Qt::LeftButton,
                        Qt::NoModifier, QPoint(220, 250));
    QVERIFY(canvas.viewOffset() != textModeOffset);

    const QString realFixture = qEnvironmentVariable(
        "CLEARVEIL_OCR_REAL_FIXTURE");
    if (!realFixture.isEmpty()) {
        const QImage fixture(realFixture);
        QVERIFY2(!fixture.isNull(), qPrintable(realFixture));
        const QString languages = OcrEngine::recognitionLanguages();
        const OcrResult recognized = OcrEngine::recognize(
            fixture, fixture.size(), languages);
        QVERIFY2(recognized.succeeded(), qPrintable(recognized.error));
        QVERIFY(recognized.fallbackRegionCount >= 1);
        QVERIFY(recognized.fallbackRegionCount
                <= OcrFallbackDetector::maximumRegionCount);
        QVERIFY(recognized.fallbackPixelCount
                <= static_cast<qsizetype>(fixture.width())
                    * fixture.height()
                    * OcrFallbackDetector::maximumRegionAreaRatio);

        const auto findSymbol = [&recognized](
                                    const QString &text, int after) {
            for (int index = std::max(0, after);
                 index < recognized.symbols.size(); ++index) {
                if (recognized.symbols.at(index).text == text)
                    return index;
            }
            return -1;
        };
        const int applicationFirst = findSymbol(QStringLiteral("应"), 0);
        const int applicationLast = findSymbol(
            QStringLiteral("用"), applicationFirst + 1);
        const int applicationProgramFirst = findSymbol(
            QStringLiteral("程"), applicationLast + 1);
        const int applicationProgramLast = findSymbol(
            QStringLiteral("序"), applicationProgramFirst + 1);
        const int dictionaryFirst = findSymbol(
            QStringLiteral("词"), applicationProgramLast + 1);
        const int dictionaryLast = findSymbol(
            QStringLiteral("典"), dictionaryFirst + 1);
        const int podcastFirst = findSymbol(
            QStringLiteral("播"), applicationProgramLast + 1);
        const int podcastLast = findSymbol(
            QStringLiteral("客"), podcastFirst + 1);
        QString recognizedText;
        for (const OcrSymbol &symbol : recognized.symbols)
            recognizedText.append(symbol.text);
        QVERIFY(applicationFirst >= 0);
        QVERIFY(applicationLast >= 0);
        QVERIFY(applicationProgramFirst >= 0);
        QVERIFY(applicationProgramLast >= 0);
        QVERIFY2(dictionaryFirst >= 0,
                 qPrintable(QStringLiteral("languages=%1 text=%2")
                                .arg(languages, recognizedText)));
        QVERIFY2(dictionaryLast >= 0,
                 qPrintable(QStringLiteral("languages=%1 text=%2")
                                .arg(languages, recognizedText)));
        QVERIFY2(podcastFirst >= 0,
                 qPrintable(QStringLiteral("languages=%1 text=%2")
                                .arg(languages, recognizedText)));
        QVERIFY2(podcastLast >= 0,
                 qPrintable(QStringLiteral("languages=%1 text=%2")
                                .arg(languages, recognizedText)));
        QVERIFY(recognized.refinementPassCount
                <= OcrEngine::maximumRefinementPasses);
        QVERIFY(recognized.refinementPixelCount
                <= static_cast<qsizetype>(fixture.width())
                    * fixture.height());
        const QStringList lowContrastLabels{
            QStringLiteral("工具"),
            QStringLiteral("效率与财务"),
            QStringLiteral("社交"),
            QStringLiteral("创意"),
            QStringLiteral("娱乐"),
            QStringLiteral("信息与阅读"),
            QStringLiteral("其他")
        };
        for (const QString &label : lowContrastLabels) {
            QVERIFY2(recognizedText.contains(label),
                     qPrintable(QStringLiteral(
                         "Missing low-contrast label '%1': %2")
                                    .arg(label, recognizedText)));
        }
        QVERIFY(std::any_of(
            recognized.symbols.cbegin(), recognized.symbols.cend(),
            [](const OcrSymbol &symbol) {
                return symbol.supplemental
                    && symbol.bounds.center().y() >= 140.0
                    && symbol.bounds.center().y() <= 210.0;
            }));

        canvas.resize(fixture.size());
        canvas.setImage(fixture);
        canvas.actualSize();
        canvas.setOcrResult(recognized);
        canvas.setOcrTextSelectionEnabled(true);
        QCoreApplication::processEvents();
        const auto widgetPoint = [&recognized](int index) {
            return recognized.symbols.at(index).bounds.center().toPoint();
        };
        const auto dragAndCopy = [&canvas](
                                     const QPoint &first,
                                     const QPoint &last) {
            QTest::mousePress(&canvas, Qt::LeftButton,
                              Qt::NoModifier, first);
            QTest::mouseMove(&canvas, last);
            QTest::mouseRelease(&canvas, Qt::LeftButton,
                                Qt::NoModifier, last);
            QApplication::clipboard()->setText(canvas.selectedText());
            return QApplication::clipboard()->text();
        };
        QCOMPARE(dragAndCopy(widgetPoint(applicationFirst),
                             widgetPoint(applicationLast)),
                 QStringLiteral("应用"));
        QTest::keyClick(&canvas, Qt::Key_Escape);
        QCOMPARE(dragAndCopy(widgetPoint(applicationFirst),
                             widgetPoint(applicationProgramLast)),
                 QStringLiteral("应用程序"));
        QTest::keyClick(&canvas, Qt::Key_Escape);
        QCOMPARE(dragAndCopy(widgetPoint(applicationProgramFirst),
                             widgetPoint(applicationProgramLast)),
                 QStringLiteral("程序"));
        QTest::keyClick(&canvas, Qt::Key_Escape);
        QCOMPARE(dragAndCopy(widgetPoint(applicationProgramFirst),
                             widgetPoint(applicationProgramFirst)),
                 QStringLiteral("程"));
        QTest::keyClick(&canvas, Qt::Key_Escape);
        QCOMPARE(dragAndCopy(widgetPoint(applicationProgramLast),
                             widgetPoint(applicationProgramLast)),
                 QStringLiteral("序"));
        QTest::keyClick(&canvas, Qt::Key_Escape);
        QCOMPARE(dragAndCopy(widgetPoint(dictionaryFirst),
                             widgetPoint(dictionaryLast)),
                 QStringLiteral("词典"));
        QTest::keyClick(&canvas, Qt::Key_Escape);
        QCOMPARE(dragAndCopy(widgetPoint(applicationFirst),
                             widgetPoint(applicationFirst)),
                 QStringLiteral("应"));
        QTest::keyClick(&canvas, Qt::Key_Escape);
        QCOMPARE(dragAndCopy(widgetPoint(applicationLast),
                             widgetPoint(applicationLast)),
                 QStringLiteral("用"));
    }
}

void CoreTest::mainWindowOcrWorkflowCopiesRecognizedText()
{
    if (!OcrEngine::isAvailable()
        || !OcrEngine::availableLanguages().contains(
            QStringLiteral("eng"))) {
        QSKIP("English Tesseract OCR is not available");
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(
        QStringLiteral("ocr-workflow.png"));
    QImage image(1200, 300, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QFont font(QStringLiteral("DejaVu Sans"));
    font.setPixelSize(120);
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(image.rect().adjusted(40, 20, -40, -20),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     QStringLiteral("CLEARVEIL OCR"));
    painter.end();
    QVERIFY(image.save(path));

    MainWindow window;
    window.show();
    QVERIFY(window.openPath(path));
    auto *action = window.findChild<QAction *>(
        QStringLiteral("textSelectionToolAction"));
    auto *handAction = window.findChild<QAction *>(
        QStringLiteral("handToolAction"));
    auto *colorAction = window.findChild<QAction *>(
        QStringLiteral("colorPickerAction"));
    auto *canvas = window.findChild<ImageCanvas *>();
    QVERIFY(action);
    QVERIFY(handAction);
    QVERIFY(colorAction);
    QVERIFY(canvas);
    QVERIFY(handAction->isChecked());
    QTRY_VERIFY_WITH_TIMEOUT(action->isEnabled(), 5000);
    action->trigger();
    QVERIFY(action->isChecked());
    QVERIFY(!handAction->isChecked());
    QTRY_VERIFY_WITH_TIMEOUT(canvas->hasOcrText(), 10'000);
    QVERIFY(canvas->ocrTextSelectionEnabled());

    canvas->setFocus();
    QTest::keyClick(canvas, Qt::Key_A, Qt::ControlModifier);
    QVERIFY(canvas->hasSelectedText());
    QVERIFY(QMetaObject::invokeMethod(
        &window, "copyImage", Qt::DirectConnection));
    QVERIFY(QApplication::clipboard()->text().contains(
        QStringLiteral("CLEARVEIL"), Qt::CaseInsensitive));

    colorAction->trigger();
    QVERIFY(colorAction->isChecked());
    QVERIFY(!action->isChecked());
    QVERIFY(!canvas->ocrTextSelectionEnabled());
    handAction->trigger();
    QVERIFY(handAction->isChecked());
    QVERIFY(!colorAction->isChecked());
}

void CoreTest::canvasGesturesZoomAndPanIncrementally()
{
    CanvasGestureController gestures;
    QVERIFY(!gestures.isTouchGestureActive());
    QVERIFY(gestures.beginTouchGesture(
        QPointF(0, 0), QPointF(100, 0)));
    const auto touchUpdate = gestures.updateTouchGesture(
        QPointF(10, 10), QPointF(130, 10));
    QVERIFY(touchUpdate.has_value());
    QCOMPARE(touchUpdate->zoomFactor, 1.2);
    QCOMPARE(touchUpdate->anchor, QPointF(70, 10));
    QCOMPARE(touchUpdate->panDelta, QPointF(20, 10));
    gestures.endTouchGesture();
    QVERIFY(!gestures.isTouchGestureActive());
    QCOMPARE(CanvasGestureController::nativeZoomFactor(0.25),
             1.25);
    QCOMPARE(CanvasGestureController::nativeZoomFactor(-0.2),
             0.8);

    ImageCanvas canvas;
    canvas.resize(500, 360);
    QImage image(1000, 800, QImage::Format_RGB32);
    image.fill(Qt::white);
    canvas.setImage(image);
    canvas.actualSize();
    const QPointF anchor(canvas.rect().center());
    const QPointF globalAnchor = canvas.mapToGlobal(
        anchor.toPoint());
    QNativeGestureEvent zoomGesture(
        Qt::ZoomNativeGesture,
        QPointingDevice::primaryPointingDevice(), 2,
        anchor, anchor, globalAnchor, 0.2, QPointF());
    QVERIFY(QCoreApplication::sendEvent(&canvas, &zoomGesture));
    QVERIFY(zoomGesture.isAccepted());
    QCOMPARE(canvas.zoom(), 1.2);

    const QPointF offsetBeforePan = canvas.viewOffset();
    QNativeGestureEvent panGesture(
        Qt::PanNativeGesture,
        QPointingDevice::primaryPointingDevice(), 3,
        anchor, anchor, globalAnchor, 0.0,
        QPointF(24, -18));
    QVERIFY(QCoreApplication::sendEvent(&canvas, &panGesture));
    QVERIFY(panGesture.isAccepted());
    QCOMPARE(canvas.viewOffset(),
             offsetBeforePan + QPointF(24, -18));
}

void CoreTest::viewerStatusExposesAccessibleNames()
{
    MainWindow window;
    const struct {
        const char *objectName;
        const char *accessibleName;
    } labels[] = {
        {"currentFrameLabel", "Current frame"},
        {"currentFileLabel", "Current image"},
        {"imageDetailsLabel", "Image details"},
        {"zoomLevelLabel", "Zoom level"},
    };

    for (const auto &expected : labels) {
        auto *label = window.findChild<QLabel *>(
            QString::fromLatin1(expected.objectName));
        QVERIFY2(label, expected.objectName);
        QCOMPARE(label->accessibleName(),
                 QString::fromLatin1(expected.accessibleName));
        QVERIFY(label->textInteractionFlags()
                & Qt::TextSelectableByMouse);
        QVERIFY(label->textInteractionFlags()
                & Qt::TextSelectableByKeyboard);
    }

    auto *messageLabel = window.findChild<QLabel *>(
        QStringLiteral("statusMessageLabel"));
    QVERIFY(messageLabel);
    QCOMPARE(messageLabel->accessibleName(),
             QStringLiteral("Status message"));
    window.statusBar()->showMessage(
        QStringLiteral("Copyable status message"));
    QCOMPARE(window.statusBar()->currentMessage(),
             QStringLiteral("Copyable status message"));
    QVERIFY(!messageLabel->isHidden());
    messageLabel->setSelection(0, messageLabel->text().size());
    QApplication::clipboard()->clear();
    QTest::keyClick(messageLabel, Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(QApplication::clipboard()->text(),
             QStringLiteral("Copyable status message"));
    window.statusBar()->clearMessage();
    QVERIFY(messageLabel->isHidden());
}

void CoreTest::windowModesCanCombineAndFitImage()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath =
        directory.filePath(QStringLiteral("window-fit.png"));
    QImage image(900, 520, QImage::Format_RGB32);
    image.fill(QColor(70, 120, 210));
    QVERIFY(image.save(imagePath));

    MainWindow window;
    window.resize(1120, 760);
    window.show();
    QVERIFY(window.openPath(imagePath));
    QCoreApplication::processEvents();

    auto *fitWindowAction = window.findChild<QAction *>(
        QStringLiteral("fitWindowToImageAction"));
    auto *borderlessAction = window.findChild<QAction *>(
        QStringLiteral("borderlessAction"));
    auto *alwaysOnTopAction = window.findChild<QAction *>(
        QStringLiteral("alwaysOnTopAction"));
    QVERIFY(fitWindowAction);
    QVERIFY(borderlessAction);
    QVERIFY(alwaysOnTopAction);
    QVERIFY(fitWindowAction->isCheckable());
    QVERIFY(borderlessAction->isCheckable());
    QVERIFY(alwaysOnTopAction->isCheckable());

    const QSize originalSize = window.size();
    fitWindowAction->setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(window.size() != originalSize, 1000);
    QVERIFY(window.size().width() >= window.minimumWidth());
    QVERIFY(window.size().height() >= window.minimumHeight());
    QVERIFY(window.size().width()
            <= std::max(window.minimumWidth(),
                        window.screen()->availableGeometry().width()));
    QVERIFY(window.size().height()
            <= std::max(window.minimumHeight(),
                        window.screen()->availableGeometry().height()));

    borderlessAction->setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(
        window.windowFlags().testFlag(Qt::FramelessWindowHint),
        1000);
    alwaysOnTopAction->setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(
        window.windowFlags().testFlag(Qt::WindowStaysOnTopHint),
        1000);
    QVERIFY(window.windowFlags().testFlag(
        Qt::FramelessWindowHint));
    QVERIFY(window.isVisible());

    borderlessAction->setChecked(false);
    alwaysOnTopAction->setChecked(false);
    QTRY_VERIFY_WITH_TIMEOUT(
        !window.windowFlags().testFlag(Qt::FramelessWindowHint),
        1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        !window.windowFlags().testFlag(Qt::WindowStaysOnTopHint),
        1000);
}

void CoreTest::fullscreenComponentsHideAndRestore()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath =
        directory.filePath(QStringLiteral("fullscreen.png"));
    QImage image(640, 420, QImage::Format_RGB32);
    image.fill(QColor(44, 92, 170));
    QVERIFY(image.save(imagePath));

    MainWindow window;
    window.show();
    QVERIFY(window.openPath(imagePath));
    QCoreApplication::processEvents();

    auto *toolbar = window.findChild<QToolBar *>(
        QStringLiteral("mainToolbar"));
    auto *filmstrip = window.findChild<QDockWidget *>(
        QStringLiteral("filmstripDock"));
    auto *information = window.findChild<QDockWidget *>(
        QStringLiteral("metadataDock"));
    auto *cornerMenu = window.findChild<QToolButton *>(
        QStringLiteral("cornerMenuButton"));
    auto *fullscreen = window.findChild<QAction *>(
        QStringLiteral("fullscreenAction"));
    auto *filmstripVisibility = window.findChild<QAction *>(
        QStringLiteral("filmstripAction"));
    auto *showToolbar = window.findChild<QAction *>(
        QStringLiteral("fullscreenToolbarAction"));
    auto *showFilmstrip = window.findChild<QAction *>(
        QStringLiteral("fullscreenFilmstripAction"));
    auto *showStatus = window.findChild<QAction *>(
        QStringLiteral("fullscreenStatusBarAction"));
    auto *showInformation = window.findChild<QAction *>(
        QStringLiteral("fullscreenInformationAction"));
    QVERIFY(toolbar);
    QVERIFY(filmstrip);
    QVERIFY(information);
    QVERIFY(cornerMenu);
    QVERIFY(fullscreen);
    QVERIFY(filmstripVisibility);
    QVERIFY(showToolbar);
    QVERIFY(showFilmstrip);
    QVERIFY(showStatus);
    QVERIFY(showInformation);

    information->show();
    filmstripVisibility->setChecked(true);
    filmstrip->show();
    showToolbar->setChecked(false);
    showFilmstrip->setChecked(false);
    showStatus->setChecked(false);
    showInformation->setChecked(false);
    QTRY_VERIFY_WITH_TIMEOUT(toolbar->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(filmstrip->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(information->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->isVisible(), 1000);

    fullscreen->trigger();
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 1000);
    QVERIFY(!toolbar->isVisible());
    QVERIFY(!filmstrip->isVisible());
    QVERIFY(!information->isVisible());
    QVERIFY(!window.statusBar()->isVisible());
    QVERIFY(cornerMenu->isVisible());

    showToolbar->setChecked(true);
    showStatus->setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(toolbar->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        window.statusBar()->isVisible(), 1000);

    fullscreen->trigger();
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(toolbar->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(filmstrip->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(information->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->isVisible(), 1000);
}

void CoreTest::windowModeControllerOwnsPresentationState()
{
    QMainWindow window;
    auto *content = new QWidget(&window);
    window.setCentralWidget(content);
    auto *toolbar = window.addToolBar(QStringLiteral("Toolbar"));
    auto *filmstrip = new QDockWidget(&window);
    filmstrip->setWidget(new QWidget(filmstrip));
    window.addDockWidget(Qt::BottomDockWidgetArea, filmstrip);
    auto *information = new QDockWidget(&window);
    information->setWidget(new QWidget(information));
    window.addDockWidget(Qt::RightDockWidgetArea, information);

    QAction fullscreen(&window);
    QAction fitWindow(&window);
    QAction borderless(&window);
    QAction alwaysOnTop(&window);
    QAction fullscreenToolbar(&window);
    QAction fullscreenFilmstrip(&window);
    QAction fullscreenStatus(&window);
    QAction fullscreenInformation(&window);
    for (QAction *action : {&fullscreen, &fitWindow, &borderless,
                            &alwaysOnTop, &fullscreenToolbar,
                            &fullscreenFilmstrip, &fullscreenStatus,
                            &fullscreenInformation}) {
        action->setCheckable(true);
    }

    WindowModeController controller(
        {&window, content, toolbar, filmstrip,
         information, window.statusBar()},
        {&fullscreen, &fitWindow, &borderless, &alwaysOnTop,
         &fullscreenToolbar, &fullscreenFilmstrip,
         &fullscreenStatus, &fullscreenInformation});
    QSignalSpy presentationChanged(
        &controller, &WindowModeController::presentationChanged);

    window.setMinimumSize(200, 160);
    window.resize(760, 520);
    window.show();
    QCoreApplication::processEvents();
    QVERIFY(toolbar->isVisible());
    QVERIFY(filmstrip->isVisible());
    QVERIFY(information->isVisible());
    QVERIFY(window.statusBar()->isVisible());

    controller.toggleFullscreen();
    QTRY_VERIFY_WITH_TIMEOUT(window.isFullScreen(), 1000);
    QVERIFY(fullscreen.isChecked());
    QVERIFY(!toolbar->isVisible());
    QVERIFY(!filmstrip->isVisible());
    QVERIFY(!information->isVisible());
    QVERIFY(!window.statusBar()->isVisible());
    QVERIFY(!controller.isApplyingComponentVisibility());

    fullscreenToolbar.setChecked(true);
    fullscreenStatus.setChecked(true);
    controller.applyFullscreenComponentVisibility();
    QVERIFY(toolbar->isVisible());
    QVERIFY(window.statusBar()->isVisible());

    controller.toggleFullscreen();
    QTRY_VERIFY_WITH_TIMEOUT(!window.isFullScreen(), 1000);
    QVERIFY(!fullscreen.isChecked());
    QVERIFY(toolbar->isVisible());
    QVERIFY(filmstrip->isVisible());
    QVERIFY(information->isVisible());
    QVERIFY(window.statusBar()->isVisible());
    QVERIFY(presentationChanged.count() >= 3);

    fitWindow.setChecked(true);
    const QSize beforeFit = window.size();
    controller.scheduleFitWindowToContent(QSize(310, 190));
    controller.scheduleFitWindowToContent(QSize(470, 280));
    QTRY_VERIFY_WITH_TIMEOUT(window.size() != beforeFit, 1000);
    QVERIFY(window.size().width() >= 470);
    QVERIFY(window.size().height() >= 280);

    borderless.setChecked(true);
    alwaysOnTop.setChecked(true);
    controller.applyWindowModeFlags();
    QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(window.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
    QVERIFY(window.isVisible());
}

void CoreTest::floatingPanelsPersistAndSurviveLayoutLock()
{
    const QString originalOrganization =
        QCoreApplication::organizationName();
    const QString originalApplication =
        QCoreApplication::applicationName();
    const QSettings::Format settingsFormat =
        QSettings::defaultFormat();
    const QString originalSettingsPath =
        QStandardPaths::writableLocation(
            QStandardPaths::GenericConfigLocation);
    QTemporaryDir settingsDirectory;
    QVERIFY(settingsDirectory.isValid());
    QSettings::setPath(
        settingsFormat, QSettings::UserScope,
        settingsDirectory.path());
    const auto restoreSettings = qScopeGuard(
        [originalOrganization, originalApplication,
         originalSettingsPath, settingsFormat] {
        QSettings().clear();
        QCoreApplication::setOrganizationName(
            originalOrganization);
        QCoreApplication::setApplicationName(
            originalApplication);
        QSettings::setPath(
            settingsFormat, QSettings::UserScope,
            originalSettingsPath);
    });
    QCoreApplication::setOrganizationName(
        QStringLiteral("ClearveilTest"));
    QCoreApplication::setApplicationName(
        QStringLiteral("clearveil-floating-layout-test"));
    QSettings().clear();

    QRect savedPanelGeometry;
    {
        MainWindow window;
        window.setGeometry(40, 40, 900, 650);
        window.show();
        auto *dock = window.findChild<QDockWidget *>(
            QStringLiteral("metadataDock"));
        auto *floatAction = window.findChild<QAction *>(
            QStringLiteral("floatMetadataAction"));
        auto *lockAction = window.findChild<QAction *>(
            QStringLiteral("layoutLockAction"));
        auto *informationAction = window.findChild<QAction *>(
            QStringLiteral("metadataAction"));
        auto *toolbar = window.findChild<QToolBar *>(
            QStringLiteral("mainToolbar"));
        auto *colorPickerAction = window.findChild<QAction *>(
            QStringLiteral("colorPickerAction"));
        auto *colorPickerDock = window.findChild<QDockWidget *>(
            QStringLiteral("colorPickerDock"));
        auto *layoutSaveTimer = window.findChild<QTimer *>(
            QStringLiteral("panelLayoutSaveTimer"));
        QVERIFY(dock);
        QVERIFY(floatAction);
        QVERIFY(lockAction);
        QVERIFY(informationAction);
        QVERIFY(toolbar);
        QVERIFY(colorPickerAction);
        QVERIFY(colorPickerDock);
        QVERIFY(layoutSaveTimer);
        QVERIFY(layoutSaveTimer->isSingleShot());
        QCOMPARE(layoutSaveTimer->interval(), 300);

        informationAction->setChecked(true);
        toolbar->setVisible(false);
        colorPickerAction->setChecked(true);
        QVERIFY(!toolbar->isVisible());
        QVERIFY(colorPickerDock->isVisible());
        lockAction->setChecked(false);
        floatAction->setChecked(true);
        QTRY_VERIFY(dock->isFloating());
        QTRY_VERIFY(dock->isVisible());
        QVERIFY(floatAction->isChecked());

        dock->setGeometry(QRect(120, 110, 360, 410));
        QCoreApplication::processEvents();
        savedPanelGeometry = dock->frameGeometry();
        QVERIFY(savedPanelGeometry.isValid());
        QTRY_VERIFY_WITH_TIMEOUT(
            !QSettings().value(QStringLiteral(
                "layout/docks/information/geometry"))
                 .toByteArray().isEmpty(),
            1500);

        lockAction->setChecked(true);
        QTRY_VERIFY(dock->isFloating());
        QTRY_VERIFY(dock->isVisible());
        QVERIFY(floatAction->isChecked());
        QVERIFY(dock->titleBarWidget());
        QVERIFY(dock->titleBarWidget()->height() >= 28);
        QCOMPARE(dock->titleBarWidget()->objectName(),
                 QStringLiteral("panelDragHandle_information"));
        auto *formButton = dock->titleBarWidget()
                               ->findChild<QToolButton *>();
        QVERIFY(formButton);
        QVERIFY(!formButton->icon().isNull());
        QVERIFY(!formButton->toolTip().trimmed().isEmpty());

        QVERIFY(window.close());
        QVERIFY(!window.isVisible());
        QCoreApplication::processEvents();
    }

    {
        QSettings saved;
        QCOMPARE(
            saved.value(
                QStringLiteral(
                    "layout/docks/information/floating"))
                .toBool(),
            true);
        QCOMPARE(saved.value(
            QStringLiteral("view/showToolbar")).toBool(), false);
        QCOMPARE(saved.value(
            QStringLiteral("view/showInformation")).toBool(), true);
        QCOMPARE(saved.value(
            QStringLiteral("view/showColorPicker")).toBool(), true);
        QVERIFY(!saved.value(
            QStringLiteral(
                "layout/docks/information/geometry"))
                     .toByteArray()
                     .isEmpty());
    }

    {
        MainWindow restored;
        restored.show();
        auto *dock = restored.findChild<QDockWidget *>(
            QStringLiteral("metadataDock"));
        auto *floatAction = restored.findChild<QAction *>(
            QStringLiteral("floatMetadataAction"));
        auto *toolbar = restored.findChild<QToolBar *>(
            QStringLiteral("mainToolbar"));
        auto *colorPickerAction = restored.findChild<QAction *>(
            QStringLiteral("colorPickerAction"));
        auto *colorPickerDock = restored.findChild<QDockWidget *>(
            QStringLiteral("colorPickerDock"));
        QVERIFY(dock);
        QVERIFY(floatAction);
        QVERIFY(toolbar);
        QVERIFY(colorPickerAction);
        QVERIFY(colorPickerDock);
        QTRY_VERIFY(dock->isFloating());
        QVERIFY(dock->isVisible());
        QVERIFY(floatAction->isChecked());
        QVERIFY(!toolbar->isVisible());
        QVERIFY(colorPickerAction->isChecked());
        QVERIFY(colorPickerDock->isVisible());
        const QRect restoredGeometry =
            dock->frameGeometry();
        QVERIFY(restoredGeometry.isValid());
        QVERIFY(std::abs(
            restoredGeometry.center().x()
            - savedPanelGeometry.center().x()) < 40);
        QVERIFY(std::abs(
            restoredGeometry.center().y()
            - savedPanelGeometry.center().y()) < 40);
        QVERIFY(std::abs(
            restoredGeometry.width()
            - savedPanelGeometry.width()) < 40);
        QVERIFY(std::abs(
            restoredGeometry.height()
            - savedPanelGeometry.height()) < 40);
        restored.close();
    }

}

void CoreTest::interfaceLayoutPageCentralizesPanelChanges()
{
    QTranslator layoutTranslator;
    const QString translationPath = qEnvironmentVariable(
        "CLEARVEIL_TEST_LAYOUT_TRANSLATION");
    const bool translatorInstalled = !translationPath.isEmpty()
        && layoutTranslator.load(translationPath)
        && QCoreApplication::installTranslator(&layoutTranslator);
    const auto removeTranslator = qScopeGuard(
        [&layoutTranslator, translatorInstalled] {
        if (translatorInstalled)
            QCoreApplication::removeTranslator(&layoutTranslator);
    });

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString imagePath =
        directory.filePath(QStringLiteral("panel-manager.png"));
    QImage image(160, 120, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QVERIFY(image.save(imagePath));

    MainWindow window;
    window.resize(960, 640);
    QVERIFY(window.openPath(imagePath));
    window.show();
    auto *layoutAction = window.findChild<QAction *>(
        QStringLiteral("interfaceLayoutAction"));
    auto *filmstripDock = window.findChild<QDockWidget *>(
        QStringLiteral("filmstripDock"));
    auto *informationDock = window.findChild<QDockWidget *>(
        QStringLiteral("metadataDock"));
    auto *filmstrip = dynamic_cast<FilmstripView *>(
        window.findChild<QListView *>(QStringLiteral("filmstrip")));
    QVERIFY(layoutAction);
    QVERIFY(filmstripDock);
    QVERIFY(informationDock);
    QVERIFY(filmstrip);
    QVERIFY(!window.findChild<QMenu *>(QStringLiteral("panelsMenu")));
    QVERIFY(!window.findChild<QDialog *>(
        QStringLiteral("panelManagerDialog")));

    bool pageSelected = false;
    bool applyKeptDialogOpen = false;
    QTimer::singleShot(0, &window, [&] {
        auto *dialog = qobject_cast<SettingsDialog *>(
            QApplication::activeModalWidget());
        QVERIFY(dialog);
        auto *tabs = dialog->findChild<QTabWidget *>(
            QStringLiteral("settingsTabs"));
        auto *page = dialog->findChild<InterfaceLayoutPage *>(
            QStringLiteral("interfaceLayoutPage"));
        auto *thumbnailPlacement = dialog->findChild<QComboBox *>(
            QStringLiteral("thumbnailsPanelPlacement"));
        auto *thumbnailVisible = dialog->findChild<QCheckBox *>(
            QStringLiteral("thumbnailsPanelVisibleCheckBox"));
        auto *floatingLayout = dialog->findChild<QComboBox *>(
            QStringLiteral("floatingThumbnailLayout"));
        auto *informationVisible = dialog->findChild<QCheckBox *>(
            QStringLiteral("informationPanelVisibleCheckBox"));
        auto *informationPlacement = dialog->findChild<QComboBox *>(
            QStringLiteral("informationPanelPlacement"));
        auto *colorPickerVisible = dialog->findChild<QCheckBox *>(
            QStringLiteral("colorPickerPanelVisibleCheckBox"));
        auto *colorPickerPlacement = dialog->findChild<QComboBox *>(
            QStringLiteral("colorPickerPanelPlacement"));
        auto *showToolbar = dialog->findChild<QCheckBox *>(
            QStringLiteral("layoutShowToolbar"));
        auto *showMenuBar = dialog->findChild<QCheckBox *>(
            QStringLiteral("layoutShowMenuBar"));
        auto *showStatusBar = dialog->findChild<QCheckBox *>(
            QStringLiteral("layoutShowStatusBar"));
        auto *toolbarPosition = dialog->findChild<QComboBox *>(
            QStringLiteral("toolbarPosition"));
        auto *lockLayout = dialog->findChild<QCheckBox *>(
            QStringLiteral("layoutLocked"));
        auto *apply = dialog->findChild<QPushButton *>(
            QStringLiteral("settingsApplyButton"));
        QVERIFY(tabs);
        QVERIFY(page);
        QVERIFY(thumbnailPlacement);
        QVERIFY(thumbnailVisible);
        QVERIFY(floatingLayout);
        QVERIFY(informationVisible);
        QVERIFY(informationPlacement);
        QVERIFY(colorPickerVisible);
        QVERIFY(colorPickerPlacement);
        QVERIFY(showToolbar);
        QVERIFY(showMenuBar);
        QVERIFY(showStatusBar);
        QVERIFY(toolbarPosition);
        QVERIFY(lockLayout);
        QVERIFY(apply);
        QVERIFY(thumbnailPlacement->findData(
                    QStringLiteral("overlay")) >= 0);
        QVERIFY(thumbnailPlacement->findData(
                    QStringLiteral("floating")) >= 0);
        pageSelected = tabs->currentWidget() == page;
        thumbnailVisible->setChecked(false);
        informationVisible->setChecked(false);
        colorPickerVisible->setChecked(false);
        showToolbar->setChecked(false);
        QVERIFY(!thumbnailPlacement->isEnabled());
        QVERIFY(!floatingLayout->isEnabled());
        QVERIFY(!informationPlacement->isEnabled());
        QVERIFY(!colorPickerPlacement->isEnabled());
        QVERIFY(!toolbarPosition->isEnabled());
        thumbnailVisible->setChecked(true);
        showToolbar->setChecked(true);
        showMenuBar->setChecked(true);
        showStatusBar->setChecked(true);
        QVERIFY(thumbnailPlacement->isEnabled());
        QVERIFY(toolbarPosition->isEnabled());
        QVERIFY(dialog->interfaceLayout().showMenuBar);
        QVERIFY(dialog->interfaceLayout().showStatusBar);
        thumbnailPlacement->setCurrentIndex(
            thumbnailPlacement->findData(QStringLiteral("overlay")));
        floatingLayout->setCurrentIndex(
            floatingLayout->findData(QStringLiteral("vertical")));
        informationVisible->setChecked(true);
        informationPlacement->setCurrentIndex(
            informationPlacement->findData(QStringLiteral("left")));
        lockLayout->setChecked(false);
        const QString screenshotPath = qEnvironmentVariable(
            "CLEARVEIL_TEST_LAYOUT_SCREENSHOT");
        if (!screenshotPath.isEmpty())
            QVERIFY(dialog->grab().save(screenshotPath));
        apply->click();
        QCoreApplication::processEvents();
        applyKeptDialogOpen = dialog->isVisible();
        dialog->accept();
    });
    QVERIFY(QMetaObject::invokeMethod(
        &window, "showInterfaceLayout", Qt::DirectConnection));

    QVERIFY(pageSelected);
    QVERIFY(applyKeptDialogOpen);
    QVERIFY(!filmstripDock->isFloating());
    QVERIFY(filmstripDock->property("clearveilOverlay").toBool());
    QCOMPARE(filmstripDock->parentWidget(), window.centralWidget());
    QVERIFY(filmstripDock->isVisible());
    QVERIFY(filmstrip->isVerticalLayout());
    QVERIFY(informationDock->isVisible());
    QCOMPARE(window.dockWidgetArea(informationDock),
             Qt::LeftDockWidgetArea);
    auto *orientationButton = window.findChild<QToolButton *>(
        QStringLiteral("floatingThumbnailLayoutButton"));
    QVERIFY(orientationButton);
    QVERIFY(orientationButton->isVisible());
    QVERIFY(!orientationButton->toolTip().trimmed().isEmpty());
    const QString overlayScreenshotPath = qEnvironmentVariable(
        "CLEARVEIL_TEST_OVERLAY_SCREENSHOT");
    if (!overlayScreenshotPath.isEmpty())
        QVERIFY(window.grab().save(overlayScreenshotPath));
    auto *automatic = window.findChild<QAction *>(
        QStringLiteral("floatingThumbnailLayoutAuto"));
    QVERIFY(automatic);
    automatic->trigger();
    filmstripDock->resize(720, 190);
    QTRY_VERIFY(!filmstrip->isVerticalLayout());
    filmstripDock->resize(260, 620);
    QTRY_VERIFY(filmstrip->isVerticalLayout());
    auto *panelController =
        window.findChild<PanelLayoutController *>();
    QVERIFY(panelController);
    panelController->setPlacement(
        filmstripDock, QStringLiteral("floating"));
    QTRY_VERIFY(filmstripDock->isFloating());
    QVERIFY(!filmstripDock->property("clearveilOverlay").toBool());
}

void CoreTest::interfaceLayoutPreviewSupportsDragRearrangement()
{
    InterfaceLayoutState state;
    state.showThumbnails = false;
    state.showInformation = true;
    state.informationPlacement = QStringLiteral("right");
    state.showColorPicker = true;
    state.colorPickerPlacement = QStringLiteral("right");
    state.panelOrder = {
        QStringLiteral("thumbnails"),
        QStringLiteral("information"),
        QStringLiteral("colorPicker")};

    InterfaceLayoutPage page(state);
    page.resize(920, 640);
    page.show();
    auto *preview = page.findChild<QWidget *>(
        QStringLiteral("interfaceLayoutPreview"));
    QVERIFY(preview);
    QTRY_VERIFY(preview->isVisible());
    QCoreApplication::processEvents();

    const QRect toolbarRect = preview->property(
        "clearveilPreviewRect_toolbar").toRect();
    QVERIFY(toolbarRect.isValid());
    const QPoint toolbarTop = toolbarRect.center();
    const QPoint viewerLeft(30, preview->height() / 2);
    QTest::mousePress(preview, Qt::LeftButton,
                      Qt::NoModifier, toolbarTop);
    QTest::mouseMove(preview, viewerLeft, 20);
    QTest::mouseRelease(preview, Qt::LeftButton,
                        Qt::NoModifier, viewerLeft);
    QTRY_COMPARE(page.state().toolbarPosition,
                 QStringLiteral("left"));
    QCoreApplication::processEvents();

    const QRect colorPickerRect = preview->property(
        "clearveilPreviewRect_colorPicker").toRect();
    const QRect informationRect = preview->property(
        "clearveilPreviewRect_information").toRect();
    QVERIFY(colorPickerRect.isValid());
    QVERIFY(informationRect.isValid());
    const QPoint colorPickerBottom = colorPickerRect.center();
    const QPoint informationTop = informationRect.center();
    QTest::mousePress(preview, Qt::LeftButton,
                      Qt::NoModifier, colorPickerBottom);
    QTest::mouseMove(preview, informationTop, 20);
    QTest::mouseRelease(preview, Qt::LeftButton,
                        Qt::NoModifier, informationTop);
    QTRY_COMPARE(
        page.state().panelOrder,
        QStringList({QStringLiteral("thumbnails"),
                     QStringLiteral("colorPicker"),
                     QStringLiteral("information")}));

    QCoreApplication::processEvents();
    const QPoint colorPickerTop = preview->property(
        "clearveilPreviewRect_colorPicker").toRect().center();
    const QPoint viewerCenter = preview->rect().center();
    QTest::mousePress(preview, Qt::LeftButton,
                      Qt::NoModifier, colorPickerTop);
    QTest::mouseMove(preview, viewerCenter, 20);
    QTest::mouseRelease(preview, Qt::LeftButton,
                        Qt::NoModifier, viewerCenter);
    QTRY_COMPARE(page.state().colorPickerPlacement,
                 QStringLiteral("overlay"));
}

void CoreTest::panelLayoutControllerOwnsFloatingState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString settingsPath =
        directory.filePath(QStringLiteral("layout.ini"));

    {
        QMainWindow window;
        auto *dock = new QDockWidget(
            QStringLiteral("Test panel"), &window);
        auto *floatingAction = new QAction(&window);
        floatingAction->setCheckable(true);
        window.addDockWidget(Qt::BottomDockWidgetArea, dock);

        PanelLayoutController controller(&window);
        controller.addPanel(
            dock, floatingAction, QStringLiteral("testPanel"),
            PanelLayoutController::DefaultFloatingPosition::Bottom,
            Qt::BottomDockWidgetArea);
        window.show();

        dock->hide();
        QVERIFY(!dock->isVisible());
        floatingAction->setChecked(true);
        QTRY_VERIFY(dock->isFloating());
        QTRY_VERIFY(dock->isVisible());
        QCOMPARE(dock->property("clearveilFloating").toBool(),
                 true);
        auto *panelTitle = dock->titleBarWidget()
                               ->findChild<QLabel *>(
                                   QStringLiteral("floatingPanelTitle"));
        QVERIFY(panelTitle);
        QVERIFY(panelTitle->isVisible());
        QCOMPARE(panelTitle->text(), QStringLiteral("Test panel"));
        dock->setGeometry(QRect(90, 80, 320, 220));
        QCoreApplication::processEvents();

        controller.setLocked(true);
        QVERIFY(dock->isFloating());
        QVERIFY(dock->titleBarWidget());
        QVERIFY(dock->titleBarWidget()->height() >= 28);
        auto *formButton = dock->titleBarWidget()
                               ->findChild<QToolButton *>();
        QVERIFY(formButton);
        QVERIFY(formButton->isVisible());
        QVERIFY(!formButton->icon().isNull());

        controller.setLocked(false);
        formButton->click();
        QTRY_VERIFY(!dock->isFloating());
        QCOMPARE(dock->property("clearveilFloating").toBool(),
                 false);
        QVERIFY(!panelTitle->isVisible());
        QVERIFY(!floatingAction->isChecked());
        formButton->click();
        QTRY_VERIFY(dock->isFloating());
        QVERIFY(floatingAction->isChecked());
        controller.setLocked(true);

        QSettings settings(settingsPath, QSettings::IniFormat);
        controller.save(settings);
        settings.sync();
        QCOMPARE(settings.value(QStringLiteral(
                     "layout/docks/testPanel/floating")).toBool(),
                 true);
        QVERIFY(!settings.value(QStringLiteral(
                     "layout/docks/testPanel/geometry"))
                     .toByteArray().isEmpty());

        controller.resetFloatingPanels();
        QVERIFY(!dock->isFloating());
        QCOMPARE(dock->titleBarWidget()->height(), 4);
        QCOMPARE(window.dockWidgetArea(dock),
                 Qt::BottomDockWidgetArea);
        QVERIFY(dock->property("clearveilLastFloatingGeometry")
                    .toByteArray().isEmpty());

        controller.setLocked(false);
        QVERIFY(dock->features().testFlag(
            QDockWidget::DockWidgetFloatable));
    }

    {
        QMainWindow window;
        auto *dock = new QDockWidget(&window);
        auto *floatingAction = new QAction(&window);
        floatingAction->setCheckable(true);
        window.addDockWidget(Qt::BottomDockWidgetArea, dock);
        PanelLayoutController controller(&window);
        controller.addPanel(
            dock, floatingAction, QStringLiteral("testPanel"),
            PanelLayoutController::DefaultFloatingPosition::Bottom,
            Qt::BottomDockWidgetArea);

        QSettings settings(settingsPath, QSettings::IniFormat);
        controller.restore(settings);
        QVERIFY(dock->isFloating());
        QVERIFY(floatingAction->isChecked());
    }
}

void CoreTest::colorPickerMigratesFromSideDockToCompactOverlay()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString settingsPath =
        directory.filePath(QStringLiteral("color-picker-layout.ini"));
    {
        QSettings settings(settingsPath, QSettings::IniFormat);
        settings.setValue(QStringLiteral(
                              "layout/docks/colorPicker/mode"),
                          QStringLiteral("docked"));
        settings.sync();
    }

    {
        QMainWindow window;
        window.resize(900, 620);
        window.setCentralWidget(new QWidget(&window));
        auto *dock = new QDockWidget(&window);
        dock->setWidget(new QWidget(dock));
        auto *floatingAction = new QAction(&window);
        floatingAction->setCheckable(true);
        window.addDockWidget(Qt::LeftDockWidgetArea, dock);
        PanelLayoutController controller(&window);
        controller.addPanel(
            dock, floatingAction, QStringLiteral("colorPicker"),
            PanelLayoutController::DefaultFloatingPosition::Left,
            Qt::LeftDockWidgetArea);
        window.show();

        QSettings settings(settingsPath, QSettings::IniFormat);
        controller.restore(settings);
        QTRY_VERIFY(controller.isOverlay(dock));
        QCOMPARE(controller.placementName(dock),
                 QStringLiteral("overlay"));
        QCOMPARE(settings.value(QStringLiteral(
                     "layout/docks/colorPicker/compactPlacementRevision"))
                     .toInt(),
                 1);

        controller.setPlacement(dock, QStringLiteral("left"));
        QTRY_VERIFY(!controller.isOverlay(dock));
        controller.save(settings);
        settings.sync();
    }

    {
        QMainWindow window;
        window.resize(900, 620);
        window.setCentralWidget(new QWidget(&window));
        auto *dock = new QDockWidget(&window);
        dock->setWidget(new QWidget(dock));
        auto *floatingAction = new QAction(&window);
        floatingAction->setCheckable(true);
        window.addDockWidget(Qt::LeftDockWidgetArea, dock);
        PanelLayoutController controller(&window);
        controller.addPanel(
            dock, floatingAction, QStringLiteral("colorPicker"),
            PanelLayoutController::DefaultFloatingPosition::Left,
            Qt::LeftDockWidgetArea);
        window.show();

        QSettings settings(settingsPath, QSettings::IniFormat);
        controller.restore(settings);
        QCoreApplication::processEvents();
        QVERIFY(!controller.isOverlay(dock));
        QCOMPARE(controller.placementName(dock),
                 QStringLiteral("left"));
    }
}

void CoreTest::panelLayoutControllerAnchorsOverlayToMainWindow()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString settingsPath =
        directory.filePath(QStringLiteral("overlay-layout.ini"));
    const QRect requestedGeometry(84, 72, 360, 240);

    {
        QMainWindow window;
        window.resize(900, 620);
        window.setCentralWidget(new QWidget(&window));
        auto *dock = new QDockWidget(
            QStringLiteral("Overlay panel"), &window);
        auto *content = new QWidget(dock);
        auto *contentLayout = new QVBoxLayout(content);
        auto *contentButton = new QPushButton(
            QStringLiteral("Interactive content"), content);
        contentLayout->addWidget(contentButton);
        contentLayout->addStretch();
        dock->setWidget(content);
        auto *floatingAction = new QAction(&window);
        floatingAction->setCheckable(true);
        window.addDockWidget(Qt::RightDockWidgetArea, dock);
        PanelLayoutController controller(&window);
        controller.addPanel(
            dock, floatingAction, QStringLiteral("overlayPanel"),
            PanelLayoutController::DefaultFloatingPosition::Right,
            Qt::RightDockWidgetArea);
        controller.setLocked(false);
        window.show();

        controller.setPlacement(dock, QStringLiteral("overlay"));
        QTRY_VERIFY(controller.isOverlay(dock));
        QCOMPARE(controller.placementName(dock),
                 QStringLiteral("overlay"));
        QCOMPARE(dock->parentWidget(), window.centralWidget());
        QVERIFY(!dock->isWindow());
        QVERIFY(dock->isVisible());
        QVERIFY(dock->property("clearveilOverlay").toBool());
        QVERIFY(dock->testAttribute(Qt::WA_StyledBackground));
        QVERIFY(dock->autoFillBackground());
        QVERIFY(content->testAttribute(Qt::WA_StyledBackground));
        QVERIFY(content->autoFillBackground());
        QVERIFY(window.centralWidget()->rect().contains(
            dock->geometry()));

        QSignalSpy contentSpy(contentButton, &QPushButton::clicked);
        contentButton->click();
        QCOMPARE(contentSpy.count(), 1);

        auto *titleBar = qobject_cast<PanelTitleBar *>(
            dock->titleBarWidget());
        auto *resizeHandle = dock->findChild<QWidget *>(
            QStringLiteral("overlayPanelResizeHandle"));
        auto *panelBorder = dock->findChild<QWidget *>(
            QStringLiteral("floatingPanelBorder"));
        QVERIFY(titleBar);
        QVERIFY(resizeHandle);
        QVERIFY(panelBorder);
        QVERIFY(titleBar->isVisible());
        QVERIFY(resizeHandle->isVisible());
        QVERIFY(panelBorder->isVisible());
        QCOMPARE(panelBorder->geometry(), dock->rect());
        QVERIFY(panelBorder->testAttribute(
            Qt::WA_TransparentForMouseEvents));
        const QPoint beforeMouseDrag = dock->pos();
        const QPoint localPress(titleBar->width() / 2,
                                titleBar->height() / 2);
        const QPoint globalPress = titleBar->mapToGlobal(localPress);
        const QPoint dragDelta(-30, 0);
        QMouseEvent pressEvent(
            QEvent::MouseButtonPress, QPointF(localPress),
            QPointF(globalPress), Qt::LeftButton,
            Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(titleBar, &pressEvent);
        QMouseEvent moveEvent(
            QEvent::MouseMove,
            QPointF(localPress + dragDelta),
            QPointF(globalPress + dragDelta), Qt::NoButton,
            Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(titleBar, &moveEvent);
        QMouseEvent releaseEvent(
            QEvent::MouseButtonRelease,
            QPointF(localPress + dragDelta),
            QPointF(globalPress + dragDelta), Qt::LeftButton,
            Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(titleBar, &releaseEvent);
        QCOMPARE(dock->pos(), beforeMouseDrag + dragDelta);

        dock->resize(requestedGeometry.size());
        QCoreApplication::processEvents();
        QVERIFY(QMetaObject::invokeMethod(
            titleBar, "overlayMoveRequested", Qt::DirectConnection,
            Q_ARG(QPoint, requestedGeometry.topLeft())));
        QCOMPARE(dock->pos(), requestedGeometry.topLeft());
        QCOMPARE(dock->geometry(), requestedGeometry);

        controller.setLocked(true);
        QVERIFY(titleBar->isVisible());
        QVERIFY(resizeHandle->isVisible());
        const QPoint lockedPosition = dock->pos();
        QVERIFY(QMetaObject::invokeMethod(
            titleBar, "overlayMoveRequested", Qt::DirectConnection,
            Q_ARG(QPoint, QPoint(20, 20))));
        QVERIFY(dock->pos() != lockedPosition);
        QCOMPARE(dock->pos(), QPoint(20, 20));
        QVERIFY(QMetaObject::invokeMethod(
            titleBar, "overlayMoveRequested", Qt::DirectConnection,
            Q_ARG(QPoint, requestedGeometry.topLeft())));
        QCOMPARE(dock->geometry(), requestedGeometry);

        const int anchoredTop = 70;
        const QPoint nearRightEdge(
            window.centralWidget()->width() - dock->width() - 6,
            anchoredTop);
        QVERIFY(QMetaObject::invokeMethod(
            titleBar, "overlayMoveRequested", Qt::DirectConnection,
            Q_ARG(QPoint, nearRightEdge)));
        QCOMPARE(dock->geometry().right(),
                 window.centralWidget()->rect().right());
        QCOMPARE(dock->geometry().top(), anchoredTop);
        window.resize(window.width() + 140, window.height());
        QTRY_COMPARE(dock->geometry().right(),
                     window.centralWidget()->rect().right());
        QCOMPARE(dock->geometry().top(), anchoredTop);

        QSettings settings(settingsPath, QSettings::IniFormat);
        controller.save(settings);
        settings.sync();
        QCOMPARE(settings.value(QStringLiteral(
                     "layout/docks/overlayPanel/mode")).toString(),
                 QStringLiteral("overlay"));
        QCOMPARE(settings.value(QStringLiteral(
                     "layout/docks/overlayPanel/overlayGeometry")).toRect(),
                 dock->geometry());
        QCOMPARE(settings.value(QStringLiteral(
                     "layout/docks/overlayPanel/overlayAnchors")).toString(),
                 QStringLiteral("right"));
    }

    {
        QMainWindow window;
        window.resize(900, 620);
        window.setCentralWidget(new QWidget(&window));
        auto *dock = new QDockWidget(
            QStringLiteral("Overlay panel"), &window);
        dock->setWidget(new QLabel(QStringLiteral("Restored"), dock));
        auto *floatingAction = new QAction(&window);
        floatingAction->setCheckable(true);
        window.addDockWidget(Qt::RightDockWidgetArea, dock);
        PanelLayoutController controller(&window);
        controller.addPanel(
            dock, floatingAction, QStringLiteral("overlayPanel"),
            PanelLayoutController::DefaultFloatingPosition::Right,
            Qt::RightDockWidgetArea);
        window.show();
        QSettings settings(settingsPath, QSettings::IniFormat);
        controller.restore(settings);
        QTRY_VERIFY(controller.isOverlay(dock));
        QCOMPARE(dock->parentWidget(), window.centralWidget());
        QCOMPARE(dock->geometry().right(),
                 window.centralWidget()->rect().right());
        QCOMPARE(dock->geometry().top(), 70);

        auto *titleBar = qobject_cast<PanelTitleBar *>(
            dock->titleBarWidget());
        QVERIFY(titleBar);
        QVERIFY(QMetaObject::invokeMethod(
            titleBar, "overlayDockRequested", Qt::DirectConnection));
        QTRY_VERIFY(!controller.isOverlay(dock));
        QCOMPARE(window.dockWidgetArea(dock),
                 Qt::RightDockWidgetArea);
    }
}

void CoreTest::panelLayoutControllerSnapsOverlayPanelsTogether()
{
    QMainWindow window;
    window.resize(900, 700);
    window.setCentralWidget(new QWidget(&window));

    auto *target = new QDockWidget(
        QStringLiteral("Target panel"), &window);
    target->setWidget(new QLabel(QStringLiteral("Target"), target));
    auto *follower = new QDockWidget(
        QStringLiteral("Follower panel"), &window);
    follower->setWidget(new QLabel(QStringLiteral("Follower"), follower));
    auto *targetAction = new QAction(&window);
    auto *followerAction = new QAction(&window);
    targetAction->setCheckable(true);
    followerAction->setCheckable(true);
    window.addDockWidget(Qt::RightDockWidgetArea, target);
    window.addDockWidget(Qt::BottomDockWidgetArea, follower);

    PanelLayoutController controller(&window);
    controller.addPanel(
        target, targetAction, QStringLiteral("target"),
        PanelLayoutController::DefaultFloatingPosition::Right,
        Qt::RightDockWidgetArea);
    controller.addPanel(
        follower, followerAction, QStringLiteral("follower"),
        PanelLayoutController::DefaultFloatingPosition::Bottom,
        Qt::BottomDockWidgetArea);
    window.show();
    controller.setPlacement(target, QStringLiteral("overlay"));
    controller.setPlacement(follower, QStringLiteral("overlay"));
    QTRY_VERIFY(controller.isOverlay(target));
    QTRY_VERIFY(controller.isOverlay(follower));

    auto *targetTitle = qobject_cast<PanelTitleBar *>(
        target->titleBarWidget());
    auto *followerTitle = qobject_cast<PanelTitleBar *>(
        follower->titleBarWidget());
    QVERIFY(targetTitle);
    QVERIFY(followerTitle);
    target->resize(260, 220);
    follower->resize(220, 180);
    QVERIFY(QMetaObject::invokeMethod(
        targetTitle, "overlayMoveRequested", Qt::DirectConnection,
        Q_ARG(QPoint, QPoint(100, 80))));
    const QPoint nearBelow(
        120, target->geometry().bottom() + 7);
    QVERIFY(QMetaObject::invokeMethod(
        followerTitle, "overlayMoveRequested", Qt::DirectConnection,
        Q_ARG(QPoint, nearBelow)));
    QCOMPARE(follower->geometry().top(),
             target->geometry().bottom() + 1);
    QCOMPARE(follower->geometry().left(), 120);

    QVERIFY(QMetaObject::invokeMethod(
        targetTitle, "overlayMoveRequested", Qt::DirectConnection,
        Q_ARG(QPoint, QPoint(160, 110))));
    QTRY_COMPARE(follower->geometry().top(),
                 target->geometry().bottom() + 1);
    QCOMPARE(follower->geometry().left(), 180);
    target->resize(260, 240);
    QTRY_COMPARE(follower->geometry().top(),
                 target->geometry().bottom() + 1);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(
        directory.filePath(QStringLiteral("peer-layout.ini")),
        QSettings::IniFormat);
    controller.save(settings);
    settings.sync();
    QCOMPARE(settings.value(QStringLiteral(
                 "layout/docks/follower/overlayPeerId")).toString(),
             QStringLiteral("target"));
    QCOMPARE(settings.value(QStringLiteral(
                 "layout/docks/follower/overlayPeerAttachment")).toString(),
             QStringLiteral("below"));

    {
        QMainWindow restoredWindow;
        restoredWindow.resize(900, 700);
        restoredWindow.setCentralWidget(
            new QWidget(&restoredWindow));
        auto *restoredTarget = new QDockWidget(
            QStringLiteral("Target panel"), &restoredWindow);
        restoredTarget->setWidget(
            new QLabel(QStringLiteral("Target"), restoredTarget));
        auto *restoredFollower = new QDockWidget(
            QStringLiteral("Follower panel"), &restoredWindow);
        restoredFollower->setWidget(
            new QLabel(QStringLiteral("Follower"), restoredFollower));
        auto *restoredTargetAction = new QAction(&restoredWindow);
        auto *restoredFollowerAction = new QAction(&restoredWindow);
        restoredTargetAction->setCheckable(true);
        restoredFollowerAction->setCheckable(true);
        restoredWindow.addDockWidget(
            Qt::RightDockWidgetArea, restoredTarget);
        restoredWindow.addDockWidget(
            Qt::BottomDockWidgetArea, restoredFollower);
        PanelLayoutController restoredController(&restoredWindow);
        restoredController.addPanel(
            restoredTarget, restoredTargetAction,
            QStringLiteral("target"),
            PanelLayoutController::DefaultFloatingPosition::Right,
            Qt::RightDockWidgetArea);
        restoredController.addPanel(
            restoredFollower, restoredFollowerAction,
            QStringLiteral("follower"),
            PanelLayoutController::DefaultFloatingPosition::Bottom,
            Qt::BottomDockWidgetArea);
        restoredWindow.show();
        QSettings restoredSettings(
            settings.fileName(), QSettings::IniFormat);
        restoredController.restore(restoredSettings);
        QTRY_VERIFY(restoredController.isOverlay(restoredTarget));
        QTRY_VERIFY(restoredController.isOverlay(restoredFollower));
        QTRY_COMPARE(restoredFollower->geometry().top(),
                     restoredTarget->geometry().bottom() + 1);
        const int restoredOffset = restoredFollower->geometry().left()
            - restoredTarget->geometry().left();
        auto *restoredTargetTitle = qobject_cast<PanelTitleBar *>(
            restoredTarget->titleBarWidget());
        QVERIFY(restoredTargetTitle);
        QVERIFY(QMetaObject::invokeMethod(
            restoredTargetTitle, "overlayMoveRequested",
            Qt::DirectConnection,
            Q_ARG(QPoint, QPoint(220, 140))));
        QTRY_COMPARE(restoredFollower->geometry().top(),
                     restoredTarget->geometry().bottom() + 1);
        QCOMPARE(restoredFollower->geometry().left(),
                 restoredTarget->geometry().left() + restoredOffset);
    }

    QVERIFY(QMetaObject::invokeMethod(
        followerTitle, "overlayMoveRequested", Qt::DirectConnection,
        Q_ARG(QPoint, QPoint(500, 430))));
    const QPoint detachedPosition = follower->pos();
    QVERIFY(QMetaObject::invokeMethod(
        targetTitle, "overlayMoveRequested", Qt::DirectConnection,
        Q_ARG(QPoint, QPoint(200, 120))));
    QCoreApplication::processEvents();
    QCOMPARE(follower->pos(), detachedPosition);
}

void CoreTest::largeFolderFilmstripSwitchDoesNotRebuildThumbnails()
{
    const QString originalOrganization =
        QCoreApplication::organizationName();
    const QString originalApplication =
        QCoreApplication::applicationName();
    const QSettings::Format settingsFormat =
        QSettings::defaultFormat();
    const QString originalSettingsPath =
        QStandardPaths::writableLocation(
            QStandardPaths::GenericConfigLocation);
    QTemporaryDir settingsDirectory;
    QVERIFY(settingsDirectory.isValid());
    QSettings::setPath(settingsFormat, QSettings::UserScope,
                       settingsDirectory.path());
    const auto restoreSettings = qScopeGuard(
        [originalOrganization, originalApplication,
         originalSettingsPath, settingsFormat] {
        QSettings().clear();
        QCoreApplication::setOrganizationName(originalOrganization);
        QCoreApplication::setApplicationName(originalApplication);
        QSettings::setPath(settingsFormat, QSettings::UserScope,
                           originalSettingsPath);
    });
    QCoreApplication::setOrganizationName(
        QStringLiteral("ClearveilTest"));
    QCoreApplication::setApplicationName(
        QStringLiteral("clearveil-large-filmstrip-test"));
    QSettings().clear();

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString seedPath =
        directory.filePath(QStringLiteral("image000.png"));
    QImage seed(640, 480, QImage::Format_RGB32);
    seed.fill(QColor(64, 112, 196));
    QVERIFY(seed.save(seedPath));

    QStringList files{seedPath};
    constexpr int imageCount = 400;
    for (int index = 1; index < imageCount; ++index) {
        const QString path = directory.filePath(
            QStringLiteral("image%1.png").arg(index, 3, 10, QLatin1Char('0')));
        QVERIFY(QFile::link(seedPath, path));
        files.append(path);
    }

    MainWindow window;
    QElapsedTimer timer;
    timer.start();
    QVERIFY(window.openPath(seedPath));
    QVERIFY2(timer.elapsed() < 1000,
             qPrintable(QStringLiteral("Initial filmstrip took %1 ms")
                            .arg(timer.elapsed())));

    auto *filmstrip =
        window.findChild<QListView *>(QStringLiteral("filmstrip"));
    QVERIFY(filmstrip);
    auto *openedModel =
        qobject_cast<ThumbnailModel *>(filmstrip->model());
    QVERIFY(openedModel);
    QCOMPARE(openedModel->rowCount(), 1);
    QVERIFY(window.openPath(directory.path()));
    QCOMPARE(openedModel->rowCount(), 1);
    QVERIFY(window.openPath(seedPath));
    QCOMPARE(openedModel->rowCount(), 1);

    auto *sourceAction = window.findChild<QAction *>(
        QStringLiteral("filmstripSourceAction"));
    QVERIFY(sourceAction);
    sourceAction->setChecked(false);
    QCoreApplication::processEvents();
    QCOMPARE(filmstrip->model(), openedModel);
    QCOMPARE(filmstrip->horizontalScrollBarPolicy(),
             Qt::ScrollBarAlwaysOff);
    auto *overlayScrollBar = filmstrip->findChild<QScrollBar *>(
        QStringLiteral("filmstripHorizontalScrollBar"));
    QVERIFY(overlayScrollBar);
    QVERIFY(!overlayScrollBar->isVisible());
    const int openedViewportHeight =
        filmstrip->viewport()->height();
    sourceAction->setChecked(true);
    QCoreApplication::processEvents();
    QCOMPARE(filmstrip->viewport()->height(),
             openedViewportHeight);
    auto *model = qobject_cast<ThumbnailModel *>(filmstrip->model());
    QVERIFY(model);
    QVERIFY(model != openedModel);
    QTRY_COMPARE_WITH_TIMEOUT(
        model->rowCount(), imageCount, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(overlayScrollBar->isVisible(), 1000);
    QCOMPARE(overlayScrollBar->height(), 14);
    bool foundDirectoryCount = false;
    for (QLabel *label : window.findChildren<QLabel *>()) {
        if (label->text().contains(QStringLiteral("/400"))) {
            foundDirectoryCount = true;
            break;
        }
    }
    QVERIFY(foundDirectoryCount);
    auto *filmstripDock = window.findChild<QDockWidget *>(
        QStringLiteral("filmstripDock"));
    QVERIFY(filmstripDock);
    const QSize initialThumbnailSize = filmstrip->iconSize();
    window.resizeDocks({filmstripDock}, {190}, Qt::Vertical);
    QTRY_VERIFY_WITH_TIMEOUT(
        filmstrip->iconSize().height() > initialThumbnailSize.height(),
        1000);
    QTRY_COMPARE_WITH_TIMEOUT(
        model->thumbnailSize(), filmstrip->iconSize(), 1000);
    QCOMPARE(filmstrip->itemAlignment(), Qt::Alignment());
    QCOMPARE(filmstrip->gridSize().height(),
             filmstrip->viewport()->height());
    QVERIFY(filmstrip->gridSize().height()
            - filmstrip->iconSize().height()
            >= filmstrip->fontMetrics().height() + 18);
    const QRect firstCell = filmstrip->visualRect(model->index(0));
    const QRect secondCell = filmstrip->visualRect(model->index(1));
    QCOMPARE(firstCell.size(), filmstrip->gridSize());
    QCOMPARE(secondCell.size(), filmstrip->gridSize());
    QCOMPARE(secondCell.left() - firstCell.left(),
             filmstrip->gridSize().width());
    QTest::qWait(150);
    QCoreApplication::processEvents();
    QCOMPARE(filmstrip->visualRect(model->index(0)), firstCell);
    QCOMPARE(filmstrip->visualRect(model->index(1)), secondCell);
    QTRY_VERIFY_WITH_TIMEOUT(
        filmstrip->horizontalScrollBar()->maximum() > 0, 1000);

    QScrollBar *scrollBar = filmstrip->horizontalScrollBar();
    const QRect overlayGeometry = overlayScrollBar->geometry();
    overlayScrollBar->setValue(
        overlayScrollBar->maximum() / 2);
    QCoreApplication::processEvents();
    QCOMPARE(scrollBar->value(), overlayScrollBar->value());
    QCOMPARE(overlayScrollBar->geometry(), overlayGeometry);
    QVERIFY(overlayScrollBar->isVisible());
    scrollBar->setValue(0);
    QWheelEvent wheelEvent(
        QPointF(filmstrip->viewport()->rect().center()),
        QPointF(filmstrip->viewport()->mapToGlobal(
            filmstrip->viewport()->rect().center())),
        QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
        Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(filmstrip->viewport(), &wheelEvent);
    QVERIFY(scrollBar->value() > 0);

    const QModelIndex visibleIndex = model->index(imageCount / 2);
    QVERIFY(visibleIndex.isValid());
    filmstrip->scrollTo(visibleIndex,
                        QAbstractItemView::PositionAtCenter);
    QCoreApplication::processEvents();
    const int positionBeforeSelection = scrollBar->value();
    const QString visibleName =
        model->data(visibleIndex, Qt::DisplayRole).toString();
    auto *toolbar = window.findChild<QToolBar *>(
        QStringLiteral("mainToolbar"));
    QVERIFY(toolbar);
    const QList<QAction *> toolbarActions = toolbar->actions();
    QList<bool> stableActionStates;
    for (int index = 2; index < toolbarActions.size(); ++index)
        stableActionStates.append(toolbarActions.at(index)->isEnabled());
    filmstrip->setCurrentIndex(visibleIndex);
    for (int index = 2; index < toolbarActions.size(); ++index) {
        if (stableActionStates.at(index - 2))
            QVERIFY(toolbarActions.at(index)->isEnabled());
    }
    QTRY_VERIFY_WITH_TIMEOUT(
        window.windowTitle().contains(visibleName), 3000);
    QCOMPARE(openedModel->rowCount(), 1);
    QVERIFY(std::abs(scrollBar->value() - positionBeforeSelection)
            < filmstrip->gridSize().width());
    QVERIFY(QMetaObject::invokeMethod(
        filmstrip, "doubleClicked", Qt::DirectConnection,
        Q_ARG(QModelIndex, visibleIndex)));
    QTRY_COMPARE_WITH_TIMEOUT(openedModel->rowCount(), 2, 1000);

    timer.restart();
    filmstrip->setCurrentIndex(model->index(100));
    filmstrip->setCurrentIndex(model->index(250));
    filmstrip->setCurrentIndex(model->index(imageCount - 1));
    QCoreApplication::processEvents();
    QVERIFY2(timer.elapsed() < 500,
             qPrintable(QStringLiteral("Folder thumbnail switch took %1 ms")
                            .arg(timer.elapsed())));
    QTRY_VERIFY_WITH_TIMEOUT(
        window.windowTitle().contains(QStringLiteral("image399.png")),
        3000);
    QCOMPARE(model->rowCount(), imageCount);
    QCOMPARE(openedModel->rowCount(), 2);

    const int directoryScrollPosition = scrollBar->value();
    timer.restart();
    sourceAction->setChecked(false);
    QCoreApplication::processEvents();
    QCOMPARE(filmstrip->model(), openedModel);
    sourceAction->setChecked(true);
    QCoreApplication::processEvents();
    QVERIFY2(timer.elapsed() < 200,
             qPrintable(QStringLiteral(
                 "Persistent source switch took %1 ms")
                 .arg(timer.elapsed())));
    QCOMPARE(filmstrip->model(), model);
    QTRY_COMPARE_WITH_TIMEOUT(
        scrollBar->value(), directoryScrollPosition, 1000);
    QCOMPARE(model->rowCount(), imageCount);

    sourceAction->setChecked(false);
    QCoreApplication::processEvents();
    const int openedCountBeforeClose = openedModel->rowCount();
    QVERIFY(openedCountBeforeClose > 1);
    const QModelIndex closeIndex = openedModel->index(0);
    const QRect closeItemRect = filmstrip->visualRect(closeIndex);
    QVERIFY(closeItemRect.isValid());
    const QPoint closePoint(
        closeItemRect.right() - 14,
        closeItemRect.top() + 16);
    QTest::mouseMove(filmstrip->viewport(), closePoint);
    QTest::mouseClick(filmstrip->viewport(), Qt::LeftButton,
                      Qt::NoModifier, closePoint);
    QTRY_COMPARE_WITH_TIMEOUT(
        openedModel->rowCount(), openedCountBeforeClose - 1, 1000);
    QVERIFY(window.windowTitle().contains(
        QStringLiteral("image399.png")));
}

QTEST_MAIN(CoreTest)
#include "application_test.moc"
