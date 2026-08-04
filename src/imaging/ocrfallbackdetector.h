// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ocrresult.h"

#include <QImage>
#include <QRect>
#include <QSize>
#include <QVector>

struct OcrFallbackPlan
{
    QVector<QRect> regions;
    QSize analysisSize;
    qsizetype inspectedPixels = 0;
    qsizetype selectedPixels = 0;
};

class OcrFallbackDetector final
{
public:
    static constexpr int maximumAnalysisDimension = 720;
    static constexpr int maximumAnalysisPasses = 2;
    static constexpr int maximumRegionCount = 2;
    static constexpr qreal maximumRegionAreaRatio = 0.32;

    [[nodiscard]] static OcrFallbackPlan plan(
        const QImage &image,
        const QSize &logicalImageSize,
        const QVector<OcrSymbol> &recognizedSymbols);
};
