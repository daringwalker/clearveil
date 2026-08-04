// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrsupport.h"

#include <QCoreApplication>
#include <QFile>
#include <QSysInfo>

#include <algorithm>

namespace {
[[maybe_unused]] constexpr const char *translationStrings[] = {
    QT_TRANSLATE_NOOP(
        "OcrSupport",
        "This Clearveil build was compiled without OCR. Installing runtime packages cannot add OCR to the existing binary; install an OCR-enabled Clearveil package, or install the development files and rebuild Clearveil."),
    QT_TRANSLATE_NOOP(
        "OcrSupport",
        "Install only the language models you need. Restart Clearveil after installation; every installed Tesseract language model is detected automatically."),
    QT_TRANSLATE_NOOP(
        "OcrSupport",
        "This Clearveil process is running in a Flatpak sandbox. Host Tesseract packages may not be visible inside the sandbox; use a Clearveil Flatpak that includes OCR and the required language models."),
};

struct DistributionInfo
{
    QString id;
    QStringList likes;
    QString prettyName;
};

QString translated(const char *text)
{
    return QCoreApplication::translate("OcrSupport", text);
}

QString unquote(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2
        && ((value.startsWith(QLatin1Char('"'))
             && value.endsWith(QLatin1Char('"')))
            || (value.startsWith(QLatin1Char('\''))
                && value.endsWith(QLatin1Char('\''))))) {
        return value.mid(1, value.size() - 2);
    }
    return value;
}

DistributionInfo currentDistribution()
{
    DistributionInfo result;
    QFile release(QStringLiteral("/etc/os-release"));
    if (release.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!release.atEnd()) {
            const QString line =
                QString::fromUtf8(release.readLine()).trimmed();
            const qsizetype equals = line.indexOf(QLatin1Char('='));
            if (equals <= 0)
                continue;
            const QString key = line.left(equals);
            const QString value = unquote(line.mid(equals + 1));
            if (key == QStringLiteral("ID"))
                result.id = value.toLower();
            else if (key == QStringLiteral("ID_LIKE"))
                result.likes = value.toLower().split(
                    QLatin1Char(' '), Qt::SkipEmptyParts);
            else if (key == QStringLiteral("PRETTY_NAME"))
                result.prettyName = value;
        }
    }
    if (result.id.isEmpty())
        result.id = QSysInfo::productType().toLower();
    if (result.prettyName.isEmpty())
        result.prettyName = QSysInfo::prettyProductName();
    return result;
}

bool belongsTo(const DistributionInfo &distribution,
               const QStringList &ids)
{
    if (ids.contains(distribution.id))
        return true;
    return std::any_of(
        distribution.likes.cbegin(), distribution.likes.cend(),
        [&ids](const QString &like) { return ids.contains(like); });
}
}

OcrInstallationAdvice OcrSupport::installationAdvice(
    const QString &distributionId,
    bool developmentFilesRequired)
{
    DistributionInfo distribution = currentDistribution();
    if (!distributionId.trimmed().isEmpty()) {
        distribution.id = distributionId.trimmed().toLower();
        distribution.likes.clear();
        distribution.prettyName.clear();
    }

    OcrInstallationAdvice advice;
    if (belongsTo(distribution, {
            QStringLiteral("arch"), QStringLiteral("manjaro"),
            QStringLiteral("endeavouros"),
        })) {
        advice.distribution = distribution.prettyName.isEmpty()
            ? QStringLiteral("Arch Linux") : distribution.prettyName;
        advice.packages = {
            QStringLiteral("tesseract"),
            QStringLiteral("tesseract-data-eng"),
            QStringLiteral("tesseract-data-chi_sim"),
        };
        advice.command = QStringLiteral("sudo pacman -S %1")
                             .arg(advice.packages.join(QLatin1Char(' ')));
    } else if (belongsTo(distribution, {
                   QStringLiteral("debian"), QStringLiteral("ubuntu"),
                   QStringLiteral("linuxmint"), QStringLiteral("pop"),
                   QStringLiteral("neon"),
               })) {
        advice.distribution = distribution.prettyName.isEmpty()
            ? QStringLiteral("Debian / Ubuntu") : distribution.prettyName;
        advice.packages = {
            QStringLiteral("tesseract-ocr"),
            QStringLiteral("tesseract-ocr-eng"),
            QStringLiteral("tesseract-ocr-chi-sim"),
        };
        if (developmentFilesRequired)
            advice.packages.prepend(QStringLiteral("libtesseract-dev"));
        advice.command = QStringLiteral("sudo apt install %1")
                             .arg(advice.packages.join(QLatin1Char(' ')));
    } else if (belongsTo(distribution, {
                   QStringLiteral("fedora"), QStringLiteral("rhel"),
                   QStringLiteral("centos"),
               })) {
        advice.distribution = distribution.prettyName.isEmpty()
            ? QStringLiteral("Fedora") : distribution.prettyName;
        advice.packages = {
            QStringLiteral("tesseract"),
            QStringLiteral("tesseract-langpack-eng"),
            QStringLiteral("tesseract-langpack-chi_sim"),
        };
        if (developmentFilesRequired)
            advice.packages.prepend(QStringLiteral("tesseract-devel"));
        advice.command = QStringLiteral("sudo dnf install %1")
                             .arg(advice.packages.join(QLatin1Char(' ')));
    } else {
        advice.distribution = distribution.prettyName;
        advice.packages = {
            QStringLiteral("Tesseract OCR"),
            QStringLiteral("English traineddata (eng)"),
            QStringLiteral("Simplified Chinese traineddata (chi_sim)"),
        };
        if (developmentFilesRequired) {
            advice.packages.prepend(
                QStringLiteral("Tesseract development files"));
        }
    }

    if (developmentFilesRequired) {
        advice.note = translated(
            "This Clearveil build was compiled without OCR. Installing runtime packages cannot add OCR to the existing binary; install an OCR-enabled Clearveil package, or install the development files and rebuild Clearveil.");
    } else {
        advice.note = translated(
            "Install only the language models you need. Restart Clearveil after installation; every installed Tesseract language model is detected automatically.");
    }
    if (qEnvironmentVariableIsSet("FLATPAK_ID")) {
        advice.command.clear();
        advice.note = translated(
            "This Clearveil process is running in a Flatpak sandbox. Host Tesseract packages may not be visible inside the sandbox; use a Clearveil Flatpak that includes OCR and the required language models.");
    }
    return advice;
}
