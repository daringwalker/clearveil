// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrengine.h"

#include "ocrfallbackdetector.h"

#include <QCoreApplication>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#ifdef CLEARVEIL_HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <tesseract/capi.h>
#include <tesseract/resultiterator.h>
#endif

namespace {
QString translated(const char *text)
{
    return QCoreApplication::translate("OcrEngine", text);
}

QStringList usableLanguages(const QStringList &languages)
{
    QStringList result;
    for (const QString &language : languages) {
        if (!language.trimmed().isEmpty()
            && language != QStringLiteral("osd")) {
            result.append(language.trimmed());
        }
    }
    result.removeDuplicates();
    return result;
}

#ifdef CLEARVEIL_HAVE_TESSERACT
struct RecognitionPass
{
    QVector<OcrSymbol> symbols;
    bool succeeded = true;
};

RecognitionPass recognizePass(
    tesseract::TessBaseAPI &api, const QImage &image,
    const QSize &logicalImageSize, const QSize &fullInputSize,
    const QPoint &inputOffset, tesseract::PageSegMode pageSegMode,
    bool supplemental)
{
    RecognitionPass pass;
    api.Clear();
    api.SetPageSegMode(pageSegMode);
    const int bytesPerPixel = image.depth() / 8;
    api.SetImage(image.constBits(), image.width(), image.height(),
                 bytesPerPixel, image.bytesPerLine());
    api.SetSourceResolution(100);
    if (api.Recognize(nullptr) != 0) {
        pass.succeeded = false;
        return pass;
    }

    std::unique_ptr<tesseract::ResultIterator> iterator(api.GetIterator());
    if (!iterator)
        return pass;

    const qreal scaleX = static_cast<qreal>(logicalImageSize.width())
        / fullInputSize.width();
    const qreal scaleY = static_cast<qreal>(logicalImageSize.height())
        / fullInputSize.height();
    int lineIndex = -1;
    int wordIndex = -1;
    do {
        if (iterator->IsAtBeginningOf(tesseract::RIL_TEXTLINE))
            ++lineIndex;
        if (iterator->IsAtBeginningOf(tesseract::RIL_WORD))
            ++wordIndex;

        std::unique_ptr<char[]> rawText(
            iterator->GetUTF8Text(tesseract::RIL_SYMBOL));
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        if (!rawText
            || !iterator->BoundingBox(
                tesseract::RIL_SYMBOL,
                &left, &top, &right, &bottom)) {
            continue;
        }
        const QString text = QString::fromUtf8(rawText.get());
        if (text.trimmed().isEmpty()
            || right <= left || bottom <= top) {
            continue;
        }

        int lineLeft = 0;
        int lineTop = top;
        int lineRight = 0;
        int lineBottom = bottom;
        if (supplemental) {
            lineTop = top;
            lineBottom = bottom;
        } else if (!iterator->BoundingBox(
                tesseract::RIL_TEXTLINE, &lineLeft, &lineTop,
                &lineRight, &lineBottom)
            || lineBottom <= lineTop) {
            lineTop = top;
            lineBottom = bottom;
        }

        pass.symbols.append({
            text,
            QRectF((inputOffset.x() + left) * scaleX,
                   (inputOffset.y() + lineTop) * scaleY,
                   (right - left) * scaleX,
                   (lineBottom - lineTop) * scaleY),
            std::max(0, lineIndex),
            std::max(0, wordIndex),
            iterator->Confidence(tesseract::RIL_SYMBOL),
            supplemental
        });
    } while (iterator->Next(tesseract::RIL_SYMBOL));
    return pass;
}

qreal overlapRatio(const QRectF &first, const QRectF &second)
{
    const QRectF overlap = first.intersected(second);
    if (overlap.isEmpty())
        return 0.0;
    const qreal smallerArea = std::min(first.width() * first.height(),
                                       second.width() * second.height());
    return smallerArea > 0.0
        ? overlap.width() * overlap.height() / smallerArea
        : 0.0;
}

bool overlapsExistingSymbol(
    const OcrSymbol &candidate,
    const QVector<OcrSymbol> &recognizedSymbols)
{
    return std::any_of(
        recognizedSymbols.cbegin(), recognizedSymbols.cend(),
        [&candidate](const OcrSymbol &recognized) {
            return overlapRatio(candidate.bounds, recognized.bounds)
                >= 0.4;
        });
}

int mergeSupplementalSymbols(
    QVector<OcrSymbol> &recognizedSymbols,
    QVector<OcrSymbol> supplementalSymbols)
{
    if (supplementalSymbols.isEmpty())
        return 0;

    int addedSymbols = 0;

    int nextLineIndex = 0;
    int nextWordIndex = 0;
    for (const OcrSymbol &symbol : std::as_const(recognizedSymbols)) {
        nextLineIndex = std::max(nextLineIndex, symbol.lineIndex + 1);
        nextWordIndex = std::max(nextWordIndex, symbol.wordIndex + 1);
    }

    for (int first = 0; first < supplementalSymbols.size();) {
        int last = first;
        while (last + 1 < supplementalSymbols.size()
               && supplementalSymbols.at(last + 1).lineIndex
                   == supplementalSymbols.at(first).lineIndex) {
            ++last;
        }

        QVector<OcrSymbol> newLine;
        qreal confidenceTotal = 0.0;
        int meaningfulSymbols = 0;
        for (int index = first; index <= last; ++index) {
            OcrSymbol symbol = supplementalSymbols.at(index);
            if (symbol.confidence < 20.0F
                || overlapsExistingSymbol(symbol, recognizedSymbols)) {
                continue;
            }
            bool meaningful = false;
            for (const QChar character : symbol.text) {
                if (character.isLetterOrNumber()) {
                    meaningful = true;
                    break;
                }
            }
            if (meaningful) {
                ++meaningfulSymbols;
                confidenceTotal += symbol.confidence;
            }
            newLine.append(std::move(symbol));
        }
        first = last + 1;

        if (meaningfulSymbols < 2
            || confidenceTotal / meaningfulSymbols < 35.0) {
            continue;
        }
        const auto isPunctuationOnly = [](const OcrSymbol &symbol) {
            return std::none_of(
                symbol.text.cbegin(), symbol.text.cend(),
                [](const QChar character) {
                    return character.isLetterOrNumber();
                });
        };
        while (!newLine.isEmpty()
               && isPunctuationOnly(newLine.constFirst())) {
            newLine.removeFirst();
        }
        while (!newLine.isEmpty()
               && isPunctuationOnly(newLine.constLast())) {
            newLine.removeLast();
        }
        if (newLine.isEmpty())
            continue;

        int sourceWordIndex = -1;
        int assignedWordIndex = nextWordIndex;
        QRectF lineBounds;
        for (OcrSymbol &symbol : newLine) {
            if (symbol.wordIndex != sourceWordIndex) {
                sourceWordIndex = symbol.wordIndex;
                assignedWordIndex = nextWordIndex++;
            }
            symbol.lineIndex = nextLineIndex;
            symbol.wordIndex = assignedWordIndex;
            lineBounds = lineBounds.isNull()
                ? symbol.bounds : lineBounds.united(symbol.bounds);
        }
        ++nextLineIndex;

        const qreal lineCentre = lineBounds.center().y();
        int insertionIndex = recognizedSymbols.size();
        for (int index = 0; index < recognizedSymbols.size(); ++index) {
            if (recognizedSymbols.at(index).bounds.center().y()
                > lineCentre + lineBounds.height() * 0.2) {
                insertionIndex = index;
                break;
            }
        }
        for (int index = 0; index < newLine.size(); ++index) {
            recognizedSymbols.insert(insertionIndex + index,
                                     std::move(newLine[index]));
        }
        addedSymbols += newLine.size();
    }
    return addedSymbols;
}

bool containsHanCharacter(const QString &text)
{
    return std::any_of(text.cbegin(), text.cend(), [](QChar character) {
        const uint value = character.unicode();
        return (value >= 0x3400 && value <= 0x4DBF)
            || (value >= 0x4E00 && value <= 0x9FFF)
            || (value >= 0xF900 && value <= 0xFAFF);
    });
}

bool isShortUppercaseLatin(const QString &text)
{
    if (text.size() < 2 || text.size() > 4)
        return false;
    return std::all_of(text.cbegin(), text.cend(), [](QChar character) {
        return character >= QLatin1Char('A')
            && character <= QLatin1Char('Z');
    });
}

QString joinedText(const QVector<OcrSymbol> &symbols)
{
    QString result;
    for (const OcrSymbol &symbol : symbols)
        result.append(symbol.text.trimmed());
    return result;
}

float averageConfidence(
    const QVector<OcrSymbol> &symbols)
{
    if (symbols.isEmpty())
        return 0.0F;
    float total = 0.0F;
    for (const OcrSymbol &symbol : symbols)
        total += symbol.confidence;
    return total / symbols.size();
}

void refineSmallUncertainLines(
    tesseract::TessBaseAPI &api, const QImage &input,
    const QSize &logicalImageSize, QVector<OcrSymbol> &symbols,
    int &passCount, int &refinedLineCount,
    qsizetype &processedPixels)
{
    if (symbols.size() < 2 || !logicalImageSize.isValid())
        return;

    const qreal toInputX = static_cast<qreal>(input.width())
        / logicalImageSize.width();
    const qreal toInputY = static_cast<qreal>(input.height())
        / logicalImageSize.height();
    const qsizetype pixelBudget = static_cast<qsizetype>(input.width())
        * input.height();
    const QRect inputBounds(QPoint(), input.size());

    const auto refineRange = [&](int first, int last) {
        const int symbolCount = last - first + 1;
        if (symbolCount < 2 || symbolCount > 16
            || passCount >= OcrEngine::maximumRefinementPasses) {
            return;
        }

        QVector<OcrSymbol> originalLine;
        originalLine.reserve(symbolCount);
        QRectF logicalBounds;
        for (int index = first; index <= last; ++index) {
            const OcrSymbol &symbol = symbols.at(index);
            originalLine.append(symbol);
            logicalBounds = logicalBounds.isNull()
                ? symbol.bounds : logicalBounds.united(symbol.bounds);
        }
        const qreal inputLineHeight = logicalBounds.height() * toInputY;
        const float originalConfidence = averageConfidence(originalLine);
        const QString originalText = joinedText(originalLine);
        const bool suspiciousCjk = containsHanCharacter(originalText)
            && originalConfidence < 90.0F;
        const bool suspiciousLatin = isShortUppercaseLatin(originalText);
        if (inputLineHeight < 7.0 || inputLineHeight > 32.0
            || (!suspiciousCjk && !suspiciousLatin)) {
            return;
        }

        QRect crop = QRectF(
            logicalBounds.left() * toInputX,
            logicalBounds.top() * toInputY,
            logicalBounds.width() * toInputX,
            logicalBounds.height() * toInputY).toAlignedRect();
        const int margin = std::max(8, qRound(inputLineHeight * 0.45));
        crop = crop.adjusted(-margin, -margin, margin, margin)
                   .intersected(inputBounds);
        if (crop.isEmpty())
            return;

        const int upscale = inputLineHeight <= 20.0 ? 3 : 2;
        QImage enhanced = input.copy(crop)
                              .convertToFormat(QImage::Format_Grayscale8)
                              .scaled(crop.size() * upscale,
                                      Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation);
        const qsizetype enhancedPixels = static_cast<qsizetype>(
            enhanced.width()) * enhanced.height();
        if (enhanced.isNull()
            || processedPixels + enhancedPixels > pixelBudget) {
            return;
        }
        processedPixels += enhancedPixels;
        ++passCount;

        const RecognitionPass refined = recognizePass(
            api, enhanced, crop.size(), enhanced.size(), QPoint(),
            tesseract::PSM_SINGLE_LINE, true);
        if (!refined.succeeded
            || refined.symbols.size() != symbolCount) {
            return;
        }
        const QString refinedText = joinedText(refined.symbols);
        const float refinedConfidence = averageConfidence(refined.symbols);
        if (refinedText.isEmpty() || refinedText == originalText)
            return;

        const bool scriptCorrection = !containsHanCharacter(originalText)
            && containsHanCharacter(refinedText)
            && refinedConfidence >= 95.0F
            && refinedConfidence + 2.0F >= originalConfidence;
        if (refinedConfidence < originalConfidence + 5.0F
            && !scriptCorrection) {
            return;
        }

        for (int offset = 0; offset < symbolCount; ++offset) {
            symbols[first + offset].text =
                refined.symbols.at(offset).text.trimmed();
            symbols[first + offset].confidence =
                refined.symbols.at(offset).confidence;
        }
        ++refinedLineCount;
    };

    for (int first = 0;
         first < symbols.size()
         && passCount < OcrEngine::maximumRefinementPasses;
         ) {
        int lineLast = first;
        while (lineLast + 1 < symbols.size()
               && symbols.at(lineLast + 1).lineIndex
                   == symbols.at(first).lineIndex
               && symbols.at(lineLast + 1).supplemental
                   == symbols.at(first).supplemental) {
            ++lineLast;
        }

        if (symbols.at(first).supplemental) {
            first = lineLast + 1;
            continue;
        }

        // Tesseract often considers every label in an icon-grid row one text
        // line. Split it at large visual gaps so a local retry sees one label
        // (for example 播客) instead of the entire row.
        for (int clusterFirst = first; clusterFirst <= lineLast;) {
            int clusterLast = clusterFirst;
            while (clusterLast + 1 <= lineLast) {
                const OcrSymbol &current = symbols.at(clusterLast);
                const OcrSymbol &next = symbols.at(clusterLast + 1);
                const qreal lineHeight = std::max(
                    current.bounds.height(), next.bounds.height());
                const qreal maximumGap = std::max(12.0,
                                                   lineHeight * 0.9);
                const qreal gap = next.bounds.left()
                    - current.bounds.right();
                if (gap > maximumGap)
                    break;
                ++clusterLast;
            }
            refineRange(clusterFirst, clusterLast);
            clusterFirst = clusterLast + 1;
        }
        first = lineLast + 1;
    }
}
#endif
}

