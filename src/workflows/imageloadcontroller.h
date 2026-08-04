#pragma once

#include "imagedocument.h"

#include <QCache>
#include <QObject>
#include <QStringList>
#include <QThreadPool>

#include <functional>
#include <stop_token>

class ImageLoadController final : public QObject
{
    Q_OBJECT

public:
    using Decoder = std::function<ImageLoadResult(
        const QString &, std::stop_token)>;
    using LegacyDecoder = std::function<ImageLoadResult(
        const QString &)>;

    explicit ImageLoadController(
        QObject *parent = nullptr, Decoder decoder = {});
    ImageLoadController(QObject *parent, LegacyDecoder decoder);
    ~ImageLoadController() override;

    void request(const QString &path, int contextIndex);
    void remember(const ImageLoadResult &result);
    void cancel();
    void prefetch(const QStringList &paths);

    [[nodiscard]] bool isLoading() const;
    [[nodiscard]] bool isCached(const QString &path) const;
    [[nodiscard]] int cacheLimitMiB() const;
    void setCacheLimitMiB(int mebibytes);
    void clearCache();

signals:
    void loadStarted(const QString &path, int contextIndex);
    void loadFinished(const ImageLoadResult &result,
                      int contextIndex, bool fromCache);
    void loadingChanged(bool loading);

private:
    [[nodiscard]] static QString cacheKey(const QString &path);
    void store(const ImageLoadResult &result);
    void setLoading(bool loading);

    Decoder m_decoder;
    // Region-backed entries retain only their bounded preview and metadata
    // while inactive; their random-access backing is released by the document.
    QCache<QString, ImageLoadResult> m_cache{256 * 1024};
    QThreadPool m_loadPool;
    QThreadPool m_prefetchPool;
    quint64 m_loadGeneration = 0;
    quint64 m_prefetchGeneration = 0;
    std::stop_source m_loadStopSource;
    std::stop_source m_prefetchStopSource;
    bool m_loading = false;
};
