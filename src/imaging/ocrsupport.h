// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

struct OcrInstallationAdvice
{
    QString distribution;
    QStringList packages;
    QString command;
    QString note;
};

class OcrSupport final
{
public:
    [[nodiscard]] static OcrInstallationAdvice installationAdvice(
        const QString &distributionId = {},
        bool developmentFilesRequired = false);
};
