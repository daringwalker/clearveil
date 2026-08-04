// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ocrresult.h"

#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QThreadPool>

#include <optional>

class OcrController final : public QObject
{
    Q_OBJECT

public:
    explicit OcrController(QObject *parent = nullptr);

    quint64 recognize(
        const QImage &image, const QSize &logicalImageSize,
        const QString &languages);
    void cancel();
    [[nodiscard]] bool isBusy() const;

signals:
    void recognitionStarted(quint64 requestId);
    void recognitionFinished(quint64 requestId,
                             const OcrResult &result);
    void busyChanged(bool busy);

private:
    struct Request {
        quint64 id = 0;
        QImage image;
        QSize logicalImageSize;
        QString languages;
    };

    void start(const Request &request);
    void complete();

    QThreadPool m_threadPool;
    QFutureWatcher<OcrResult> m_watcher;
    std::optional<Request> m_pending;
    quint64 m_nextRequestId = 0;
    quint64 m_latestRequestId = 0;
    quint64 m_activeRequestId = 0;
};
