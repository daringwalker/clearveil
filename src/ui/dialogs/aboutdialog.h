#pragma once

#include <QDialog>

class AboutDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

    [[nodiscard]] static QString projectUrl();
};
