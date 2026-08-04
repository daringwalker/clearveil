#include "applicationsettings.h"

#include "foldernavigationcontroller.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QSettings>

#include <algorithm>

namespace {
QString normalizedChoice(const QString &value,
                         const QSet<QString> &allowed,
                         const QString &fallback)
{
    return allowed.contains(value) ? value : fallback;
}

QStringList normalizedDirectories(
    const QStringList &directories, int maximumCount)
{
    QStringList result;
    for (const QString &path : directories) {
        if (path.trimmed().isEmpty())
            continue;
        const QString normalized = QDir::cleanPath(
            QFileInfo(path).absoluteFilePath());
        if (result.contains(normalized))
            continue;
        result.append(normalized);
        if (result.size() >= maximumCount)
            break;
    }
    return result;
}

QStringList normalizedPanelOrder(const QStringList &order)
{
    const QStringList defaults{
        QStringLiteral("thumbnails"),
        QStringLiteral("information"),
        QStringLiteral("colorPicker")};
    QStringList result;
    for (const QString &panelId : order) {
        if (defaults.contains(panelId) && !result.contains(panelId))
            result.append(panelId);
    }
    for (const QString &panelId : defaults) {
        if (!result.contains(panelId))
            result.append(panelId);
    }
    return result;
}
}

