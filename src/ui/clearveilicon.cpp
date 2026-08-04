#include "clearveilicon.h"

#include <QApplication>
#include <QFile>
#include <QIconEngine>
#include <QPainter>
#include <QPalette>
#include <QSvgRenderer>

#include <algorithm>

namespace {
class PaletteSvgIconEngine final : public QIconEngine
{
public:
    explicit PaletteSvgIconEngine(QByteArray source)
        : m_source(std::move(source))
    {
    }

    QIconEngine *clone() const override
    {
        return new PaletteSvgIconEngine(m_source);
    }

    void paint(QPainter *painter, const QRect &rect,
               QIcon::Mode mode, QIcon::State state) override
    {
        if (!painter || rect.isEmpty() || m_source.isEmpty())
            return;
        const QPalette palette = qApp
            ? qApp->palette() : QPalette{};
        const QPalette::ColorGroup group = mode == QIcon::Disabled
            ? QPalette::Disabled : QPalette::Active;
        const QPalette::ColorRole role = state == QIcon::On
            ? QPalette::HighlightedText : QPalette::ButtonText;
        const QColor color = palette.color(group, role);
        QByteArray source = m_source;
        source.replace("#000000", color.name(QColor::HexRgb).toUtf8());

        QSvgRenderer renderer(source);
        if (!renderer.isValid())
            return;
        const int extent = std::min(rect.width(), rect.height());
        const QRectF target(
            rect.center().x() - extent / 2.0,
            rect.center().y() - extent / 2.0,
            extent, extent);
        renderer.render(painter, target);
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                   QIcon::State state) override
    {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, result.rect(), mode, state);
        return result;
    }

    QSize actualSize(const QSize &size, QIcon::Mode,
                     QIcon::State) override
    {
        const int extent = std::min(size.width(), size.height());
        return QSize(extent, extent);
    }

    QString key() const override
    {
        return QStringLiteral("clearveil-palette-svg");
    }

private:
    QByteArray m_source;
};

QByteArray toolbarIconSource()
{
    static const QByteArray source = [] {
        QFile file(QStringLiteral(":/icons/toolbar-icons.svg"));
        if (!file.open(QIODevice::ReadOnly))
            return QByteArray{};
        return file.readAll();
    }();
    return source;
}

QByteArray standaloneIconSource(const QString &name)
{
    const QByteArray source = toolbarIconSource();
    const QByteArray opening = QByteArrayLiteral("<g id=\"")
        + name.toUtf8() + QByteArrayLiteral("\">");
    const qsizetype start = source.indexOf(opening);
    if (start < 0)
        return {};
    const qsizetype end = source.indexOf(
        QByteArrayLiteral("</g>"), start + opening.size());
    if (end < 0)
        return {};
    const QByteArray element = source.mid(
        start, end + qsizetype(4) - start);
    return QByteArrayLiteral(
               "<svg xmlns=\"http://www.w3.org/2000/svg\" "
               "viewBox=\"0 0 24 24\" fill=\"none\" "
               "stroke=\"#000000\" stroke-width=\"1.9\" "
               "stroke-linecap=\"round\" "
               "stroke-linejoin=\"round\">")
        + element + QByteArrayLiteral("</svg>");
}
}

QIcon ClearveilIcon::fromName(const QString &name)
{
    const QByteArray source = standaloneIconSource(name.trimmed());
    if (source.isEmpty() || name.trimmed().isEmpty())
        return {};
    return QIcon(new PaletteSvgIconEngine(source));
}
