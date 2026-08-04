// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPalette>
#include <QString>
#include <QStringList>

class BreezeTheme final
{
public:
    enum class Variant {
        Light,
        Dark,
    };

    [[nodiscard]] static QPalette palette(Variant variant);
    [[nodiscard]] static QString colorSchemePath(Variant variant);
    [[nodiscard]] static QString preferredStyleName(
        const QStringList &availableStyleNames,
        bool nativeStylesAvailable = true);
};
