#include "persistentthumbnailcache.h"
#include "thumbnailmodel.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QImageWriter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>

#include <algorithm>
#include <optional>

namespace {
QStringList createCorpus(const QString &directoryPath, int imageCount)
{
    QStringList paths;
    paths.reserve(imageCount);
    quint32 state = 0x6d2b79f5U;
    for (int index = 0; index < imageCount; ++index) {
        QImage image(1280, 720, QImage::Format_RGB32);
        for (int y = 0; y < image.height(); ++y) {
            auto *line =
                reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                state = state * 1664525U + 1013904223U;
                const int red =
                    ((state >> 24) + x + index * 3) & 0xff;
                const int green =
                    ((state >> 16) + y + index * 5) & 0xff;
                const int blue =
                    ((state >> 8) + x / 3 + y / 2) & 0xff;
                line[x] = qRgb(red, green, blue);
            }
        }
        const QString path = QStringLiteral("%1/image-%2.jpg")
            .arg(directoryPath)
            .arg(index, 4, 10, QLatin1Char('0'));
        QImageWriter writer(path, "jpeg");
        writer.setQuality(88);
        if (!writer.write(image))
            return {};
        paths.append(path);
    }
    return paths;
}

std::optional<qint64> measureThumbnailPass(
    const QStringList &paths, int timeoutMilliseconds)
{
    ThumbnailModel model;
    model.setThumbnailSize(QSize(192, 128));
    model.setFiles(paths);
    if (model.rowCount() != paths.size())
        return std::nullopt;

    QSet<int> completedRows;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(
        &timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(
        &model, &QAbstractItemModel::dataChanged, &loop,
        [&completedRows, &loop, expected = paths.size()](
            const QModelIndex &first, const QModelIndex &last,
            const QList<int> &roles) {
        if (!roles.isEmpty()
            && !roles.contains(Qt::DecorationRole)) {
            return;
        }
        for (int row = first.row(); row <= last.row(); ++row)
            completedRows.insert(row);
        if (completedRows.size() >= expected)
            loop.quit();
    });

    QElapsedTimer elapsed;
    elapsed.start();
    for (int row = 0; row < model.rowCount(); ++row)
        model.data(model.index(row), Qt::DecorationRole);
    timeout.start(timeoutMilliseconds);
    if (completedRows.size() < paths.size())
        loop.exec();
    timeout.stop();
    if (completedRows.size() != paths.size())
        return std::nullopt;
    return elapsed.elapsed();
}

qint64 median(QList<qint64> values)
{
    if (values.isEmpty())
        return -1;
    std::sort(values.begin(), values.end());
    const qsizetype middle = values.size() / 2;
    if (values.size() % 2 != 0)
        return values.at(middle);
    return (values.at(middle - 1)
            + values.at(middle)) / 2;
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
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(
        QStringLiteral("clearveil-thumbnail-benchmark"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral(
            "Deterministic Clearveil thumbnail pipeline benchmark"));
    parser.addHelpOption();
    const QCommandLineOption imageCountOption(
        QStringLiteral("images"),
        QStringLiteral("Number of deterministic source images"),
        QStringLiteral("count"), QStringLiteral("200"));
    const QCommandLineOption runsOption(
        QStringLiteral("runs"),
        QStringLiteral("Number of measured passes per mode"),
        QStringLiteral("count"), QStringLiteral("3"));
    const QCommandLineOption timeoutOption(
        QStringLiteral("timeout-ms"),
        QStringLiteral("Timeout for each thumbnail pass"),
        QStringLiteral("milliseconds"), QStringLiteral("30000"));
    const QCommandLineOption limitOption(
        QStringLiteral("max-session-median-ms"),
        QStringLiteral(
            "Exit with status 2 when the no-disk median exceeds this value; 0 disables the threshold"),
        QStringLiteral("milliseconds"), QStringLiteral("0"));
    const QCommandLineOption jsonOption(
        QStringLiteral("json"),
        QStringLiteral("Print machine-readable JSON"));
    parser.addOptions({
        imageCountOption, runsOption, timeoutOption,
        limitOption, jsonOption,
    });
    parser.process(application);

    bool validImages = false;
    bool validRuns = false;
    bool validTimeout = false;
    bool validLimit = false;
    const int imageCount =
        parser.value(imageCountOption).toInt(&validImages);
    const int runs = parser.value(runsOption).toInt(&validRuns);
    const int timeoutMilliseconds =
        parser.value(timeoutOption).toInt(&validTimeout);
    const qint64 limit =
        parser.value(limitOption).toLongLong(&validLimit);
    if (!validImages || imageCount < 1
        || !validRuns || runs < 1
        || !validTimeout || timeoutMilliseconds < 1
        || !validLimit || limit < 0) {
        QTextStream(stderr)
            << "Invalid benchmark arguments.\n";
        return 64;
    }

    QTemporaryDir corpusDirectory;
    QTemporaryDir cacheParent;
    if (!corpusDirectory.isValid() || !cacheParent.isValid())
        return 1;
    const QStringList paths =
        createCorpus(corpusDirectory.path(), imageCount);
    if (paths.size() != imageCount) {
        QTextStream(stderr)
            << "Could not create the deterministic image corpus.\n";
        return 1;
    }

    QList<qint64> sessionTimes;
    PersistentThumbnailCache::configure(
        false, PersistentThumbnailCache::defaultMaximumBytes);
    for (int run = 0; run < runs; ++run) {
        const auto elapsed =
            measureThumbnailPass(paths, timeoutMilliseconds);
        if (!elapsed) {
            QTextStream(stderr)
                << "The session-cache pass timed out.\n";
            return 1;
        }
        sessionTimes.append(*elapsed);
    }

    const QString cacheDirectory =
        cacheParent.filePath(QStringLiteral("thumbnails"));
    PersistentThumbnailCache::configure(
        true, 512LL * 1024LL * 1024LL, cacheDirectory);
    PersistentThumbnailCache::clear();
    const auto fillTime =
        measureThumbnailPass(paths, timeoutMilliseconds);
    if (!fillTime) {
        QTextStream(stderr)
            << "The persistent-cache fill pass timed out.\n";
        return 1;
    }

    QList<qint64> persistentWarmTimes;
    for (int run = 0; run < runs; ++run) {
        const auto elapsed =
            measureThumbnailPass(paths, timeoutMilliseconds);
        if (!elapsed) {
            QTextStream(stderr)
                << "The persistent-cache warm pass timed out.\n";
            return 1;
        }
        persistentWarmTimes.append(*elapsed);
    }
    const qint64 cacheBytes =
        PersistentThumbnailCache::sizeBytes();
    PersistentThumbnailCache::configure(
        false, PersistentThumbnailCache::defaultMaximumBytes);

    const qint64 sessionMedian = median(sessionTimes);
    const qint64 persistentWarmMedian =
        median(persistentWarmTimes);
    QJsonObject report{
        {QStringLiteral("images"), imageCount},
        {QStringLiteral("runs"), runs},
        {QStringLiteral("thumbnail_width"), 192},
        {QStringLiteral("thumbnail_height"), 128},
        {QStringLiteral("session_no_disk_ms"),
         jsonTimes(sessionTimes)},
        {QStringLiteral("session_no_disk_median_ms"),
         sessionMedian},
        {QStringLiteral("persistent_fill_ms"), *fillTime},
        {QStringLiteral("persistent_warm_ms"),
         jsonTimes(persistentWarmTimes)},
        {QStringLiteral("persistent_warm_median_ms"),
         persistentWarmMedian},
        {QStringLiteral("persistent_cache_bytes"),
         cacheBytes},
    };

    QTextStream output(stdout);
    if (parser.isSet(jsonOption)) {
        output << QJsonDocument(report).toJson(
            QJsonDocument::Indented);
    } else {
        output
            << "Clearveil thumbnail benchmark\n"
            << "  corpus: " << imageCount
            << " images, 1280x720 JPEG\n"
            << "  thumbnail: 192x128\n"
            << "  no-disk session passes: ";
        for (const qint64 value : sessionTimes)
            output << value << " ms ";
        output << "(median " << sessionMedian << " ms)\n"
               << "  persistent cache fill: "
               << *fillTime << " ms\n"
               << "  persistent cache warm passes: ";
        for (const qint64 value : persistentWarmTimes)
            output << value << " ms ";
        output << "(median " << persistentWarmMedian
               << " ms)\n"
               << "  persistent cache size: "
               << cacheBytes << " bytes\n";
    }
    output.flush();

    if (limit > 0 && sessionMedian > limit) {
        QTextStream(stderr)
            << "Session median " << sessionMedian
            << " ms exceeded the configured limit of "
            << limit << " ms.\n";
        return 2;
    }
    return 0;
}
