#pragma once

#include <QWidget>
#include <QHash>

#include "directoryscanservice.h"

class QLineEdit;
class QListView;
class QMenu;
class QSlider;
class QSortFilterProxyModel;
class FolderNavigationController;
class ThumbnailModel;
class QToolButton;

class BrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BrowserWidget(QWidget *parent = nullptr);
    BrowserWidget(DirectoryScanService *scanService,
                  QWidget *parent);

    void setDirectory(const QString &directoryPath);
    void setCurrentPath(const QString &filePath);
    void refreshAppearance();
    void refreshDirectory();
    void applyDirectoryResult(
        const DirectoryScanResult &result);
    [[nodiscard]] QString directoryPath() const;
    [[nodiscard]] QStringList selectedImageFiles() const;
    [[nodiscard]] int imageCount() const;
    void setStoredLocations(
        const QStringList &recentDirectories,
        const QStringList &favoriteDirectories);
    [[nodiscard]] QStringList recentDirectories() const;
    [[nodiscard]] QStringList favoriteDirectories() const;

signals:
    void imageActivated(const QString &filePath);
    void addToOpenedRequested(const QStringList &filePaths);
    void revealRequested(const QString &filePath);
    void fileOperationRequested(
        const QString &operationId,
        const QStringList &filePaths);
    void compareRequested(const QStringList &filePaths);
    void directoryChanged(const QString &directoryPath);
    void backRequested();
    void scanFailed(const QString &directoryPath,
                    const QString &error);

private:
    void activate(const QModelIndex &proxyIndex);
    void updateGridSize(int thumbnailPixels);
    void applyScanResult(quint64 requestId,
                         const DirectoryScanResult &result);
    void saveDirectoryState();
    void rebuildLocationsMenu();
    void updateNavigationControls();
    void navigateToStoredDirectory(const QString &directoryPath);

protected:
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched,
                     QEvent *event) override;

    struct DirectoryState {
        QString selectedPath;
        QString filter;
        int verticalScroll = 0;
    };

    ThumbnailModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxy = nullptr;
    QListView *m_view = nullptr;
    QLineEdit *m_filter = nullptr;
    QSlider *m_sizeSlider = nullptr;
    QToolButton *m_historyBackButton = nullptr;
    QToolButton *m_historyForwardButton = nullptr;
    QToolButton *m_upButton = nullptr;
    QToolButton *m_locationsButton = nullptr;
    QMenu *m_locationsMenu = nullptr;
    QToolButton *m_openSelectedButton = nullptr;
    QToolButton *m_compareSelectedButton = nullptr;
    FolderNavigationController *m_navigation = nullptr;
    DirectoryScanService *m_scanService = nullptr;
    quint64 m_scanRequestId = 0;
    QString m_requestedDirectory;
    QString m_preferredPath;
    QHash<QString, DirectoryState> m_directoryStates;
    bool m_refreshingAppearance = false;
};
