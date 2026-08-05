cmake_minimum_required(VERSION 3.24)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CLEARVEIL_VERSION_LABEL)
string(STRIP "${CLEARVEIL_VERSION_LABEL}" CLEARVEIL_VERSION_LABEL)
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+"
    CLEARVEIL_PROJECT_VERSION "${CLEARVEIL_VERSION_LABEL}")
if(NOT CLEARVEIL_PROJECT_VERSION)
    message(FATAL_ERROR "VERSION must start with a numeric semantic version")
endif()

project(clearveil
    VERSION "${CLEARVEIL_PROJECT_VERSION}"
    DESCRIPTION "Clearveil image viewer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
include(CTest)
include(cmake/ClearveilWarnings.cmake)
option(CLEARVEIL_BUILD_BENCHMARKS
    "Build deterministic performance benchmark tools" OFF)
option(CLEARVEIL_WARNINGS_AS_ERRORS
    "Treat compiler warnings as errors" OFF)
option(CLEARVEIL_ENABLE_OCR
    "Enable Tesseract-backed image text recognition when available" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Concurrent Core DBus Gui Network PrintSupport Svg Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIV2 QUIET IMPORTED_TARGET exiv2)
pkg_check_modules(COLORD QUIET IMPORTED_TARGET colord)
if(CLEARVEIL_ENABLE_OCR)
    pkg_check_modules(TESSERACT QUIET IMPORTED_TARGET tesseract)
endif()
pkg_check_modules(VIPS REQUIRED IMPORTED_TARGET vips)

add_library(clearveil_core
    src/ui/dialogs/aboutdialog.cpp
    src/ui/dialogs/aboutdialog.h
    src/app/actionregistry.cpp
    src/app/actionregistry.h
    src/app/applicationsettings.cpp
    src/app/applicationsettings.h
    src/browser/browserwidget.cpp
    src/browser/browserwidget.h
    src/browser/browserfileoperationscontroller.cpp
    src/browser/browserfileoperationscontroller.h
    src/viewer/canvasgesturecontroller.cpp
    src/viewer/canvasgesturecontroller.h
    src/viewer/canvasappearancecontroller.cpp
    src/viewer/canvasappearancecontroller.h
    src/ui/clearveilicon.cpp
    src/ui/clearveilicon.h
    src/ui/inputcontextpolicy.cpp
    src/ui/inputcontextpolicy.h
    src/ui/breezetheme.cpp
    src/ui/breezetheme.h
    src/viewer/comparewidget.cpp
    src/viewer/comparewidget.h
    src/viewer/colorpickercontroller.cpp
    src/viewer/colorpickercontroller.h
    src/ui/panels/colorpickerpanel.cpp
    src/ui/panels/colorpickerpanel.h
    src/platform/desktopintegration.cpp
    src/platform/desktopintegration.h
    src/browser/directorymonitor.cpp
    src/browser/directorymonitor.h
    src/browser/directoryscanservice.cpp
    src/browser/directoryscanservice.h
    src/imaging/displaycolor.cpp
    src/imaging/displaycolor.h
    src/imaging/displaycolorcontroller.cpp
    src/imaging/displaycolorcontroller.h
    src/workflows/documentworkflowcontroller.cpp
    src/workflows/documentworkflowcontroller.h
    src/ui/dialogs/editdialogs.cpp
    src/ui/dialogs/editdialogs.h
    src/filmstrip/filmstripcontroller.cpp
    src/filmstrip/filmstripcontroller.h
    src/filmstrip/filmstriplayoutcontroller.cpp
    src/filmstrip/filmstriplayoutcontroller.h
    src/filmstrip/filmstripview.cpp
    src/filmstrip/filmstripview.h
    src/browser/fileoperations.cpp
    src/browser/fileoperations.h
    src/browser/foldernavigationcontroller.cpp
    src/browser/foldernavigationcontroller.h
    src/imaging/framecontroller.cpp
    src/imaging/framecontroller.h
    src/imaging/formatcapabilities.cpp
    src/imaging/formatcapabilities.h
    src/ui/dialogs/formatcapabilitiesdialog.cpp
    src/ui/dialogs/formatcapabilitiesdialog.h
    src/viewer/imagecanvas.cpp
    src/viewer/imagecanvas.h
    src/imaging/imagesource.cpp
    src/imaging/imagesource.h
    src/imaging/imagedecoder.cpp
    src/imaging/imagedecoder.h
    src/imaging/ocrengine.cpp
    src/imaging/ocrengine.h
    src/imaging/ocrsupport.cpp
    src/imaging/ocrsupport.h
    src/ui/dialogs/ocrsupportdialog.cpp
    src/ui/dialogs/ocrsupportdialog.h
    src/imaging/ocrfallbackdetector.cpp
    src/imaging/ocrfallbackdetector.h
    src/domain/ocrresult.h
    src/workflows/imageeditcontroller.cpp
    src/workflows/imageeditcontroller.h
    src/workflows/imageloadcontroller.cpp
    src/workflows/imageloadcontroller.h
    src/imaging/largeimagesamplecontroller.cpp
    src/imaging/largeimagesamplecontroller.h
    src/viewer/ocrtextselectionmodel.cpp
    src/viewer/ocrtextselectionmodel.h
    src/imaging/imageexportservice.cpp
    src/imaging/imageexportservice.h
    src/ui/panels/interfacelayoutpage.cpp
    src/ui/panels/interfacelayoutpage.h
    src/domain/imagedocument.cpp
    src/domain/imagedocument.h
    src/workflows/imagesessioncontroller.cpp
    src/workflows/imagesessioncontroller.h
    src/domain/imagesequence.cpp
    src/domain/imagesequence.h
    src/ui/panels/metadatapanel.cpp
    src/ui/panels/metadatapanel.h
    src/ui/panels/panellayoutcontroller.cpp
    src/ui/panels/panellayoutcontroller.h
    src/ui/panels/paneltitlebar.cpp
    src/ui/panels/paneltitlebar.h
    src/ui/selectablelabel.cpp
    src/ui/selectablelabel.h
    src/ui/selectablestatusbar.cpp
    src/ui/selectablestatusbar.h
    src/filmstrip/persistentthumbnailcache.cpp
    src/filmstrip/persistentthumbnailcache.h
    src/workflows/slideshowcontroller.cpp
    src/workflows/slideshowcontroller.h
    src/workflows/ocrcontroller.cpp
    src/workflows/ocrcontroller.h
    src/platform/systemappearancecontroller.cpp
    src/platform/systemappearancecontroller.h
    src/filmstrip/thumbnailmodel.cpp
    src/filmstrip/thumbnailmodel.h
    src/viewer/tiledimageviewmodel.cpp
    src/viewer/tiledimageviewmodel.h
    src/viewer/viewernavigationcontroller.cpp
    src/viewer/viewernavigationcontroller.h
    src/viewer/vieweruistatecontroller.cpp
    src/viewer/vieweruistatecontroller.h
    src/imaging/vipsimagesource.cpp
    src/imaging/vipsimagesource.h
    src/platform/windowdragcontroller.cpp
    src/platform/windowdragcontroller.h
    src/platform/windowmodecontroller.cpp
    src/platform/windowmodecontroller.h
)
target_link_libraries(clearveil_core PUBLIC Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::Svg Qt6::Widgets
    PRIVATE Qt6::DBus Qt6::PrintSupport PkgConfig::VIPS)
if(TARGET PkgConfig::EXIV2)
    target_link_libraries(clearveil_core PRIVATE PkgConfig::EXIV2)
    target_compile_definitions(clearveil_core PRIVATE CLEARVEIL_HAVE_EXIV2=1)
endif()
if(TARGET PkgConfig::COLORD)
    target_link_libraries(clearveil_core PRIVATE PkgConfig::COLORD)
    target_compile_definitions(clearveil_core PRIVATE CLEARVEIL_HAVE_COLORD=1)
endif()
if(TARGET PkgConfig::TESSERACT)
    target_link_libraries(clearveil_core PRIVATE PkgConfig::TESSERACT)
    target_compile_definitions(clearveil_core PRIVATE CLEARVEIL_HAVE_TESSERACT=1)
endif()
target_include_directories(clearveil_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/browser"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/domain"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/filmstrip"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/imaging"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/panels"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/viewer"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/workflows")
target_compile_definitions(clearveil_core PUBLIC
    CLEARVEIL_VERSION="${CLEARVEIL_VERSION_LABEL}")
clearveil_enable_warnings(clearveil_core)

add_executable(clearveil
    src/resources.qrc
    src/app/main.cpp
    src/app/mainwindow.cpp
    src/app/mainwindow.h
    src/app/singleinstance.cpp
    src/app/singleinstance.h
)
target_include_directories(clearveil PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app")
target_link_libraries(clearveil PRIVATE clearveil_core Qt6::DBus Qt6::Network Qt6::PrintSupport)
clearveil_enable_warnings(clearveil)

find_program(LRELEASE_EXECUTABLE
    NAMES lrelease6 lrelease
    HINTS
        /usr/lib/qt6/bin
        /usr/lib64/qt6/bin
    DOC "Qt 6 translation compiler"
    REQUIRED
)
set(CLEARVEIL_ZH_QM "${CMAKE_CURRENT_BINARY_DIR}/clearveil_zh_CN.qm")
add_custom_command(
    OUTPUT "${CLEARVEIL_ZH_QM}"
    COMMAND "${LRELEASE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/translations/clearveil_zh_CN.ts"
            -qm "${CLEARVEIL_ZH_QM}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/translations/clearveil_zh_CN.ts"
    VERBATIM
)
set_source_files_properties("${CLEARVEIL_ZH_QM}" PROPERTIES
    GENERATED TRUE
    QT_RESOURCE_ALIAS "clearveil_zh_CN.qm"
)
qt_add_resources(clearveil "clearveil_translations"
    PREFIX "/i18n"
    FILES "${CLEARVEIL_ZH_QM}"
)

install(TARGETS clearveil RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES packaging/io.github.daringwalker.clearveil.desktop
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES packaging/io.github.daringwalker.clearveil.metainfo.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
install(FILES packaging/io.github.daringwalker.clearveil.svg
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
install(FILES
    LICENSE
    LICENSES/CC0-1.0.txt
    COPYRIGHT.md
    THIRD_PARTY.md
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/clearveil")
install(FILES packaging/clearveil.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

include(cmake/ClearveilPackaging.cmake)

if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

if(CLEARVEIL_BUILD_BENCHMARKS)
    add_executable(clearveil_thumbnail_benchmark
        benchmarks/thumbnail_benchmark.cpp
    )
    target_link_libraries(clearveil_thumbnail_benchmark
        PRIVATE clearveil_core Qt6::Widgets)
    clearveil_enable_warnings(clearveil_thumbnail_benchmark)

    add_executable(clearveil_image_decode_benchmark
        benchmarks/image_decode_benchmark.cpp
    )
    target_link_libraries(clearveil_image_decode_benchmark
        PRIVATE clearveil_core Qt6::Core Qt6::Gui)
    clearveil_enable_warnings(clearveil_image_decode_benchmark)

    add_executable(clearveil_ocr_benchmark
        benchmarks/ocr_benchmark.cpp
    )
    target_link_libraries(clearveil_ocr_benchmark
        PRIVATE clearveil_core Qt6::Core Qt6::Gui)
    clearveil_enable_warnings(clearveil_ocr_benchmark)
endif()
