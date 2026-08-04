#pragma once

#include <QHash>
#include <QImage>
#include <QSet>
#include <QWidget>

#include <array>

class QTreeWidget;
class QTreeWidgetItem;

class HistogramWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget *parent = nullptr);
    void setImage(const QImage &image);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::array<quint32, 256> m_red{};
    std::array<quint32, 256> m_green{};
    std::array<quint32, 256> m_blue{};
    quint32 m_peak = 0;
};

class MetadataPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MetadataPanel(QWidget *parent = nullptr);
    void setImage(const QString &filePath, const QImage &image,
                  const QSize &logicalSize = {});
    void clear();

private:
    void addRow(const QString &name, const QString &value,
                const QString &group = {}, const QString &sourceKey = {});

    QTreeWidget *m_tree = nullptr;
    HistogramWidget *m_histogram = nullptr;
    QSet<QString> m_seenRows;
    QHash<QString, QTreeWidgetItem *> m_groups;
};