ApplicationSettings ApplicationSettings::load(
    QSettings &settings)
{
    ApplicationSettings result;
    result.theme = settings.value(
        QStringLiteral("appearance/theme"), result.theme).toString();
    result.language = settings.value(
        QStringLiteral("ui/language"), result.language).toString();
    result.toolbarLayout = settings.value(
        QStringLiteral("ui/toolbarLayout")).toStringList();
    result.shortcutLayout = settings.value(
        QStringLiteral("ui/shortcutLayout")).toStringList();
    result.recentFolders = settings.value(
        QStringLiteral("browser/recentFolders")).toStringList();
    result.favoriteFolders = settings.value(
        QStringLiteral("browser/favoriteFolders")).toStringList();
    result.panelOrder = settings.value(
        QStringLiteral("layout/panelOrder"),
        result.panelOrder).toStringList();

    result.wheelAction = settings.value(
        QStringLiteral("input/mouseWheelAction"),
        result.wheelAction).toString();
    result.ctrlWheelAction = settings.value(
        QStringLiteral("input/ctrlMouseWheelAction"),
        result.ctrlWheelAction).toString();
    result.doubleClickAction = settings.value(
        QStringLiteral("input/doubleClickAction"),
        result.doubleClickAction).toString();
    result.middleButtonAction = settings.value(
        QStringLiteral("input/middleButtonAction"),
        result.middleButtonAction).toString();
    result.backButtonAction = settings.value(
        QStringLiteral("input/backButtonAction"),
        result.backButtonAction).toString();
    result.forwardButtonAction = settings.value(
        QStringLiteral("input/forwardButtonAction"),
        result.forwardButtonAction).toString();
    result.directoryThumbnailSortKey = settings.value(
        QStringLiteral("filmstrip/directorySortKey"),
        result.directoryThumbnailSortKey).toString();
    result.floatingThumbnailLayout = settings.value(
        QStringLiteral("filmstrip/floatingLayout"),
        result.floatingThumbnailLayout).toString();

    result.persistentThumbnailCacheEnabled = settings.value(
        QStringLiteral("thumbnails/persistentCacheEnabled"),
        result.persistentThumbnailCacheEnabled).toBool();
    result.persistentThumbnailCacheMiB = settings.value(
        QStringLiteral("thumbnails/persistentCacheMiB"),
        result.persistentThumbnailCacheMiB).toInt();
    result.imageMemoryCacheMiB = settings.value(
        QStringLiteral("performance/imageMemoryCacheMiB"),
        result.imageMemoryCacheMiB).toInt();
    result.slideshowIntervalMs = settings.value(
        QStringLiteral("slideshow/intervalMs"),
        result.slideshowIntervalMs).toInt();
    result.showToolbar = settings.value(
        QStringLiteral("view/showToolbar"),
        result.showToolbar).toBool();
    result.showFilmstrip = settings.value(
        QStringLiteral("view/showFilmstrip"),
        result.showFilmstrip).toBool();
    result.showInformation = settings.value(
        QStringLiteral("view/showInformation"),
        result.showInformation).toBool();
    result.showColorPicker = settings.value(
        QStringLiteral("view/showColorPicker"),
        result.showColorPicker).toBool();
    result.showFilmstripFileNames = settings.value(
        QStringLiteral("filmstrip/showFileNames"),
        result.showFilmstripFileNames).toBool();
    result.filmstripThumbnailExtent = settings.value(
        QStringLiteral("filmstrip/thumbnailExtent"),
        result.filmstripThumbnailExtent).toInt();
    result.filmstripVerticalColumns = settings.value(
        QStringLiteral("filmstrip/verticalColumns"),
        result.filmstripVerticalColumns).toInt();
    result.directoryThumbnailSortAscending = settings.value(
        QStringLiteral("filmstrip/directorySortAscending"),
        result.directoryThumbnailSortAscending).toBool();
    result.showTransparencyCheckerboard = settings.value(
        QStringLiteral("appearance/showTransparencyCheckerboard"),
        result.showTransparencyCheckerboard).toBool();
    result.showMenuBar = settings.value(
        QStringLiteral("view/showMenuBar"),
        result.showMenuBar).toBool();
    result.showStatusBar = settings.value(
        QStringLiteral("view/showStatusBar"),
        result.showStatusBar).toBool();
    result.randomSlideshow = settings.value(
        QStringLiteral("slideshow/random"),
        result.randomSlideshow).toBool();
    result.fullscreenSlideshow = settings.value(
        QStringLiteral("slideshow/fullscreen"),
        result.fullscreenSlideshow).toBool();
    result.layoutLocked = settings.value(
        QStringLiteral("view/layoutLocked"),
        result.layoutLocked).toBool();
    result.fitWindowToImage = settings.value(
        QStringLiteral("window/fitToImage"),
        result.fitWindowToImage).toBool();
    result.borderless = settings.value(
        QStringLiteral("window/borderless"),
        result.borderless).toBool();
    result.alwaysOnTop = settings.value(
        QStringLiteral("window/alwaysOnTop"),
        result.alwaysOnTop).toBool();
    result.showToolbarInFullscreen = settings.value(
        QStringLiteral("fullscreen/showToolbar"),
        result.showToolbarInFullscreen).toBool();
    result.showFilmstripInFullscreen = settings.value(
        QStringLiteral("fullscreen/showFilmstrip"),
        result.showFilmstripInFullscreen).toBool();
    result.showStatusBarInFullscreen = settings.value(
        QStringLiteral("fullscreen/showStatusBar"),
        result.showStatusBarInFullscreen).toBool();
    result.showInformationInFullscreen = settings.value(
        QStringLiteral("fullscreen/showInformation"),
        result.showInformationInFullscreen).toBool();
    result.windowGeometry = settings.value(
        QStringLiteral("window/geometry")).toByteArray();
    result.windowState = settings.value(
        QStringLiteral("window/state")).toByteArray();
    result.normalize();
    return result;
}

