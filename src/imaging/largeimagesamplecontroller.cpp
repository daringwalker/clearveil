#include "largeimagesamplecontroller.h"

#include <QtConcurrentRun>

#include <algorithm>

namespace {
constexpr int kSampleRadius = 5;
constexpr int kSampleExtent = kSampleRadius * 2 + 1;
}

LargeImageSampleController::LargeImageSampleController(QObject *parent)
    : QObject(parent)
{
    m_pool.setMaxThreadCount(1);
    m_pool.setExpiryTimeout(10'000);
    connect(&m_watcher, &QFutureWatcher<Result>::finished,
            this, [this] {
        const Result result = m_watcher.result();
        m_busy = false;
        if (result.generation == m_generation
            && result.color.isValid()
            && !result.sample.isNull()) {
            emit sampleReady(
                result.color, result.request.position,
                result.sample, result.request.picked,
                result.request.adjusted);
        }
        startLatestRequest();
    });
}

LargeImageSampleController::~LargeImageSampleController()
{
    m_stopSource.request_stop();
    m_pool.clear();
    m_pool.waitForDone();
}

void LargeImageSampleController::setSource(
    const ImageSourcePtr &source)
{
    if (m_source == source)
        return;
    m_stopSource.request_stop();
    m_stopSource = std::stop_source();
    ++m_generation;
    m_latestRequest.reset();
    m_source = source;
}

void LargeImageSampleController::requestSample(
    const QPoint &position, bool picked, bool adjusted)
{
    if (!m_source || !m_source->isRegionBacked()
        || !QRect(QPoint(), m_source->logicalSize()).contains(position)) {
        return;
    }
    m_latestRequest = Request{position, picked, adjusted};
    startLatestRequest();
}

void LargeImageSampleController::startLatestRequest()
{
    if (m_busy || !m_latestRequest || !m_source)
        return;
    const Request request = *m_latestRequest;
    m_latestRequest.reset();
    const ImageSourcePtr source = m_source;
    const QSize logicalSize = source->logicalSize();
    const quint64 generation = m_generation;
    const std::stop_token stopToken = m_stopSource.get_token();
    m_busy = true;
    m_watcher.setFuture(QtConcurrent::run(
        &m_pool,
        [source, logicalSize, request, generation, stopToken] {
            Result result;
            result.request = request;
            result.generation = generation;
            if (stopToken.stop_requested())
                return result;

            const QRect wanted(
                request.position.x() - kSampleRadius,
                request.position.y() - kSampleRadius,
                kSampleExtent, kSampleExtent);
            const QRect valid = wanted.intersected(
                QRect(QPoint(), logicalSize));
            if (!valid.isValid())
                return result;
            const QImage region = source->readRegion(
                valid, valid.size(), stopToken);
            if (region.isNull() || stopToken.stop_requested())
                return result;

            result.sample = QImage(
                kSampleExtent, kSampleExtent,
                QImage::Format_RGBA8888);
            result.sample.fill(Qt::transparent);
            const QPoint offset(
                valid.x() - wanted.x(),
                valid.y() - wanted.y());
            for (int y = 0; y < region.height(); ++y) {
                for (int x = 0; x < region.width(); ++x) {
                    result.sample.setPixelColor(
                        offset.x() + x, offset.y() + y,
                        region.pixelColor(x, y));
                }
            }
            result.color = result.sample.pixelColor(
                kSampleRadius, kSampleRadius);
            return result;
        }));
}
