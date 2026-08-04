#include "formatcapabilities.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImageWriter>
#include <QSysInfo>

#include <algorithm>

namespace {
[[maybe_unused]] constexpr const char *translationStrings[] = {
    QT_TRANSLATE_NOOP("FormatCapabilities", "Common"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Modern"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Professional"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Vector"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Legacy"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Camera RAW"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Other runtime formats"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "JPEG"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "PNG"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "GIF"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "BMP"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "WebP"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "TIFF"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Icon"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Portable bitmap"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "X11 bitmap"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "AVIF"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "HEIF / HEIC"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "JPEG XL"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "QOI"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "JPEG 2000"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "JPEG XR"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "OpenEXR"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "PSD / PSB"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "XCF"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "KRA / OpenRaster"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Radiance HDR"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "DirectDraw Surface"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "SVG"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "EPS"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "TGA"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "MNG"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "WBMP"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "PCX"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "SGI RGB"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Sun Raster"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "ICNS icon"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Qt"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Qt Image Formats"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "KImageFormats / Qt plugin"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Qt Image Formats / KImageFormats"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "KImageFormats + LibRaw"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Qt SVG"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Qt plugin"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "Installed Qt image plugin"),
    QT_TRANSLATE_NOOP("FormatCapabilities", "a compatible Qt image plugin"),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "The file no longer exists or was moved."),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "Clearveil cannot read this file. Check its permissions and storage device."),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "This format is available, but the file contents could not be decoded. The file may be damaged, incomplete, or use an unsupported variant."),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "The file has no extension and its contents were not recognized as a supported image."),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "No decoder for .%1 images is available in this Clearveil installation."),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "\n\nOn %1, install these packages: %2"),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "\n\nInstallation command:\n%1"),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "\n\nPackage names commonly used by Linux distributions: %1"),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "\n\nRestart Clearveil after installation."),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "\n\nSuggested backend: %1"),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "This Clearveil process is running in a Flatpak sandbox. Host packages may not be visible inside the sandbox; install a Clearveil/Flatpak build that includes the required image plugin."),
    QT_TRANSLATE_NOOP("FormatCapabilities",
                      "\n\nDecoder details: %1"),
};

struct FormatDefinition
{
    const char *category;
    const char *name;
    const char *extensions;
    const char *backend;
};

QString translated(const char *text)
{
    return QCoreApplication::translate("FormatCapabilities", text);
}

QSet<QString> normalizedFormats(const QList<QByteArray> &formats)
{
    QSet<QString> result;
    for (const QByteArray &format : formats)
        result.insert(QString::fromLatin1(format).toLower());
    if (result.contains(QStringLiteral("jpeg")))
        result.unite({QStringLiteral("jpg"), QStringLiteral("jpe")});
    if (result.contains(QStringLiteral("jpg")))
        result.unite({QStringLiteral("jpeg"), QStringLiteral("jpe")});
    if (result.contains(QStringLiteral("tiff")))
        result.insert(QStringLiteral("tif"));
    if (result.contains(QStringLiteral("tif")))
        result.insert(QStringLiteral("tiff"));
    if (result.contains(QStringLiteral("svg")))
        result.insert(QStringLiteral("svgz"));
    if (result.contains(QStringLiteral("heif")))
        result.insert(QStringLiteral("heic"));
    if (result.contains(QStringLiteral("heic")))
        result.insert(QStringLiteral("heif"));
    return result;
}

