// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrcontroller.h"

#include "ocrengine.h"

#include <QtConcurrent>

#include <utility>

OcrController::OcrController(QObject *parent)
    : QObject(parent)
{
    m_threadPool.setMaxThreadCount(1);
    m_threadPool.setExpiryTimeout(10'000);
    connect(&m_watcher, &QFutureWatcher<OcrResult>::finished,
            this, &OcrController::complete);
}

quint64 OcrController::recognize(
    const QImage &image, const QSize &logicalImageSize,
    const QString &languages)
{
    Request request{
        ++m_nextRequestId, image, logicalImageSize, languages
    };
    m_latestRequestId = request.id;
    if (m_watcher.isRunning()) {
        m_pending = std::move(request);
    } else {
        start(request);
    }
    return m_latestRequestId;
}

void OcrController::cancel()
{
    ++m_nextRequestId;
    m_latestRequestId = m_nextRequestId;
    m_pending.reset();
}

bool OcrController::isBusy() const
{
    return m_watcher.isRunning() || m_pending.has_value();
}

void OcrController::start(const Request &request)
{
    m_activeRequestId = request.id;
    emit recognitionStarted(request.id);
    emit busyChanged(true);
    m_watcher.setFuture(QtConcurrent::run(
        &m_threadPool,
        [request] {
            return OcrEngine::recognize(
                request.image, request.logicalImageSize,
                request.languages);
        }));
}

void OcrController::complete()
{
    const OcrResult result = m_watcher.result();
    const quint64 completedId = m_activeRequestId;
    if (completedId == m_latestRequestId)
        emit recognitionFinished(completedId, result);

    if (m_pending) {
        const Request request = std::move(*m_pending);
        m_pending.reset();
        start(request);
        return;
    }
    m_activeRequestId = 0;
    emit busyChanged(false);
}
