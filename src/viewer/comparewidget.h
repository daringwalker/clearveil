#pragma once

#include <QWidget>

class QGridLayout;
class QLabel;

class CompareWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CompareWidget(QWidget *parent = nullptr);
    void setFiles(const QStringList &filePaths);

signals:
    void backRequested();

private:
    QGridLayout *m_grid = nullptr;
    QLabel *m_title = nullptr;
    QList<QWidget *> m_panes;
};
