// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrfallbackdetector.h"

#include <QRectF>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace {
struct RowActivity
{
    int edgeCount = 0;
    int firstEdge = 0;
    int lastEdge = -1;
    bool active = false;
};

struct ScoredRegion
{
    QRect rect;
    qreal score = 0.0;
};

QRect scaledRect(const QRectF &bounds, const QSize &from,
                 const QSize &to)
{
    if (!from.isValid() || !to.isValid())
        return {};
    const qreal scaleX = static_cast<qreal>(to.width()) / from.width();
    const qreal scaleY = static_cast<qreal>(to.height()) / from.height();
    return QRectF(bounds.left() * scaleX,
                  bounds.top() * scaleY,
                  bounds.width() * scaleX,
                  bounds.height() * scaleY)
        .toAlignedRect();
}

QRect analysisToInput(const QRect &rect, const QSize &analysisSize,
                      const QSize &inputSize)
{
    const qreal scaleX = static_cast<qreal>(inputSize.width())
        / analysisSize.width();
    const qreal scaleY = static_cast<qreal>(inputSize.height())
        / analysisSize.height();
    return QRectF(rect.left() * scaleX,
                  rect.top() * scaleY,
                  rect.width() * scaleX,
                  rect.height() * scaleY)
        .toAlignedRect()
        .intersected(QRect(QPoint(0, 0), inputSize));
}

qreal intersectionRatio(const QRect &first, const QRect &second)
{
    const QRect overlap = first.intersected(second);
    if (overlap.isEmpty())
        return 0.0;
    const qreal smallerArea = std::min<qreal>(
        static_cast<qreal>(first.width()) * first.height(),
        static_cast<qreal>(second.width()) * second.height());
    return smallerArea > 0.0
        ? static_cast<qreal>(overlap.width()) * overlap.height()
            / smallerArea
        : 0.0;
}
}