bool OcrEngine::isAvailable()
{
#ifdef CLEARVEIL_HAVE_TESSERACT
    return true;
#else
    return false;
#endif
}

QStringList OcrEngine::availableLanguages()
{
#ifdef CLEARVEIL_HAVE_TESSERACT
    tesseract::TessBaseAPI api;
    if (api.Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) != 0) {
        api.End();
        if (api.Init(nullptr, "osd",
                     tesseract::OEM_LSTM_ONLY) != 0) {
            return {};
        }
    }
    char **languages = TessBaseAPIGetAvailableLanguagesAsVector(&api);
    api.End();
    QStringList result;
    if (languages) {
        for (char **language = languages; *language; ++language)
            result.append(QString::fromUtf8(*language));
        TessDeleteTextArray(languages);
    }
    result.sort(Qt::CaseInsensitive);
    return usableLanguages(result);
#else
    return {};
#endif
}

QString OcrEngine::recognitionLanguages(
    const QStringList &available)
{
    QStringList usable = usableLanguages(available);
    if (usable.isEmpty())
        return {};
    // Installed OCR data expresses the user's desired recognition coverage.
    // Keep this independent from the desktop and interface language so an
    // English UI can recognize Chinese (or any other installed language).
    usable.sort(Qt::CaseInsensitive);
    return usable.join(QLatin1Char('+'));
}