const QList<FormatDefinition> &knownFormats()
{
    static const QList<FormatDefinition> definitions{
        {"Common", "JPEG", "jpg jpeg jpe", "Qt"},
        {"Common", "PNG", "png", "Qt"},
        {"Common", "GIF", "gif", "Qt"},
        {"Common", "BMP", "bmp dib", "Qt"},
        {"Common", "WebP", "webp", "Qt Image Formats"},
        {"Common", "TIFF", "tif tiff", "Qt Image Formats"},
        {"Common", "Icon", "ico cur", "Qt"},
        {"Common", "Portable bitmap", "pbm pgm ppm", "Qt"},
        {"Common", "X11 bitmap", "xbm xpm", "Qt"},

        {"Modern", "AVIF", "avif", "KImageFormats / Qt plugin"},
        {"Modern", "HEIF / HEIC", "heif heic", "KImageFormats / Qt plugin"},
        {"Modern", "JPEG XL", "jxl", "KImageFormats / Qt plugin"},
        {"Modern", "QOI", "qoi", "KImageFormats / Qt plugin"},
        {"Modern", "JPEG 2000", "jp2 j2k jpc jpf jpx",
         "Qt Image Formats / KImageFormats"},
        {"Modern", "JPEG XR", "jxr wdp hdp", "KImageFormats / Qt plugin"},

        {"Professional", "OpenEXR", "exr", "KImageFormats / Qt plugin"},
        {"Professional", "PSD / PSB", "psd psb", "KImageFormats / Qt plugin"},
        {"Professional", "XCF", "xcf", "KImageFormats / Qt plugin"},
        {"Professional", "KRA / OpenRaster", "kra ora",
         "KImageFormats / Qt plugin"},
        {"Professional", "Radiance HDR", "hdr pic",
         "KImageFormats / Qt plugin"},
        {"Professional", "DirectDraw Surface", "dds", "Qt plugin"},

        {"Vector", "SVG", "svg svgz", "Qt SVG"},
        {"Vector", "EPS", "eps epsi epsf", "KImageFormats / Qt plugin"},

        {"Legacy", "TGA", "tga", "Qt Image Formats"},
        {"Legacy", "MNG", "mng", "Qt Image Formats"},
        {"Legacy", "WBMP", "wbmp", "Qt Image Formats"},
        {"Legacy", "PCX", "pcx", "KImageFormats / Qt plugin"},
        {"Legacy", "SGI RGB", "rgb rgba sgi bw",
         "KImageFormats / Qt plugin"},
        {"Legacy", "Sun Raster", "ras", "KImageFormats / Qt plugin"},
        {"Legacy", "ICNS icon", "icns", "Qt Image Formats"},

        {"Camera RAW", "Camera RAW",
         "3fr arw bay bmq cap cine cr2 cr3 crw cs1 dc2 dcr dng erf fff iiq "
         "k25 kc2 kdc mdc mef mos mrw nef nrw orf pef ptx pxn qtk raf raw "
         "rdc rw2 rwl rwz sr2 srf srw sti x3f",
         "KImageFormats + LibRaw"},
    };
    return definitions;
}

bool intersects(const QSet<QString> &formats,
                const QStringList &extensions)
{
    return std::any_of(
        extensions.cbegin(), extensions.cend(),
        [&formats](const QString &extension) {
            return formats.contains(extension);
        });
}

QString normalizedExtension(const QString &extension)
{
    QString result = extension.trimmed().toLower();
    while (result.startsWith(QLatin1Char('.')))
        result.remove(0, 1);
    return result;
}

struct DistributionInfo
{
    QString id;
    QStringList likes;
    QString prettyName;
};

QString unquoteOsReleaseValue(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2
        && ((value.startsWith(QLatin1Char('"'))
             && value.endsWith(QLatin1Char('"')))
            || (value.startsWith(QLatin1Char('\''))
                && value.endsWith(QLatin1Char('\''))))) {
        value = value.mid(1, value.size() - 2);
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
            const QString value =
                unquoteOsReleaseValue(line.mid(equals + 1));
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
        [&ids](const QString &like) {
            return ids.contains(like);
        });
}

enum class PackageFamily
{
    QtImageFormats,
    QtSvg,
    KImageFormats,
};

PackageFamily packageFamilyForExtension(const QString &extension)
{
    static const QSet<QString> svg{
        QStringLiteral("svg"), QStringLiteral("svgz"),
    };
    static const QSet<QString> qtImageFormats{
        QStringLiteral("webp"),
        QStringLiteral("tif"), QStringLiteral("tiff"),
        QStringLiteral("tga"), QStringLiteral("mng"),
        QStringLiteral("wbmp"), QStringLiteral("icns"),
        QStringLiteral("jp2"), QStringLiteral("j2k"),
        QStringLiteral("jpc"), QStringLiteral("jpf"),
        QStringLiteral("jpx"),
    };
    if (svg.contains(extension))
        return PackageFamily::QtSvg;
    if (qtImageFormats.contains(extension))
        return PackageFamily::QtImageFormats;
    return PackageFamily::KImageFormats;
}

