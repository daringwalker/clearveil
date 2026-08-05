#pragma once

#include "imagesource.h"

#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QPoint>
#include <QThreadPool>

#include <optional>
#include <stop_token>

// Coalesces high-frequency pointer movement into at most one active region
// read and one latest pending read. This keeps exact large-image sampling off
// the UI thread without building an unbounded queue behind the cursor.
class LargeImageSampleController final : public QObject
{
    Q_OBJECT

public:
    explicit LargeImageSampleController(QObject *parent = nullptr);
    ~LargeImageSampleController() override;

    void setSource(const ImageSourcePtr &source);
    void requestSample(const QPoint &position,
                       bool picked, bool adjusted);

signals:
    void sampleReady(const QColor &color, const QPoint &position,
                     const QImage &sample, bool picked,
                     bool adjusted);

private:
    struct Request {
        QPoint position;
        bool picked = false;
        bool adjusted = false;
        quint64 serial = 0;
    };
    struct Result {
        Request request;
        QColor color;
        QImage sample;
        quint64 generation = 0;
    };

    void startLatestRequest();

    ImageSourcePtr m_source;
    QThreadPool m_pool;
    QFutureWatcher<Result> m_watcher;
    std::optional<Request> m_latestRequest;
    std::stop_source m_stopSource;
    quint64 m_generation = 0;
    quint64 m_requestSerial = 0;
    bool m_busy = false;
};