OcrFallbackPlan OcrFallbackDetector::plan(
    const QImage &image, const QSize &logicalImageSize,
    const QVector<OcrSymbol> &recognizedSymbols)
{
    OcrFallbackPlan result;
    if (image.isNull() || image.width() < 160 || image.height() < 80)
        return result;

    QImage analysis = image;
    if (std::max(image.width(), image.height())
        > maximumAnalysisDimension) {
        analysis = image.scaled(
            QSize(maximumAnalysisDimension, maximumAnalysisDimension),
            Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    analysis = analysis.convertToFormat(QImage::Format_Grayscale8);
    result.analysisSize = analysis.size();
    result.inspectedPixels = static_cast<qsizetype>(analysis.width())
        * analysis.height();

    QImage coverage(analysis.size(), QImage::Format_Grayscale8);
    coverage.fill(0);
    const QSize logicalSize = logicalImageSize.isValid()
        ? logicalImageSize : image.size();
    const QRect analysisBounds(QPoint(0, 0), analysis.size());
    for (const OcrSymbol &symbol : recognizedSymbols) {
        const QRect covered = scaledRect(
            symbol.bounds, logicalSize, analysis.size())
                                  .adjusted(-2, -2, 2, 2)
                                  .intersected(analysisBounds);
        if (covered.isEmpty())
            continue;
        for (int y = covered.top(); y <= covered.bottom(); ++y) {
            std::memset(coverage.scanLine(y) + covered.left(), 255,
                        static_cast<size_t>(covered.width()));
        }
    }

    const int width = analysis.width();
    const int height = analysis.height();
    const int minimumEdges = std::max(10, width / 45);
    const int maximumEdges = std::max(minimumEdges + 1,
                                      width * 3 / 5);
    QVector<RowActivity> rows(height);
    for (int y = 1; y < height - 1; ++y) {
        const uchar *above = analysis.constScanLine(y - 1);
        const uchar *current = analysis.constScanLine(y);
        const uchar *below = analysis.constScanLine(y + 1);
        const uchar *covered = coverage.constScanLine(y);
        RowActivity activity;
        activity.firstEdge = width;
        for (int x = 1; x < width - 1; ++x) {
            if (covered[x] != 0)
                continue;
            const int horizontal = std::abs(
                static_cast<int>(current[x + 1]) - current[x - 1]);
            const int vertical = std::abs(
                static_cast<int>(below[x]) - above[x]);
            if (std::max(horizontal, vertical) < 18)
                continue;
            ++activity.edgeCount;
            activity.firstEdge = std::min(activity.firstEdge, x);
            activity.lastEdge = x;
        }
        activity.active = activity.edgeCount >= minimumEdges
            && activity.edgeCount <= maximumEdges;
        rows[y] = activity;
    }

    QVector<ScoredRegion> candidates;
    for (int y = 1; y < height - 1;) {
        if (!rows.at(y).active) {
            ++y;
            continue;
        }
        const int firstRow = y;
        int lastActiveRow = y;
        int inactiveGap = 0;
        int activeRows = 0;
        int firstEdge = width;
        int lastEdge = -1;
        qsizetype totalEdges = 0;
        while (y < height - 1 && inactiveGap <= 2) {
            if (rows.at(y).active) {
                lastActiveRow = y;
                inactiveGap = 0;
                ++activeRows;
                totalEdges += rows.at(y).edgeCount;
                firstEdge = std::min(firstEdge,
                                     rows.at(y).firstEdge);
                lastEdge = std::max(lastEdge,
                                    rows.at(y).lastEdge);
            } else {
                ++inactiveGap;
            }
            ++y;
        }

        const int groupHeight = lastActiveRow - firstRow + 1;
        QVector<int> columnEdges(width);
        qsizetype groupedEdges = 0;
        for (int row = firstRow; row <= lastActiveRow; ++row) {
            const uchar *above = analysis.constScanLine(row - 1);
            const uchar *current = analysis.constScanLine(row);
            const uchar *below = analysis.constScanLine(row + 1);
            const uchar *covered = coverage.constScanLine(row);
            for (int x = 1; x < width - 1; ++x) {
                if (covered[x] != 0)
                    continue;
                const int horizontal = std::abs(
                    static_cast<int>(current[x + 1]) - current[x - 1]);
                const int vertical = std::abs(
                    static_cast<int>(below[x]) - above[x]);
                if (std::max(horizontal, vertical) < 18)
                    continue;
                ++columnEdges[x];
                ++groupedEdges;
            }
        }
        result.inspectedPixels += static_cast<qsizetype>(groupHeight)
            * width;
        if (groupedEdges > 0) {
            const qsizetype trimEdges = std::max<qsizetype>(
                1, groupedEdges / 100);
            qsizetype accumulated = 0;
            for (int x = 1; x < width - 1; ++x) {
                accumulated += columnEdges.at(x);
                if (accumulated >= trimEdges) {
                    firstEdge = x;
                    break;
                }
            }
            accumulated = 0;
            for (int x = width - 2; x >= 1; --x) {
                accumulated += columnEdges.at(x);
                if (accumulated >= trimEdges) {
                    lastEdge = x;
                    break;
                }
            }
        }
        const int edgeSpan = lastEdge - firstEdge + 1;
        const int maximumTextHeight = std::max(18, height / 11);
        if (activeRows < 3 || groupHeight > maximumTextHeight
            || edgeSpan < width / 10) {
            continue;
        }

        const int horizontalMargin = std::max(10, width / 45);
        const int verticalMargin = std::max(12, groupHeight * 2);
        QRect analysisRect(
            firstEdge - horizontalMargin,
            firstRow - verticalMargin,
            edgeSpan + horizontalMargin * 2,
            groupHeight + verticalMargin * 2);
        analysisRect = analysisRect.intersected(analysisBounds);
        const QRect inputRect = analysisToInput(
            analysisRect, analysis.size(), image.size());
        if (inputRect.width() < 80 || inputRect.height() < 36)
            continue;

        const qreal density = static_cast<qreal>(totalEdges)
            / std::max(1, activeRows * edgeSpan);
        const QRectF logicalRegion(
            inputRect.x() * static_cast<qreal>(logicalSize.width())
                / image.width(),
            inputRect.y() * static_cast<qreal>(logicalSize.height())
                / image.height(),
            inputRect.width() * static_cast<qreal>(logicalSize.width())
                / image.width(),
            inputRect.height() * static_cast<qreal>(logicalSize.height())
                / image.height());
        int recognizedInRegion = 0;
        for (const OcrSymbol &symbol : recognizedSymbols) {
            if (logicalRegion.intersects(symbol.bounds))
                ++recognizedInRegion;
        }
        // A row already containing many OCR boxes is less likely to be a
        // genuine omission. Penalizing it keeps the small fallback budget for
        // uncovered text instead of re-reading icon labels and dense rows.
        const qreal coveragePenalty = 1.0
            + recognizedInRegion * 0.45;
        candidates.append({
            inputRect,
            density * edgeSpan * std::sqrt(activeRows)
                / coveragePenalty
        });
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const ScoredRegion &first,
                 const ScoredRegion &second) {
        return first.score > second.score;
    });

    const qsizetype imagePixels = static_cast<qsizetype>(image.width())
        * image.height();
    const qsizetype pixelBudget = static_cast<qsizetype>(
        imagePixels * maximumRegionAreaRatio);
    for (const ScoredRegion &candidate : candidates) {
        if (result.regions.size() >= maximumRegionCount)
            break;
        bool duplicate = false;
        for (const QRect &selected : std::as_const(result.regions)) {
            if (intersectionRatio(candidate.rect, selected) >= 0.5) {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;
        const qsizetype pixels = static_cast<qsizetype>(
            candidate.rect.width()) * candidate.rect.height();
        if (result.selectedPixels + pixels > pixelBudget)
            continue;
        result.regions.append(candidate.rect);
        result.selectedPixels += pixels;
    }
    return result;
}
