#include "metadatapanel.h"

#include "selectablelabel.h"

#include <QColorSpace>
#include <QFileInfo>
#include <QHeaderView>
#include <QImageReader>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#ifdef CLEARVEIL_HAVE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

#include <algorithm>
#include <cmath>

HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(260, 180);
    setAccessibleName(tr("RGB histogram"));
}

void HistogramWidget::setImage(const QImage &image)
{
    m_red.fill(0);
    m_green.fill(0);
    m_blue.fill(0);
    m_peak = 0;
    if (image.isNull()) {
        update();
        return;
    }

    QImage sample = image;
    constexpr qint64 maximumSamples = 1'000'000;
    const qint64 pixels = static_cast<qint64>(image.width()) * image.height();
    if (pixels > maximumSamples) {
        const qreal scale = std::sqrt(maximumSamples / static_cast<qreal>(pixels));
        sample = image.scaled(qRound(image.width() * scale),
                              qRound(image.height() * scale),
                              Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    sample = sample.convertToFormat(QImage::Format_RGBA8888);
    for (int y = 0; y < sample.height(); ++y) {
        const uchar *line = sample.constScanLine(y);
        for (int x = 0; x < sample.width(); ++x) {
            const uchar *pixel = line + x * 4;
            ++m_red.at(pixel[0]);
            ++m_green.at(pixel[1]);
            ++m_blue.at(pixel[2]);
        }
    }
    for (int value = 0; value < 256; ++value) {
        m_peak = std::max({m_peak, m_red.at(value),
                           m_green.at(value), m_blue.at(value)});
    }
    update();
}

void HistogramWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));
    const QRectF plot = QRectF(rect()).adjusted(10, 10, -10, -18);
    painter.setPen(palette().color(QPalette::Mid));
    painter.drawRect(plot);
    if (m_peak == 0)
        return;

    const auto drawChannel = [&](const std::array<quint32, 256> &channel,
                                 const QColor &color) {
        QPainterPath path;
        for (int value = 0; value < 256; ++value) {
            const qreal x = plot.left() + value * plot.width() / 255.0;
            const qreal normalized = std::log1p(channel.at(value))
                / std::log1p(m_peak);
            const qreal y = plot.bottom() - normalized * plot.height();
            if (value == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }
        painter.setPen(QPen(color, 1.5));
        painter.drawPath(path);
    };
    painter.setRenderHint(QPainter::Antialiasing);
    drawChannel(m_red, QColor(235, 72, 72, 205));
    drawChannel(m_green, QColor(54, 190, 104, 205));
    drawChannel(m_blue, QColor(72, 122, 235, 205));
}

MetadataPanel::MetadataPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    auto *tabs = new QTabWidget(this);
    m_tree = new QTreeWidget(tabs);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({tr("Property"), tr("Value")});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tree->header()->setMinimumSectionSize(72);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // The panel selects text inside its cell widgets, not whole metadata rows.
    // A row highlight underneath the text selection makes the actual copied
    // range difficult to distinguish.
    m_tree->setSelectionMode(QAbstractItemView::NoSelection);
    m_tree->setAlternatingRowColors(true);
    m_tree->setAnimated(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setIndentation(0);
    m_tree->setItemsExpandable(true);
    // Cell widgets provide the only visible text. Their height can differ from
    // the item delegate's default height, so each row supplies its own hint.
    m_tree->setUniformRowHeights(false);
    m_tree->setWordWrap(false);
    m_tree->setTextElideMode(Qt::ElideRight);
    m_tree->setAccessibleName(tr("Image metadata"));
    tabs->addTab(m_tree, tr("Details"));
    m_histogram = new HistogramWidget(tabs);
    tabs->addTab(m_histogram, tr("Histogram"));
    layout->addWidget(tabs);
}

