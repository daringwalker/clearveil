// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrengine.h"
#include "ocrfallbackdetector.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <algorithm>
#include <utility>

namespace {
qint64 median(QList<qint64> values)
{
    if (values.isEmpty())
        return -1;
    std::sort(values.begin(), values.end());
    return values.at(values.size() / 2);
}

QJsonArray jsonTimes(const QList<qint64> &values)
{
    QJsonArray result;
    for (const qint64 value : values)
        result.append(value);
    return result;
}
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName(
        QStringLiteral("clearveil-ocr-benchmark"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Compare Clearveil primary and selective-fallback OCR"));
    parser.addHelpOption();
    const QCommandLineOption imageOption(
        QStringLiteral("image"), QStringLiteral("Input image path"),
        QStringLiteral("path"));
    const QCommandLineOption runsOption(
        QStringLiteral("runs"), QStringLiteral("Measured runs per mode"),
        QStringLiteral("count"), QStringLiteral("3"));
    const QCommandLineOption jsonOption(
        QStringLiteral("json"), QStringLiteral("Print machine-readable JSON"));
    parser.addOptions({imageOption, runsOption, jsonOption});
    parser.process(application);

    bool runsValid = false;
    const int runs = parser.value(runsOption).toInt(&runsValid);
    const QString imagePath = parser.value(imageOption);
    if (!runsValid || runs < 1 || imagePath.isEmpty()) {
        QTextStream(stderr) << "Invalid benchmark arguments.\n";
        return 64;
    }
    const QImage image(imagePath);
    if (image.isNull()) {
        QTextStream(stderr) << "Could not load image: "
                            << imagePath << '\n';
        return 1;
    }
    const QString languages = OcrEngine::recognitionLanguages();
    if (languages.isEmpty()) {
        QTextStream(stderr) << OcrEngine::unavailableMessage() << '\n';
        return 1;
    }

    QList<qint64> primaryTimes;
    QList<qint64> enhancedTimes;
    OcrResult primaryResult;
    OcrResult enhancedResult;
    const OcrRecognitionOptions primaryOnly{false, false};
    for (int run = 0; run < runs; ++run) {
        const bool enhancedFirst = (run & 1) != 0;
        for (int pass = 0; pass < 2; ++pass) {
            const bool enhanced = (pass == 0) == enhancedFirst;
            QElapsedTimer timer;
            timer.start();
            OcrResult result = OcrEngine::recognize(
                image, image.size(), languages,
                enhanced ? OcrRecognitionOptions{} : primaryOnly);
            const qint64 elapsed = timer.elapsed();
            if (!result.succeeded()) {
                QTextStream(stderr) << result.error << '\n';
                return 1;
            }
            if (enhanced) {
                enhancedTimes.append(elapsed);
                enhancedResult = std::move(result);
            } else {
                primaryTimes.append(elapsed);
                primaryResult = std::move(result);
            }
        }
    }

    const qint64 primaryMedian = median(primaryTimes);
    const qint64 enhancedMedian = median(enhancedTimes);
    const qreal overheadPercent = primaryMedian > 0
        ? (enhancedMedian - primaryMedian) * 100.0 / primaryMedian
        : 0.0;
    const OcrFallbackPlan fallbackPlan = OcrFallbackDetector::plan(
        image, image.size(), primaryResult.symbols);
    QJsonArray fallbackRegions;
    for (const QRect &region : fallbackPlan.regions) {
        fallbackRegions.append(QJsonObject{
            {QStringLiteral("x"), region.x()},
            {QStringLiteral("y"), region.y()},
            {QStringLiteral("width"), region.width()},
            {QStringLiteral("height"), region.height()}
        });
    }
    const QJsonObject report{
        {QStringLiteral("image"), QFileInfo(imagePath).absoluteFilePath()},
        {QStringLiteral("width"), image.width()},
        {QStringLiteral("height"), image.height()},
        {QStringLiteral("languages"), languages},
        {QStringLiteral("runs"), runs},
        {QStringLiteral("primary_ms"), jsonTimes(primaryTimes)},
        {QStringLiteral("enhanced_ms"), jsonTimes(enhancedTimes)},
        {QStringLiteral("primary_median_ms"), primaryMedian},
        {QStringLiteral("enhanced_median_ms"), enhancedMedian},
        {QStringLiteral("overhead_percent"), overheadPercent},
        {QStringLiteral("primary_symbols"), primaryResult.symbols.size()},
        {QStringLiteral("enhanced_symbols"), enhancedResult.symbols.size()},
        {QStringLiteral("fallback_regions"),
         enhancedResult.fallbackRegionCount},
        {QStringLiteral("fallback_pixels"),
         static_cast<qint64>(enhancedResult.fallbackPixelCount)},
        {QStringLiteral("refinement_passes"),
         enhancedResult.refinementPassCount},
        {QStringLiteral("refined_lines"),
         enhancedResult.refinedLineCount},
        {QStringLiteral("refinement_pixels"),
         static_cast<qint64>(enhancedResult.refinementPixelCount)},
        {QStringLiteral("fallback_region_rects"), fallbackRegions}
    };

    QTextStream output(stdout);
    if (parser.isSet(jsonOption)) {
        output << QJsonDocument(report).toJson(QJsonDocument::Indented);
    } else {
        output << "Clearveil OCR benchmark\n"
               << "  image: " << image.width() << 'x' << image.height()
               << "  languages: " << languages << '\n'
               << "  primary median: " << primaryMedian << " ms\n"
               << "  enhanced median: " << enhancedMedian << " ms ("
               << overheadPercent << "% overhead)\n"
               << "  symbols: " << primaryResult.symbols.size() << " -> "
               << enhancedResult.symbols.size() << '\n'
               << "  fallback: " << enhancedResult.fallbackRegionCount
               << " region(s), " << enhancedResult.fallbackPixelCount
               << " pixels\n"
               << "  small-text refinement: "
               << enhancedResult.refinementPassCount << " pass(es), "
               << enhancedResult.refinedLineCount << " corrected line(s), "
               << enhancedResult.refinementPixelCount << " pixels\n";
    }
    output.flush();
    return 0;
}