void ApplicationSettings::save(QSettings &settings) const
{
    ApplicationSettings normalized = *this;
    normalized.normalize();
    settings.setValue(QStringLiteral("appearance/theme"),
                      normalized.theme);
    settings.setValue(QStringLiteral("ui/language"),
                      normalized.language);
    settings.setValue(QStringLiteral("ui/toolbarLayout"),
                      normalized.toolbarLayout);
    settings.setValue(QStringLiteral("ui/shortcutLayout"),
                      normalized.shortcutLayout);
    settings.setValue(QStringLiteral("browser/recentFolders"),
                      normalized.recentFolders);
    settings.setValue(QStringLiteral("browser/favoriteFolders"),
                      normalized.favoriteFolders);
    settings.setValue(QStringLiteral("layout/panelOrder"),
                      normalized.panelOrder);
    settings.setValue(QStringLiteral("input/mouseWheelAction"),
                      normalized.wheelAction);
    settings.setValue(QStringLiteral("input/ctrlMouseWheelAction"),
                      normalized.ctrlWheelAction);
    settings.setValue(QStringLiteral("input/doubleClickAction"),
                      normalized.doubleClickAction);
    settings.setValue(QStringLiteral("input/middleButtonAction"),
                      normalized.middleButtonAction);
    settings.setValue(QStringLiteral("input/backButtonAction"),
                      normalized.backButtonAction);
    settings.setValue(QStringLiteral("input/forwardButtonAction"),
                      normalized.forwardButtonAction);
    settings.setValue(QStringLiteral("filmstrip/directorySortKey"),
                      normalized.directoryThumbnailSortKey);
    settings.setValue(QStringLiteral("filmstrip/floatingLayout"),
                      normalized.floatingThumbnailLayout);
    settings.setValue(QStringLiteral("thumbnails/persistentCacheEnabled"),
                      normalized.persistentThumbnailCacheEnabled);
    settings.setValue(QStringLiteral("thumbnails/persistentCacheMiB"),
                      normalized.persistentThumbnailCacheMiB);
    settings.setValue(QStringLiteral("performance/imageMemoryCacheMiB"),
                      normalized.imageMemoryCacheMiB);
    settings.setValue(QStringLiteral("slideshow/intervalMs"),
                      normalized.slideshowIntervalMs);
    settings.setValue(QStringLiteral("view/showToolbar"),
                      normalized.showToolbar);
    settings.setValue(QStringLiteral("view/showFilmstrip"),
                      normalized.showFilmstrip);
    settings.setValue(QStringLiteral("view/showInformation"),
                      normalized.showInformation);
    settings.setValue(QStringLiteral("view/showColorPicker"),
                      normalized.showColorPicker);
    settings.setValue(QStringLiteral("filmstrip/showFileNames"),
                      normalized.showFilmstripFileNames);
    settings.setValue(QStringLiteral("filmstrip/thumbnailExtent"),
                      normalized.filmstripThumbnailExtent);
    settings.setValue(QStringLiteral("filmstrip/verticalColumns"),
                      normalized.filmstripVerticalColumns);
    settings.setValue(
        QStringLiteral("filmstrip/directorySortAscending"),
        normalized.directoryThumbnailSortAscending);
    settings.setValue(
        QStringLiteral("appearance/showTransparencyCheckerboard"),
        normalized.showTransparencyCheckerboard);
    settings.setValue(QStringLiteral("view/showMenuBar"),
                      normalized.showMenuBar);
    settings.setValue(QStringLiteral("view/showStatusBar"),
                      normalized.showStatusBar);
    settings.setValue(QStringLiteral("slideshow/random"),
                      normalized.randomSlideshow);
    settings.setValue(QStringLiteral("slideshow/fullscreen"),
                      normalized.fullscreenSlideshow);
    settings.setValue(QStringLiteral("view/layoutLocked"),
                      normalized.layoutLocked);
    settings.setValue(QStringLiteral("window/fitToImage"),
                      normalized.fitWindowToImage);
    settings.setValue(QStringLiteral("window/borderless"),
                      normalized.borderless);
    settings.setValue(QStringLiteral("window/alwaysOnTop"),
                      normalized.alwaysOnTop);
    settings.setValue(QStringLiteral("fullscreen/showToolbar"),
                      normalized.showToolbarInFullscreen);
    settings.setValue(QStringLiteral("fullscreen/showFilmstrip"),
                      normalized.showFilmstripInFullscreen);
    settings.setValue(QStringLiteral("fullscreen/showStatusBar"),
                      normalized.showStatusBarInFullscreen);
    settings.setValue(QStringLiteral("fullscreen/showInformation"),
                      normalized.showInformationInFullscreen);
    settings.setValue(QStringLiteral("window/geometry"),
                      normalized.windowGeometry);
    settings.setValue(QStringLiteral("window/state"),
                      normalized.windowState);
}

