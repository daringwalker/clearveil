#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

class QSettings;

struct ApplicationSettings
{
    static constexpr int minimumSlideshowIntervalMs = 1'000;
    static constexpr int maximumSlideshowIntervalMs = 60'000;
    static constexpr int minimumThumbnailCacheMiB = 64;
    static constexpr int maximumThumbnailCacheMiB = 65'536;
    static constexpr int minimumImageMemoryCacheMiB = 16;
    static constexpr int maximumImageMemoryCacheMiB = 4'096;
    static constexpr int minimumFilmstripThumbnailExtent = 48;
    static constexpr int maximumFilmstripThumbnailExtent = 256;
    static constexpr int minimumFilmstripVerticalColumns = 1;
    static constexpr int maximumFilmstripVerticalColumns = 4;

    QString theme = QStringLiteral("system");
    QString language = QStringLiteral("system");
    QStringList toolbarLayout;
    QStringList shortcutLayout;
    QStringList recentFolders;
    QStringList favoriteFolders;
    QStringList panelOrder{
        QStringLiteral("thumbnails"),
        QStringLiteral("information"),
        QStringLiteral("colorPicker")};

    QString wheelAction = QStringLiteral("scroll");
    QString ctrlWheelAction = QStringLiteral("zoom");
    QString doubleClickAction = QStringLiteral("toggle_zoom");
    QString middleButtonAction = QStringLiteral("none");
    QString backButtonAction = QStringLiteral("previous");
    QString forwardButtonAction = QStringLiteral("next");
    QString directoryThumbnailSortKey = QStringLiteral("name");
    QString floatingThumbnailLayout = QStringLiteral("auto");

    bool persistentThumbnailCacheEnabled = false;
    int persistentThumbnailCacheMiB = 512;
    int imageMemoryCacheMiB = 256;
    int slideshowIntervalMs = 3'000;
    bool showToolbar = true;
    bool showFilmstrip = true;
    bool showInformation = false;
    bool showColorPicker = false;
    bool showFilmstripFileNames = true;
    int filmstripThumbnailExtent = 256;
    int filmstripVerticalColumns = 1;
    bool directoryThumbnailSortAscending = true;
    bool showTransparencyCheckerboard = true;
    bool showMenuBar = false;
    bool showStatusBar = true;
    bool randomSlideshow = false;
    bool fullscreenSlideshow = false;
    bool layoutLocked = true;
    bool fitWindowToImage = false;
    bool borderless = false;
    bool alwaysOnTop = false;
    bool showToolbarInFullscreen = false;
    bool showFilmstripInFullscreen = false;
    bool showStatusBarInFullscreen = false;
    bool showInformationInFullscreen = false;

    QByteArray windowGeometry;
    QByteArray windowState;

    static ApplicationSettings load(QSettings &settings);
    void save(QSettings &settings) const;
    void normalize();
};
