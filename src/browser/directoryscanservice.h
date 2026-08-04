#pragma once

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QThreadPool>

#include <atomic>
#include <memory>

struct DirectoryScanEntry
{
    QString path;
    QString name;
    bool directory = false;
    qint64 size = 0;
    QDateTime modified;
    QString suffix;
};

struct DirectoryScanResult
{
    QString directoryPath;
    QList<DirectoryScanEntry> entries;
    QDateTime directoryModified;
    QString error;
    bool cancelled = false;
    qint64 elapsedMs = 0;

    [[nodiscard]] bool succeeded() const;
    [[nodiscard]] QStringList imageFiles() const;
    [[nodiscard]] int imageCount() const;
};

Q_DECLARE_METATYPE(DirectoryScanResult)

class DirectoryScanService final : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryScanService(QObject *parent = nullptr);
    ~DirectoryScanService() override;

    quint64 requestScan(const QString &directoryPath,
                        bool forceRefresh = false);
    void cancel(quint64 requestId);
    void invalidate(const QString &directoryPath);
    [[nodiscard]] bool cachedResult(
        const QString &directoryPath,
        DirectoryScanResult *result = nullptr) const;

signals:
    void scanFinished(quint64 requestId,
                      const DirectoryScanResult &result);

private:
    struct RequestState {
        std::shared_ptr<std::atomic_bool> cancelled;
    };

    void storeCache(const DirectoryScanResult &result);
    [[nodiscard]] bool cacheIsFresh(
        const DirectoryScanResult &result) const;

    QThreadPool m_pool;
    QSet<QString> m_supportedSuffixes;
    QHash<quint64, RequestState> m_requests;
    QHash<QString, DirectoryScanResult> m_cache;
    QStringList m_cacheOrder;
    quint64 m_nextRequestId = 1;
};