void ApplicationSettings::normalize()
{
    static const QSet<QString> themes{
        QStringLiteral("system"),
        QStringLiteral("light"),
        QStringLiteral("dark")
    };
    static const QSet<QString> languages{
        QStringLiteral("system"),
        QStringLiteral("zh_CN"),
        QStringLiteral("en")
    };
    static const QSet<QString> wheelActions{
        QStringLiteral("scroll"),
        QStringLiteral("zoom"),
        QStringLiteral("navigate"),
        QStringLiteral("none")
    };
    static const QSet<QString> pointerActions{
        QStringLiteral("none"),
        QStringLiteral("toggle_zoom"),
        QStringLiteral("fullscreen"),
        QStringLiteral("previous"),
        QStringLiteral("next"),
        QStringLiteral("fit"),
        QStringLiteral("actual_size"),
        QStringLiteral("slideshow")
    };
    static const QSet<QString> thumbnailSortKeys{
        QStringLiteral("name"),
        QStringLiteral("modified"),
        QStringLiteral("size"),
        QStringLiteral("type")
    };
    static const QSet<QString> floatingThumbnailLayouts{
        QStringLiteral("auto"),
        QStringLiteral("horizontal"),
        QStringLiteral("vertical")
    };

    theme = normalizedChoice(theme, themes, QStringLiteral("system"));
    language = normalizedChoice(
        language, languages, QStringLiteral("system"));
    wheelAction = normalizedChoice(
        wheelAction, wheelActions, QStringLiteral("scroll"));
    ctrlWheelAction = normalizedChoice(
        ctrlWheelAction, wheelActions, QStringLiteral("zoom"));
    doubleClickAction = normalizedChoice(
        doubleClickAction, pointerActions,
        QStringLiteral("toggle_zoom"));
    middleButtonAction = normalizedChoice(
        middleButtonAction, pointerActions, QStringLiteral("none"));
    backButtonAction = normalizedChoice(
        backButtonAction, pointerActions, QStringLiteral("previous"));
    forwardButtonAction = normalizedChoice(
        forwardButtonAction, pointerActions, QStringLiteral("next"));
    directoryThumbnailSortKey = normalizedChoice(
        directoryThumbnailSortKey, thumbnailSortKeys,
        QStringLiteral("name"));
    floatingThumbnailLayout = normalizedChoice(
        floatingThumbnailLayout, floatingThumbnailLayouts,
        QStringLiteral("auto"));
    recentFolders = normalizedDirectories(
        recentFolders,
        FolderNavigationController::maximumRecentDirectories);
    favoriteFolders = normalizedDirectories(
        favoriteFolders,
        FolderNavigationController::maximumFavoriteDirectories);
    panelOrder = normalizedPanelOrder(panelOrder);
    persistentThumbnailCacheMiB = std::clamp(
        persistentThumbnailCacheMiB,
        minimumThumbnailCacheMiB, maximumThumbnailCacheMiB);
    imageMemoryCacheMiB = std::clamp(
        imageMemoryCacheMiB,
        minimumImageMemoryCacheMiB, maximumImageMemoryCacheMiB);
    slideshowIntervalMs = std::clamp(
        slideshowIntervalMs,
        minimumSlideshowIntervalMs, maximumSlideshowIntervalMs);
    filmstripThumbnailExtent = std::clamp(
        filmstripThumbnailExtent,
        minimumFilmstripThumbnailExtent,
        maximumFilmstripThumbnailExtent);
    filmstripVerticalColumns = std::clamp(
        filmstripVerticalColumns,
        minimumFilmstripVerticalColumns,
        maximumFilmstripVerticalColumns);
}
