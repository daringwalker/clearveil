// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QRectF>
#include <QSize>
#include <QString>
#include <QVector>

struct OcrSymbol
{
    QString text;
    QRectF bounds;
    int lineIndex = 0;
    int wordIndex = 0;
    float confidence = 0.0F;
    bool supplemental = false;
};

struct OcrResult
{
    QSize imageSize;
    QVector<OcrSymbol> symbols;
    QString languages;
    QString error;
    int fallbackRegionCount = 0;
    qsizetype fallbackPixelCount = 0;
    QSize fallbackAnalysisSize;
    int refinementPassCount = 0;
    int refinedLineCount = 0;
    qsizetype refinementPixelCount = 0;

    [[nodiscard]] bool succeeded() const
    {
        return error.isEmpty();
    }
};
