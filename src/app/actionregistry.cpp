#include "actionregistry.h"

#include <QAction>
#include <QSet>

ActionRegistry::ActionRegistry(QObject *parent)
    : QObject(parent)
{
}

void ActionRegistry::addAction(
    const QString &id, QAction *action,
    const QKeySequence &defaultShortcut)
{
    if (id.isEmpty() || !action || m_actions.contains(id))
        return;
    m_actions.insert(id, action);
    m_shortcutItems.append({id, defaultShortcut});
}

void ActionRegistry::addToolbarItem(
    const QString &id, bool defaultEnabled,
    const QString &fallbackLabel)
{
    if (id.isEmpty())
        return;
    for (const ToolbarItem &item : m_toolbarItems) {
        if (item.id == id)
            return;
    }
    if (!m_actions.contains(id) && fallbackLabel.isEmpty())
        return;
    m_toolbarItems.append({id, fallbackLabel, defaultEnabled});
}

QAction *ActionRegistry::action(const QString &id) const
{
    return m_actions.value(id);
}

QList<ActionRegistry::ToolbarItemDefinition>
ActionRegistry::toolbarItemDefinitions() const
{
    QList<ToolbarItemDefinition> result;
    result.reserve(m_toolbarItems.size());
    for (const ToolbarItem &item : m_toolbarItems) {
        QAction *registeredAction = action(item.id);
        result.append(ToolbarItemDefinition{
            item.id,
            registeredAction
                ? registeredAction->text() : item.fallbackLabel,
            registeredAction ? registeredAction->icon() : QIcon{}
        });
    }
    return result;
}

QList<QPair<QString, QString>>
ActionRegistry::shortcutItemDefinitions() const
{
    QList<QPair<QString, QString>> result;
    result.reserve(m_shortcutItems.size());
    for (const ShortcutItem &item : m_shortcutItems) {
        if (QAction *registeredAction = action(item.id))
            result.append({item.id, registeredAction->text()});
    }
    return result;
}

QStringList ActionRegistry::defaultToolbarLayout() const
{
    QStringList result;
    result.reserve(m_toolbarItems.size());
    for (const ToolbarItem &item : m_toolbarItems) {
        result.append(item.defaultEnabled
            ? item.id : QStringLiteral("!") + item.id);
    }
    return result;
}

QStringList ActionRegistry::defaultShortcutLayout() const
{
    QStringList result;
    result.reserve(m_shortcutItems.size());
    for (const ShortcutItem &item : m_shortcutItems)
        result.append(shortcutEntry(item.id, item.defaultShortcut));
    return result;
}

QStringList ActionRegistry::normalizedToolbarLayout(
    const QStringList &layout) const
{
    QSet<QString> knownIds;
    for (const ToolbarItem &item : m_toolbarItems)
        knownIds.insert(item.id);

    QStringList result;
    QSet<QString> seen;
    for (const QString &encoded : layout) {
        const bool enabled = !encoded.startsWith(QLatin1Char('!'));
        const QString id = enabled ? encoded : encoded.mid(1);
        if (!knownIds.contains(id) || seen.contains(id))
            continue;
        result.append(enabled ? id : QStringLiteral("!") + id);
        seen.insert(id);
    }
    for (const ToolbarItem &item : m_toolbarItems) {
        if (!seen.contains(item.id))
            result.append(QStringLiteral("!") + item.id);
    }
    return result;
}

QStringList ActionRegistry::normalizedShortcutLayout(
    const QStringList &layout) const
{
    QSet<QString> knownIds;
    for (const ShortcutItem &item : m_shortcutItems)
        knownIds.insert(item.id);

    QStringList result;
    QSet<QString> seen;
    for (const QString &encoded : layout) {
        const QString id = shortcutEntryId(encoded);
        if (!knownIds.contains(id) || seen.contains(id))
            continue;
        result.append(shortcutEntry(
            id, shortcutEntrySequence(encoded)));
        seen.insert(id);
    }
    for (const ShortcutItem &item : m_shortcutItems) {
        if (!seen.contains(item.id)) {
            result.append(shortcutEntry(
                item.id, item.defaultShortcut));
        }
    }
    return result;
}

QString ActionRegistry::shortcutEntry(
    const QString &id, const QKeySequence &shortcut)
{
    return id + QLatin1Char('\t')
        + shortcut.toString(QKeySequence::PortableText);
}

QString ActionRegistry::shortcutEntryId(const QString &encoded)
{
    const int separator = encoded.indexOf(QLatin1Char('\t'));
    return separator < 0 ? encoded : encoded.left(separator);
}

QKeySequence ActionRegistry::shortcutEntrySequence(
    const QString &encoded)
{
    const int separator = encoded.indexOf(QLatin1Char('\t'));
    if (separator < 0)
        return {};
    return QKeySequence::fromString(
        encoded.mid(separator + 1),
        QKeySequence::PortableText);
}
