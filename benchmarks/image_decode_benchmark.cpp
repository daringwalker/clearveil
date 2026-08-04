#include "imagedecoder.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QColorSpace>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

#include <algorithm>

namespace {
QStringList createCorpus(const QString &directoryPath, int imageCount,
                         const QSize &imageSize)
{
    QStringList paths;
    paths.reserve(imageCount);
    quint32 state = 0x9e3779b9U;
    for (int index = 0; index < imageCount; ++index) {
        QImage image(imageSize, QImage::Format_RGB32);
        image.setColorSpace(QColorSpace(QColorSpace::DisplayP3));
        for (int y = 0; y < image.height(); ++y) {
            auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                state = state * 1664525U + 1013904223U;
                line[x] = qRgb(
                    ((state >> 24) + x / 5 + index * 11) & 0xff,
                    ((state >> 16) + y / 3 + index * 7) & 0xff,
                    ((state >> 8) + x / 7 + y / 9) & 0xff);
            }
        }
        const QString path = QStringLiteral("%1/image-%2.jpg")
            .arg(directoryPath)
            .arg(index, 3, 10, QLatin1Char('0'));
        QImageWriter writer(path, "jpeg");
        writer.setQuality(90);
        if (!writer.write(image))
            return {};
        paths.append(path);
    }
    return paths;
}

qint64 median(QList<qint64> values)
{
    if (values.isEmpty())
        return -1;
    std::sort(values.begin(), values.end());
    const qsizetype middle = values.size() / 2;
    return values.size() % 2
        ? values.at(middle)
        : (values.at(middle - 1) + values.at(middle)) / 2;
}

QJsonArray jsonTimes(const QList<qint64> &values)
{
    QJsonArray array;
    for (qint64 value : values)
        array.append(value);
    return array;
}
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName(
        QStringLiteral("clearveil-image-decode-benchmark"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Deterministic Clearveil main-image decode benchmark"));
    parser.addHelpOption();
    const QCommandLineOption imageCountOption(
        QStringLiteral("images"), QStringLiteral("Corpus image count"),
        QStringLiteral("count"), QStringLiteral("6"));
    const QCommandLineOption runsOption(
        QStringLiteral("runs"), QStringLiteral("Measured corpus passes"),
        QStringLiteral("count"), QStringLiteral("3"));
    const QCommandLineOption widthOption(
        QStringLiteral("width"), QStringLiteral("Source image width"),
        QStringLiteral("pixels"), QStringLiteral("1920"));
    const QCommandLineOption heightOption(
        QStringLiteral("height"), QStringLiteral("Source image height"),
        QStringLiteral("pixels"), QStringLiteral("1080"));
    const QCommandLineOption limitOption(
        QStringLiteral("max-median-ms"),
        QStringLiteral("Exit with status 2 when median pass time exceeds this value; 0 disables the threshold"),
        QStringLiteral("milliseconds"), QStringLiteral("0"));
    const QCommandLineOption jsonOption(
        QStringLiteral("json"), QStringLiteral("Print machine-readable JSON"));
    parser.addOptions({imageCountOption, runsOption, widthOption,
                       heightOption, limitOption, jsonOption});
    parser.process(application);

    bool imageCountValid = false;
    bool runsValid = false;
    bool widthValid = false;
    bool heightValid = false;
    bool limitValid = false;
    const int imageCount = parser.value(imageCountOption).toInt(
        &imageCountValid);
    const int runs = parser.value(runsOption).toInt(&runsValid);
    const int width = parser.value(widthOption).toInt(&widthValid);
    const int height = parser.value(heightOption).toInt(&heightValid);
    const qint64 limit = parser.value(limitOption).toLongLong(&limitValid);
    if (!imageCountValid || imageCount < 1
        || !runsValid || runs < 1
        || !widthValid || width < 1
        || !heightValid || height < 1
        || !limitValid || limit < 0) {
        QTextStream(stderr) << "Invalid benchmark arguments.\n";
        return 64;
    }

    QTemporaryDir corpusDirectory;
    if (!corpusDirectory.isValid())
        return 1;
    const QSize imageSize(width, height);
    const QStringList paths = createCorpus(
        corpusDirectory.path(), imageCount, imageSize);
    if (paths.size() != imageCount) {
        QTextStream(stderr) << "Could not create the decode corpus.\n";
        return 1;
    }

    qint64 inputBytes = 0;
    for (const QString &path : paths)
        inputBytes += QFileInfo(path).size();

    QList<qint64> passTimes;
    qint64 decodedBytes = 0;
    qint64 decoderNanoseconds = 0;
    int colorConversions = 0;
    for (int run = 0; run < runs; ++run) {
        QElapsedTimer passTimer;
        passTimer.start();
        for (const QString &path : paths) {
            const ImageLoadResult result = ImageDecoder::decode(path);
            if (!result.succeeded()) {
                QTextStream(stderr)
                    << "Decode failed: " << result.error << '\n';
                return 1;
            }
            decodedBytes += result.metrics.decodedBytes;
            decoderNanoseconds += result.metrics.elapsedNanoseconds;
            colorConversions += result.metrics.convertedToSrgb ? 1 : 0;
        }
        passTimes.append(passTimer.elapsed());
    }

    const qint64 medianMilliseconds = median(passTimes);
    const double medianSeconds = std::max(0.001,
        medianMilliseconds / 1000.0);
    const double imagesPerSecond = imageCount / medianSeconds;
    const double megapixelsPerSecond =
        (static_cast<double>(imageCount) * width * height / 1'000'000.0)
        / medianSeconds;
    const int totalDecodes = imageCount * runs;
    const double averageDecoderMilliseconds = totalDecodes > 0
        ? decoderNanoseconds / 1'000'000.0 / totalDecodes : 0.0;

    const QJsonObject report{
        {QStringLiteral("images"), imageCount},
        {QStringLiteral("runs"), runs},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
        {QStringLiteral("input_bytes"), inputBytes},
        {QStringLiteral("pass_ms"), jsonTimes(passTimes)},
        {QStringLiteral("median_ms"), medianMilliseconds},
        {QStringLiteral("images_per_second"), imagesPerSecond},
        {QStringLiteral("megapixels_per_second"), megapixelsPerSecond},
        {QStringLiteral("average_decoder_ms"), averageDecoderMilliseconds},
        {QStringLiteral("decoded_bytes_total"), decodedBytes},
        {QStringLiteral("color_conversions"), colorConversions},
    };

    QTextStream output(stdout);
    if (parser.isSet(jsonOption)) {
        output << QJsonDocument(report).toJson(QJsonDocument::Indented);
    } else {
        output << "Clearveil main-image decode benchmark\n"
               << "  corpus: " << imageCount << " images, "
               << width << 'x' << height << " JPEG Display P3\n"
               << "  passes: ";
        for (qint64 time : passTimes)
            output << time << " ms ";
        output << "(median " << medianMilliseconds << " ms)\n"
               << "  throughput: " << imagesPerSecond
               << " images/s, " << megapixelsPerSecond << " MP/s\n"
               << "  average decoder call: "
               << averageDecoderMilliseconds << " ms\n";
    }
    output.flush();

    if (limit > 0 && medianMilliseconds > limit) {
        QTextStream(stderr)
            << "Median " << medianMilliseconds
            << " ms exceeded the configured limit of "
            << limit << " ms.\n";
        return 2;
    }
    return 0;
}
