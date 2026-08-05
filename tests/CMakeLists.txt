add_executable(clearveil_tests
    integration/application_test.cpp
    "${PROJECT_SOURCE_DIR}/src/resources.qrc"
    "${PROJECT_SOURCE_DIR}/src/app/mainwindow.cpp"
    "${PROJECT_SOURCE_DIR}/src/app/mainwindow.h"
    "${PROJECT_SOURCE_DIR}/src/app/singleinstance.cpp"
    "${PROJECT_SOURCE_DIR}/src/app/singleinstance.h"
)
target_include_directories(clearveil_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/app")
target_link_libraries(clearveil_tests PRIVATE clearveil_core
    Qt6::DBus Qt6::Network Qt6::PrintSupport Qt6::Test)
clearveil_enable_warnings(clearveil_tests)

add_test(NAME clearveil_tests COMMAND clearveil_tests)
add_test(NAME clearveil_regression_navigation
    COMMAND clearveil_tests
        imageLoaderKeepsLatestRequestAndReusesCache
        viewerNavigationOwnsPendingActivationAndPrefetch
        folderSlideshowActivatesItsThumbnail
        largeDirectoryOpenReturnsBeforeScanCompletes)
add_test(NAME clearveil_regression_filmstrip
    COMMAND clearveil_tests
        directoryFilmstripRefreshesIncrementally
        folderOpenUsesViewerAndOverviewAvoidsDuplicateStrip
        thumbnailStripSettingsControlCachesAndLayout
        largeFolderFilmstripSwitchDoesNotRebuildThumbnails)
add_test(NAME clearveil_regression_layout
    COMMAND clearveil_tests
        browserRefreshesItsViewportPalette
        systemAppearanceResolvesPortalColorScheme
        breezeThemesMatchKdeReferenceColors
        mainToolbarCompactsWithoutOverflow
        windowChromePopupAndDragRegionsAreUsable
        settingsDialogAppliesWithoutClosing
        preferencesApplyRecolorsMenuBar
        fullscreenComponentsHideAndRestore
        interfaceLayoutPageCentralizesPanelChanges
        interfaceLayoutPreviewSupportsDragRearrangement
        panelLayoutControllerOwnsFloatingState
        colorPickerMigratesFromSideDockToCompactOverlay
        panelLayoutControllerAnchorsOverlayToMainWindow
        panelLayoutControllerSnapsOverlayPanelsTogether
        floatingPanelsPersistAndSurviveLayoutLock)
add_test(NAME clearveil_regression_input
    COMMAND clearveil_tests
        inputContextPolicyReservesLocalShortcuts
        largeImageSampleControllerDropsStaleResults
        mouseActionsEmitConfiguredCommands
        ocrTextSelectionPreservesReadingOrder
        ocrEngineRecognizesRenderedText
        ocrEngineRecognizesInstalledChineseModel
        ocrCanvasSupportsMouseSelection
        mainWindowOcrWorkflowCopiesRecognizedText
        canvasGesturesZoomAndPanIncrementally
        metadataPanelShowsPresentEmbeddedMetadata
        viewerStatusExposesAccessibleNames
        singleInstanceForwardsExactlyOnce)

set_tests_properties(
    clearveil_tests
    clearveil_regression_navigation
    clearveil_regression_filmstrip
    clearveil_regression_layout
    clearveil_regression_input
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
set_tests_properties(
    clearveil_regression_navigation
    clearveil_regression_filmstrip
    clearveil_regression_layout
    clearveil_regression_input
    PROPERTIES LABELS "regression")

find_program(APPSTREAMCLI_EXECUTABLE NAMES appstreamcli)
if(APPSTREAMCLI_EXECUTABLE)
    add_test(NAME clearveil_packaging_appstream
        COMMAND "${APPSTREAMCLI_EXECUTABLE}" validate --pedantic --no-net
            "${PROJECT_SOURCE_DIR}/packaging/io.github.daringwalker.clearveil.metainfo.xml")
    set_tests_properties(clearveil_packaging_appstream
        PROPERTIES LABELS "packaging")
endif()

find_program(DESKTOP_FILE_VALIDATE_EXECUTABLE
    NAMES desktop-file-validate)
if(DESKTOP_FILE_VALIDATE_EXECUTABLE)
    add_test(NAME clearveil_packaging_desktop
        COMMAND "${DESKTOP_FILE_VALIDATE_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/packaging/io.github.daringwalker.clearveil.desktop")
    set_tests_properties(clearveil_packaging_desktop
        PROPERTIES LABELS "packaging")
endif()

find_program(FLATPAK_BUILDER_EXECUTABLE NAMES flatpak-builder)
if(FLATPAK_BUILDER_EXECUTABLE)
    add_test(NAME clearveil_packaging_flatpak_manifest
        COMMAND "${FLATPAK_BUILDER_EXECUTABLE}" --show-manifest
            "${PROJECT_SOURCE_DIR}/packaging/io.github.daringwalker.clearveil.yml")
    set_tests_properties(clearveil_packaging_flatpak_manifest
        PROPERTIES LABELS "packaging")
endif()

add_test(NAME clearveil_packaging_release_metadata
    COMMAND bash "${PROJECT_SOURCE_DIR}/tools/check_release_metadata.sh")
set_tests_properties(clearveil_packaging_release_metadata
    PROPERTIES
        LABELS "packaging"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