QStringList archPackagesForExtension(
    const QString &extension, PackageFamily family)
{
    if (family == PackageFamily::QtSvg)
        return {QStringLiteral("qt6-svg")};
    if (family == PackageFamily::QtImageFormats)
        return {QStringLiteral("qt6-imageformats")};

    QStringList packages{QStringLiteral("kimageformats")};
    static const QSet<QString> rawExtensions{
        QStringLiteral("3fr"), QStringLiteral("arw"),
        QStringLiteral("bay"), QStringLiteral("bmq"),
        QStringLiteral("cap"), QStringLiteral("cine"),
        QStringLiteral("cr2"), QStringLiteral("cr3"),
        QStringLiteral("crw"), QStringLiteral("cs1"),
        QStringLiteral("dc2"), QStringLiteral("dcr"),
        QStringLiteral("dng"), QStringLiteral("erf"),
        QStringLiteral("fff"), QStringLiteral("iiq"),
        QStringLiteral("k25"), QStringLiteral("kc2"),
        QStringLiteral("kdc"), QStringLiteral("mdc"),
        QStringLiteral("mef"), QStringLiteral("mos"),
        QStringLiteral("mrw"), QStringLiteral("nef"),
        QStringLiteral("nrw"), QStringLiteral("orf"),
        QStringLiteral("pef"), QStringLiteral("ptx"),
        QStringLiteral("pxn"), QStringLiteral("qtk"),
        QStringLiteral("raf"), QStringLiteral("raw"),
        QStringLiteral("rdc"), QStringLiteral("rw2"),
        QStringLiteral("rwl"), QStringLiteral("rwz"),
        QStringLiteral("sr2"), QStringLiteral("srf"),
        QStringLiteral("srw"), QStringLiteral("sti"),
        QStringLiteral("x3f"),
    };
    if (extension == QStringLiteral("heif")
        || extension == QStringLiteral("heic")) {
        packages.append(QStringLiteral("libheif"));
    } else if (extension == QStringLiteral("avif")) {
        packages.append(QStringLiteral("libavif"));
    } else if (extension == QStringLiteral("jxl")) {
        packages.append(QStringLiteral("libjxl"));
    } else if (extension == QStringLiteral("exr")) {
        packages.append(QStringLiteral("openexr"));
    } else if (rawExtensions.contains(extension)) {
        packages.append(QStringLiteral("libraw"));
    }
    return packages;
}
}

QList<ImageFormatCapability> FormatCapabilities::capabilities()
{
    const QSet<QString> readers =
        normalizedFormats(QImageReader::supportedImageFormats());
    const QSet<QString> writers =
        normalizedFormats(QImageWriter::supportedImageFormats());
    QSet<QString> described;
    QList<ImageFormatCapability> result;

    for (const FormatDefinition &definition : knownFormats()) {
        const QStringList extensions =
            QString::fromLatin1(definition.extensions)
                .split(QLatin1Char(' '),
                       Qt::SkipEmptyParts);
        for (const QString &extension : extensions)
            described.insert(extension);
        result.append({
            translated(definition.category),
            translated(definition.name),
            extensions,
            intersects(readers, extensions),
            intersects(writers, extensions),
            translated(definition.backend),
        });
    }

    QSet<QString> other = readers;
    other.unite(writers);
    other.subtract(described);
    QStringList sortedOther(other.cbegin(), other.cend());
    sortedOther.sort(Qt::CaseInsensitive);
    for (const QString &extension : std::as_const(sortedOther)) {
        result.append({
            translated("Other runtime formats"),
            extension.toUpper(),
            {extension},
            readers.contains(extension),
            writers.contains(extension),
            translated("Installed Qt image plugin"),
        });
    }
    return result;
}

QSet<QString> FormatCapabilities::readableExtensions()
{
    return normalizedFormats(QImageReader::supportedImageFormats());
}

QSet<QString> FormatCapabilities::writableExtensions()
{
    return normalizedFormats(QImageWriter::supportedImageFormats());
}

bool FormatCapabilities::canReadExtension(const QString &extension)
{
    return readableExtensions().contains(
        normalizedExtension(extension));
}

QString FormatCapabilities::imageDialogPatterns()
{
    QSet<QString> patterns;
    for (const ImageFormatCapability &capability : capabilities()) {
        for (const QString &extension : capability.extensions)
            patterns.insert(QStringLiteral("*.") + extension);
    }
    QStringList sorted(patterns.cbegin(), patterns.cend());
    sorted.sort(Qt::CaseInsensitive);
    return sorted.join(QLatin1Char(' '));
}

