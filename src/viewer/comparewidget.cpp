#include "comparewidget.h"

#include "imagecanvas.h"

#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CompareWidget::CompareWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    auto *header = new QHBoxLayout;
    auto *back = new QPushButton(tr("Back to browser"), this);
    back->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    back->setAccessibleName(tr("Back to browser"));
    header->addWidget(back);
    m_title = new QLabel(this);
    QFont titleFont = m_title->font();
    titleFont.setWeight(QFont::DemiBold);
    m_title->setFont(titleFont);
    header->addWidget(m_title, 1);
    layout->addLayout(header);
    m_grid = new QGridLayout;
    m_grid->setSpacing(6);
    layout->addLayout(m_grid, 1);
    connect(back, &QPushButton::clicked, this, &CompareWidget::backRequested);
}

void CompareWidget::setFiles(const QStringList &filePaths)
{
    for (QWidget *pane : std::as_const(m_panes))
        pane->deleteLater();
    m_panes.clear();

    const QStringList files = filePaths.mid(0, 4);
    m_title->setText(tr("Comparing %1 images").arg(files.size()));
    for (int index = 0; index < files.size(); ++index) {
        const QString path = files.at(index);
        auto *pane = new QWidget(this);
        auto *paneLayout = new QVBoxLayout(pane);
        paneLayout->setContentsMargins(0, 0, 0, 0);
        paneLayout->setSpacing(2);
        auto *name = new QLabel(QFileInfo(path).fileName(), pane);
        name->setAlignment(Qt::AlignCenter);
        name->setToolTip(path);
        paneLayout->addWidget(name);
        auto *canvas = new ImageCanvas(pane);
        QImageReader reader(path);
        reader.setAutoTransform(true);
        canvas->setImage(reader.read());
        canvas->setAccessibleName(
            tr("Comparison image: %1").arg(QFileInfo(path).fileName()));
        paneLayout->addWidget(canvas, 1);
        m_grid->addWidget(pane, index / 2, index % 2);
        m_panes.append(pane);
    }
}
