#pragma once

#include <QImageReader>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

struct ImageFormatCapability
{
    QString category;
    QString name;
    QStringList extensions;
    bool readable = false;
    bool writable = false;
    QString backend;
};

struct ImageFormatInstallationAdvice
{
    QString distribution;
    QStringList packages;
    QString command;
    QString note;
};

class FormatCapabilities
{
public:
    [[nodiscard]] static QList<ImageFormatCapability> capabilities();
    [[nodiscard]] static QSet<QString> readableExtensions();
    [[nodiscard]] static QSet<QString> writableExtensions();
    [[nodiscard]] static bool canReadExtension(const QString &extension);
    [[nodiscard]] static QString imageDialogPatterns();
    [[nodiscard]] static ImageFormatInstallationAdvice installationAdvice(
        const QString &extension,
        const QString &distributionId = {});
    [[nodiscard]] static QString friendlyDecodeError(
        const QString &filePath,
        QImageReader::ImageReaderError error,
        const QString &decoderDetails);
};