QString OcrEngine::unavailableMessage()
{
#ifdef CLEARVEIL_HAVE_TESSERACT
    if (availableLanguages().isEmpty()) {
        return translated(
            QT_TRANSLATE_NOOP("OcrEngine",
                "No OCR language data is installed. Install at least the English or Chinese Tesseract language package."));
    }
    return {};
#else
    return translated(
        QT_TRANSLATE_NOOP("OcrEngine",
            "This Clearveil build does not include Tesseract OCR support. Install the Tesseract development package and rebuild Clearveil."));
#endif
}

OcrResult OcrEngine::recognize(
    const QImage &image, const QSize &logicalImageSize,
    const QString &languages, const OcrRecognitionOptions &options)
{
    OcrResult result;
    result.imageSize = logicalImageSize.isValid()
        ? logicalImageSize : image.size();
    result.languages = languages;
    if (image.isNull()) {
        result.error = translated(QT_TRANSLATE_NOOP(
            "OcrEngine", "There is no image to recognize."));
        return result;
    }
    if (languages.trimmed().isEmpty()) {
        result.error = unavailableMessage();
        if (result.error.isEmpty()) {
            result.error = translated(
                QT_TRANSLATE_NOOP("OcrEngine",
                    "No usable OCR language data was found."));
        }
        return result;
    }

#ifdef CLEARVEIL_HAVE_TESSERACT
    QImage input = image;
    const int largestDimension = std::max(input.width(), input.height());
    if (largestDimension > maximumInputDimension) {
        input = input.scaled(
            QSize(maximumInputDimension, maximumInputDimension),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    // Preserve color until Tesseract performs its own adaptive thresholding.
    // Converting low-contrast screenshots to Qt's generic grayscale first can
    // make isolated labels disappear during page segmentation. Scale before
    // conversion so very large source images never create a full-size RGB
    // temporary buffer solely for OCR.
    input = input.convertToFormat(QImage::Format_RGB888);
    tesseract::TessBaseAPI api;
    const QByteArray languageBytes = languages.toUtf8();
    if (api.Init(nullptr, languageBytes.constData(),
                 tesseract::OEM_LSTM_ONLY) != 0) {
        result.error = translated(
            QT_TRANSLATE_NOOP("OcrEngine",
                "Tesseract could not load the selected OCR language data."));
        return result;
    }
    // QGuiApplication may normalize an image's dots-per-meter value to the
    // desktop logical DPI (for example 100 -> 96) without changing a single
    // pixel. recognizePass therefore uses one stable analysis resolution.
    const RecognitionPass primary = recognizePass(
        api, input, result.imageSize, input.size(), QPoint(),
        tesseract::PSM_AUTO, false);
    if (!primary.succeeded) {
        result.error = translated(QT_TRANSLATE_NOOP(
            "OcrEngine", "Text recognition failed."));
        return result;
    }
    result.symbols = primary.symbols;

    // Full-page thresholding can erase a low-contrast text row even though
    // the same pixels are readable in local context. The detector works on a
    // bounded, low-resolution luminance image and returns at most two regions
    // whose combined area is capped. Ordinary images therefore avoid an
    // unconditional second OCR pass.
    const OcrFallbackPlan fallbackPlan = options.recoverMissingText
        ? OcrFallbackDetector::plan(
              input, result.imageSize, result.symbols)
        : OcrFallbackPlan{};
    result.fallbackAnalysisSize = fallbackPlan.analysisSize;
    for (const QRect &region : fallbackPlan.regions) {
        ++result.fallbackRegionCount;
        result.fallbackPixelCount += static_cast<qsizetype>(
            region.width()) * region.height();
        const QImage localInput = input.copy(region);
        const RecognitionPass supplemental = recognizePass(
            api, localInput, result.imageSize, input.size(),
            region.topLeft(), tesseract::PSM_SINGLE_BLOCK, true);
        if (supplemental.succeeded) {
            const int addedSymbols = mergeSupplementalSymbols(
                result.symbols, supplemental.symbols);
            // One productive local pass is enough. Continue to the second
            // ranked candidate only when the first produced no useful line.
            if (addedSymbols >= 4)
                break;
        }
    }
    // Small UI labels can be segmented correctly while individual glyphs are
    // still confused (for example 播客 -> 播宰/BE). Recheck only short,
    // low-confidence primary lines on a tiny grayscale crop. The pass count
    // and total enhanced pixels are capped, and text is replaced only when
    // the local result is materially more confident.
    if (options.refineSmallText) {
        refineSmallUncertainLines(
            api, input, result.imageSize, result.symbols,
            result.refinementPassCount, result.refinedLineCount,
            result.refinementPixelCount);
    }
#else
    result.error = unavailableMessage();
#endif
    return result;
}
