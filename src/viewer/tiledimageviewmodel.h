#pragma once

#include "imagesource.h"

#include <QCache>
#include <QImage>
#include <QList>
#include <QObject>
#include <QRect>
#include <QSet>
#include <QThreadPool>

#include <stop_token>

class TiledImageViewModel final : public QObject
{
    Q_OBJECT

public:
    struct Tile {
        QRect sourceRect;
        QImage image;
    };

    explicit TiledImageViewModel(QObject *parent = nullptr);
    ~TiledImageViewModel() override;

    void setSource(const ImageSourcePtr &source);
    void updateViewport(const QRectF &visibleSourceRect,
                        qreal zoom);
    [[nodiscard]] QList<Tile> visibleTiles() const;
    [[nodiscard]] int cacheLimitMiB() const;
    void setCacheLimitMiB(int mebibytes);

signals:
    void tilesChanged();

private:
    struct CachedTile {
        QRect sourceRect;
        QImage image;
    };

    struct TileRequest {
        QString key;
        QRect sourceRect;
        QSize outputSize;
        qreal distanceSquared = 0.0;
    };

    [[nodiscard]] static QString tileKey(int column, int row,
                                         int level);
    void startNextTile();
    void requestTile(const TileRequest &request,
                     quint64 generation);

    ImageSourcePtr m_source;
    QCache<QString, CachedTile> m_cache{64 * 1024};
    QSet<QString> m_pending;
    QSet<QString> m_needed;
    QList<TileRequest> m_requestQueue;
    QThreadPool m_pool;
    std::stop_source m_stopSource;
    quint64 m_generation = 0;
    bool m_workerBusy = false;
};
