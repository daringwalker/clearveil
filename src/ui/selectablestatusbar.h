#pragma once

#include <QPointer>
#include <QStatusBar>

class SelectableLabel;
class QTimer;

class SelectableStatusBar final : public QStatusBar
{
    Q_OBJECT

public:
    explicit SelectableStatusBar(QWidget *parent = nullptr);

    void setPrimaryWidget(QWidget *widget);
    [[nodiscard]] QString currentMessage() const;
    [[nodiscard]] SelectableLabel *messageLabel() const;

public slots:
    void showMessage(const QString &message, int timeout = 0);
    void clearMessage();

private:
    QPointer<QWidget> m_primaryWidget;
    SelectableLabel *m_messageLabel = nullptr;
    QTimer *m_messageTimer = nullptr;
    QString m_currentMessage;
};
