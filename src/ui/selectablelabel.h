#pragma once

#include <QLabel>

class QKeyEvent;

class SelectableLabel final : public QLabel
{
    Q_OBJECT

public:
    explicit SelectableLabel(QWidget *parent = nullptr);
    explicit SelectableLabel(const QString &text,
                             QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void initializeTextInteraction();
};