ImageFormatInstallationAdvice FormatCapabilities::installationAdvice(
    const QString &extension,
    const QString &distributionId)
{
    const QString normalized = normalizedExtension(extension);
    const PackageFamily family =
        packageFamilyForExtension(normalized);
    DistributionInfo distribution = currentDistribution();
    if (!distributionId.trimmed().isEmpty()) {
        distribution.id = distributionId.trimmed().toLower();
        distribution.likes.clear();
        distribution.prettyName.clear();
    }

    ImageFormatInstallationAdvice advice;
    if (belongsTo(distribution, {
            QStringLiteral("arch"),
            QStringLiteral("manjaro"),
            QStringLiteral("endeavouros"),
        })) {
        advice.distribution = distribution.prettyName.isEmpty()
            ? QStringLiteral("Arch Linux")
            : distribution.prettyName;
        advice.packages =
            archPackagesForExtension(normalized, family);
        advice.command =
            QStringLiteral("sudo pacman -S %1")
                .arg(advice.packages.join(QLatin1Char(' ')));
    } else if (belongsTo(distribution, {
                   QStringLiteral("debian"),
                   QStringLiteral("ubuntu"),
                   QStringLiteral("linuxmint"),
                   QStringLiteral("pop"),
                   QStringLiteral("neon"),
               })) {
        advice.distribution = distribution.prettyName.isEmpty()
            ? QStringLiteral("Debian / Ubuntu")
            : distribution.prettyName;
        if (family == PackageFamily::QtSvg)
            advice.packages = {QStringLiteral("qt6-svg-plugins")};
        else if (family == PackageFamily::QtImageFormats)
            advice.packages = {
                QStringLiteral("qt6-image-formats-plugins"),
            };
        else
            advice.packages = {
                QStringLiteral("kimageformat6-plugins"),
            };
        advice.command =
            QStringLiteral("sudo apt install %1")
                .arg(advice.packages.join(QLatin1Char(' ')));
    } else if (belongsTo(distribution, {
                   QStringLiteral("fedora"),
                   QStringLiteral("rhel"),
                   QStringLiteral("centos"),
               })) {
        advice.distribution = distribution.prettyName.isEmpty()
            ? QStringLiteral("Fedora")
            : distribution.prettyName;
        if (family == PackageFamily::QtSvg)
            advice.packages = {QStringLiteral("qt6-qtsvg")};
        else if (family == PackageFamily::QtImageFormats)
            advice.packages = {QStringLiteral("qt6-qtimageformats")};
        else
            advice.packages = {QStringLiteral("kf6-kimageformats")};
        advice.command =
            QStringLiteral("sudo dnf install %1")
                .arg(advice.packages.join(QLatin1Char(' ')));
    } else {
        advice.distribution = distribution.prettyName;
        if (family == PackageFamily::QtSvg) {
            advice.packages = {
                QStringLiteral("qt6-svg"),
                QStringLiteral("qt6-svg-plugins"),
                QStringLiteral("qt6-qtsvg"),
            };
        } else if (family == PackageFamily::QtImageFormats) {
            advice.packages = {
                QStringLiteral("qt6-imageformats"),
                QStringLiteral("qt6-image-formats-plugins"),
                QStringLiteral("qt6-qtimageformats"),
            };
        } else {
            advice.packages = {
                QStringLiteral("kimageformats"),
                QStringLiteral("kimageformat6-plugins"),
                QStringLiteral("kf6-kimageformats"),
            };
        }
    }

    if (qEnvironmentVariableIsSet("FLATPAK_ID")) {
        advice.command.clear();
        advice.note = translated(
            "This Clearveil process is running in a Flatpak sandbox. "
            "Host packages may not be visible inside the sandbox; install a "
            "Clearveil/Flatpak build that includes the required image plugin.");
    }
    return advice;
}

QString FormatCapabilities::friendlyDecodeError(
    const QString &filePath,
    QImageReader::ImageReaderError error,
    const QString &decoderDetails)
{
    const QFileInfo file(filePath);
    if (!file.exists()) {
        return translated(
            "The file no longer exists or was moved.");
    }
    if (!file.isReadable()
        || error == QImageReader::DeviceError) {
        return translated(
            "Clearveil cannot read this file. Check its permissions and storage device.");
    }

    const QString extension =
        normalizedExtension(file.suffix());
    const QString details = decoderDetails.trimmed();
    if (error == QImageReader::InvalidDataError
        || canReadExtension(extension)) {
        return translated(
                   "This format is available, but the file contents could not be decoded. "
                   "The file may be damaged, incomplete, or use an unsupported variant.")
            + (details.isEmpty()
                   ? QString()
                   : translated("\n\nDecoder details: %1").arg(details));
    }

    if (extension.isEmpty()) {
        return translated(
                   "The file has no extension and its contents were not recognized as a supported image.")
            + (details.isEmpty()
                   ? QString()
                   : translated("\n\nDecoder details: %1").arg(details));
    }

    QString backend = translated("a compatible Qt image plugin");
    for (const ImageFormatCapability &capability : capabilities()) {
        if (capability.extensions.contains(extension)) {
            backend = capability.backend;
            break;
        }
    }
    const ImageFormatInstallationAdvice advice =
        installationAdvice(extension);
    QString message = translated(
        "No decoder for .%1 images is available in this Clearveil installation.")
                          .arg(extension.toUpper());
    if (!advice.command.isEmpty()) {
        message += translated(
            "\n\nOn %1, install these packages: %2")
                       .arg(advice.distribution,
                            advice.packages.join(
                                QStringLiteral(", ")));
        message += translated(
            "\n\nInstallation command:\n%1")
                       .arg(advice.command);
    } else {
        message += translated(
            "\n\nPackage names commonly used by Linux distributions: %1")
                       .arg(advice.packages.join(
                           QStringLiteral(", ")));
    }
    if (!advice.note.isEmpty())
        message += QLatin1String("\n\n") + advice.note;
    message += translated(
        "\n\nRestart Clearveil after installation.");
    message += translated(
        "\n\nSuggested backend: %1").arg(backend);
    if (!details.isEmpty())
        message += translated("\n\nDecoder details: %1").arg(details);
    return message;
}
