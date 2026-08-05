#include "selectablestatusbar.h"

#include "selectablelabel.h"

#include <QTimer>

SelectableStatusBar::SelectableStatusBar(QWidget *parent)
    : QStatusBar(parent),
      m_messageLabel(new SelectableLabel(this)),
      m_messageTimer(new QTimer(this))
{
    m_messageLabel->setObjectName(QStringLiteral("statusMessageLabel"));
    m_messageLabel->hide();
    addWidget(m_messageLabel, 1);

    m_messageTimer->setSingleShot(true);
    connect(m_messageTimer, &QTimer::timeout,
            this, &SelectableStatusBar::clearMessage);
}

void SelectableStatusBar::setPrimaryWidget(QWidget *widget)
{
    m_primaryWidget = widget;
    if (m_primaryWidget)
        m_primaryWidget->setVisible(m_currentMessage.isEmpty());
}

QString SelectableStatusBar::currentMessage() const
{
    return m_currentMessage;
}

SelectableLabel *SelectableStatusBar::messageLabel() const
{
    return m_messageLabel;
}

void SelectableStatusBar::showMessage(const QString &message, int timeout)
{
    if (message.isEmpty()) {
        clearMessage();
        return;
    }

    const bool changed = m_currentMessage != message;
    m_messageTimer->stop();
    m_currentMessage = message;
    m_messageLabel->setText(message);
    m_messageLabel->show();
    if (m_primaryWidget)
        m_primaryWidget->hide();
    if (timeout > 0)
        m_messageTimer->start(timeout);
    if (changed)
        emit messageChanged(message);
}

void SelectableStatusBar::clearMessage()
{
    m_messageTimer->stop();
    const bool changed = !m_currentMessage.isEmpty();
    m_currentMessage.clear();
    m_messageLabel->clear();
    m_messageLabel->hide();
    if (m_primaryWidget)
        m_primaryWidget->show();
    if (changed)
        emit messageChanged({});
}
