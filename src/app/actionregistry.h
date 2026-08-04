#pragma once

#include <QHash>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QStringList>

class QAction;

class ActionRegistry final : public QObject
{
    Q_OBJECT

public:
    struct ToolbarItemDefinition {
        QString id;
        QString label;
        QIcon icon;
    };

    explicit ActionRegistry(QObject *parent = nullptr);

    void addAction(const QString &id, QAction *action,
                   const QKeySequence &defaultShortcut);
    void addToolbarItem(const QString &id, bool defaultEnabled,
                        const QString &fallbackLabel = {});

    [[nodiscard]] QAction *action(const QString &id) const;
    [[nodiscard]] QList<ToolbarItemDefinition>
        toolbarItemDefinitions() const;
    [[nodiscard]] QList<QPair<QString, QString>>
        shortcutItemDefinitions() const;
    [[nodiscard]] QStringList defaultToolbarLayout() const;
    [[nodiscard]] QStringList defaultShortcutLayout() const;
    [[nodiscard]] QStringList normalizedToolbarLayout(
        const QStringList &layout) const;
    [[nodiscard]] QStringList normalizedShortcutLayout(
        const QStringList &layout) const;

    [[nodiscard]] static QString shortcutEntry(
        const QString &id, const QKeySequence &shortcut);
    [[nodiscard]] static QString shortcutEntryId(
        const QString &encoded);
    [[nodiscard]] static QKeySequence shortcutEntrySequence(
        const QString &encoded);

private:
    struct ShortcutItem {
        QString id;
        QKeySequence defaultShortcut;
    };
    struct ToolbarItem {
        QString id;
        QString fallbackLabel;
        bool defaultEnabled = false;
    };

    QHash<QString, QPointer<QAction>> m_actions;
    QList<ShortcutItem> m_shortcutItems;
    QList<ToolbarItem> m_toolbarItems;
};
