// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ocrresult.h"

#include <QImage>
#include <QString>
#include <QStringList>

struct OcrRecognitionOptions
{
    bool recoverMissingText = true;
    bool refineSmallText = true;
};

class OcrEngine final
{
public:
    static constexpr int maximumInputDimension = 4096;
    static constexpr int maximumRefinementPasses = 16;

    [[nodiscard]] static bool isAvailable();
    [[nodiscard]] static QStringList availableLanguages();
    [[nodiscard]] static QString recognitionLanguages(
        const QStringList &available = availableLanguages());
    [[nodiscard]] static QString unavailableMessage();
    [[nodiscard]] static OcrResult recognize(
        const QImage &image,
        const QSize &logicalImageSize,
        const QString &languages,
        const OcrRecognitionOptions &options = {});
};
