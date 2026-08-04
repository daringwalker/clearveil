#pragma once

#include <QObject>
#include <QStringList>

class FolderNavigationController final : public QObject
{
    Q_OBJECT

public:
    static constexpr int maximumRecentDirectories = 12;
    static constexpr int maximumFavoriteDirectories = 32;
    static constexpr int maximumHistoryEntries = 50;

    explicit FolderNavigationController(QObject *parent = nullptr);

    void recordVisit(const QString &directoryPath);
    void goBack();
    void goForward();
    void navigationFailed(const QString &directoryPath);
    void toggleFavorite(const QString &directoryPath);
    void clearRecentDirectories();
    void setStoredLocations(
        const QStringList &recentDirectories,
        const QStringList &favoriteDirectories);

    [[nodiscard]] QString currentDirectory() const;
    [[nodiscard]] QStringList recentDirectories() const;
    [[nodiscard]] QStringList favoriteDirectories() const;
    [[nodiscard]] bool canGoBack() const;
    [[nodiscard]] bool canGoForward() const;
    [[nodiscard]] bool isFavorite(
        const QString &directoryPath) const;

signals:
    void directoryRequested(const QString &directoryPath);
    void stateChanged();

private:
    enum class PendingNavigation {
        None,
        Back,
        Forward
    };

    [[nodiscard]] static QString normalizedDirectory(
        const QString &directoryPath);
    [[nodiscard]] static QStringList normalizedLocations(
        const QStringList &locations, int maximumCount);
    void touchRecent(const QString &directoryPath);
    static void appendBounded(
        QStringList &entries, const QString &directoryPath,
        int maximumCount);

    QString m_currentDirectory;
    QStringList m_backHistory;
    QStringList m_forwardHistory;
    QStringList m_recentDirectories;
    QStringList m_favoriteDirectories;
    QString m_pendingDirectory;
    PendingNavigation m_pendingNavigation = PendingNavigation::None;
};