void MetadataPanel::setImage(const QString &filePath, const QImage &image,
                             const QSize &logicalSize)
{
    clear();
    const QFileInfo file(filePath);
    addRow(tr("File name"), file.fileName().isEmpty() ? tr("Clipboard image")
                                                       : file.fileName(),
           tr("General"));
    const QSize displayedSize = logicalSize.isValid()
        ? logicalSize : image.size();
    addRow(tr("Dimensions"), tr("%1 × %2 pixels")
               .arg(displayedSize.width()).arg(displayedSize.height()),
           tr("General"));
    addRow(tr("Color depth"), tr("%1 bpp").arg(image.depth()), tr("General"));
    if (file.exists()) {
        addRow(tr("Format"),
               QString::fromLatin1(QImageReader::imageFormat(filePath)).toUpper(),
               tr("General"));
        addRow(tr("File size"), QString::number(file.size()), tr("General"));
        addRow(tr("Location"), file.absolutePath(), tr("General"));
        addRow(tr("Modified"), QLocale::system().toString(
                   file.lastModified(), QLocale::LongFormat),
               tr("General"));
    }
    if (image.colorSpace().isValid()) {
        addRow(tr("Color space"), image.colorSpace().description(),
               tr("Color"));
    }

    QImageReader reader(filePath);
    for (const QString &key : reader.textKeys()) {
        const QString value = reader.text(key).trimmed();
        if (!value.isEmpty())
            addRow(key, value, tr("Embedded text"));
    }

#ifdef CLEARVEIL_HAVE_EXIV2
    if (file.exists()) {
        try {
            auto metadataImage = Exiv2::ImageFactory::open(filePath.toStdString());
            if (metadataImage.get() != nullptr) {
                metadataImage->readMetadata();
                addRow(tr("MIME type"),
                       QString::fromStdString(metadataImage->mimeType()),
                       tr("General"));
                addRow(tr("Comment"),
                       QString::fromStdString(metadataImage->comment()),
                       tr("Embedded text"));
                const auto appendData = [this](const auto &data, const QString &group) {
                    static const QSet<QString> summaryKeys = {
                        QStringLiteral("Exif.Image.ImageWidth"),
                        QStringLiteral("Exif.Image.ImageLength"),
                        QStringLiteral("Exif.Photo.PixelXDimension"),
                        QStringLiteral("Exif.Photo.PixelYDimension"),
                    };
                    for (const auto &item : data) {
                        const QString label =
                            QString::fromStdString(item.tagLabel()).trimmed();
                        const QString key =
                            QString::fromStdString(item.key()).trimmed();
                        const QString value =
                            QString::fromStdString(item.toString()).trimmed();
                        if (value.isEmpty() || summaryKeys.contains(key))
                            continue;
                        const QString property = label.isEmpty() ? key : label;
                        addRow(property, value, group, key);
                    }
                };
                appendData(metadataImage->exifData(), QStringLiteral("EXIF"));
                appendData(metadataImage->iptcData(), QStringLiteral("IPTC"));
                appendData(metadataImage->xmpData(), QStringLiteral("XMP"));
            }
        } catch (const Exiv2::Error &) {
            // Metadata is supplementary; a malformed block must not prevent viewing.
        }
    }
#endif
    m_tree->clearSelection();
    m_tree->setCurrentItem(nullptr);
    m_histogram->setImage(image);
}

void MetadataPanel::clear()
{
    m_tree->clear();
    m_seenRows.clear();
    m_groups.clear();
    m_histogram->setImage({});
}

void MetadataPanel::addRow(const QString &name, const QString &value,
                           const QString &group, const QString &sourceKey)
{
    const QString cleanName = name.trimmed();
    const QString cleanValue = value.trimmed();
    if (cleanName.isEmpty() || cleanValue.isEmpty())
        return;
    const QString cleanGroup = group.trimmed();
    const QString identity = sourceKey.trimmed().isEmpty()
        ? cleanGroup + QChar(0x1e) + cleanName
        : sourceKey.trimmed();
    const QString fingerprint = identity.toCaseFolded()
        + QChar(0x1f) + cleanValue;
    if (m_seenRows.contains(fingerprint))
        return;
    m_seenRows.insert(fingerprint);

    QTreeWidgetItem *groupItem = m_groups.value(cleanGroup);
    if (!groupItem) {
        groupItem = new QTreeWidgetItem(m_tree);
        groupItem->setData(0, Qt::UserRole, cleanGroup);
        QFont groupFont = groupItem->font(0);
        groupFont.setBold(true);
        groupItem->setFont(0, groupFont);
        groupItem->setExpanded(true);
        groupItem->setFlags(groupItem->flags() & ~Qt::ItemIsSelectable);
        m_tree->setFirstColumnSpanned(
            m_tree->indexOfTopLevelItem(groupItem), QModelIndex(), true);
        auto *groupLabel = new SelectableLabel(cleanGroup, m_tree);
        groupLabel->setObjectName(
            QStringLiteral("metadataGroupText"));
        groupLabel->setFont(groupFont);
        groupLabel->setToolTip(cleanGroup);
        groupLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        groupLabel->setMinimumWidth(0);
        groupLabel->setSizePolicy(QSizePolicy::Ignored,
                                  QSizePolicy::Preferred);
        groupItem->setSizeHint(
            0, QSize(0, groupLabel->sizeHint().height() + 2));
        m_tree->setItemWidget(groupItem, 0, groupLabel);
        m_groups.insert(cleanGroup, groupItem);
    }

    auto *propertyItem = new QTreeWidgetItem(groupItem);
    propertyItem->setData(0, Qt::UserRole, cleanName);
    propertyItem->setData(1, Qt::UserRole, cleanValue);
    propertyItem->setToolTip(0, sourceKey.trimmed().isEmpty()
        ? cleanName : sourceKey.trimmed());
    propertyItem->setToolTip(1, cleanValue);

    auto *nameLabel = new SelectableLabel(cleanName, m_tree);
    nameLabel->setObjectName(QStringLiteral("metadataPropertyText"));
    nameLabel->setToolTip(propertyItem->toolTip(0));
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameLabel->setMinimumWidth(0);
    nameLabel->setSizePolicy(QSizePolicy::Ignored,
                             QSizePolicy::Preferred);
    auto *valueLabel = new SelectableLabel(cleanValue, m_tree);
    valueLabel->setObjectName(QStringLiteral("metadataValueText"));
    valueLabel->setToolTip(cleanValue);
    valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    valueLabel->setMinimumWidth(0);
    valueLabel->setSizePolicy(QSizePolicy::Ignored,
                              QSizePolicy::Preferred);
    const int rowHeight = std::max(nameLabel->sizeHint().height(),
                                   valueLabel->sizeHint().height()) + 2;
    propertyItem->setSizeHint(0, QSize(0, rowHeight));
    propertyItem->setSizeHint(1, QSize(0, rowHeight));
    m_tree->setItemWidget(propertyItem, 0, nameLabel);
    m_tree->setItemWidget(propertyItem, 1, valueLabel);
}
