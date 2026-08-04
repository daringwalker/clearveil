#include "systemappearancecontroller.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

namespace {
constexpr auto portalService = "org.freedesktop.portal.Desktop";
constexpr auto portalPath = "/org/freedesktop/portal/desktop";
constexpr auto settingsInterface = "org.freedesktop.portal.Settings";
constexpr auto appearanceNamespace = "org.freedesktop.appearance";
constexpr auto colorSchemeKey = "color-scheme";
}

SystemAppearanceController::SystemAppearanceController(QObject *parent)
    : QObject(parent)
{
    QDBusConnection::sessionBus().connect(
        QString::fromLatin1(portalService),
        QString::fromLatin1(portalPath),
        QString::fromLatin1(settingsInterface),
        QStringLiteral("SettingChanged"),
        this,
        SLOT(portalSettingChanged(QString,QString,QDBusVariant)));
    requestPortalColorScheme();
}

SystemAppearanceController::ColorScheme
SystemAppearanceController::colorScheme() const
{
    return m_colorScheme;
}

QString SystemAppearanceController::resolvedTheme(
    const QString &configuredTheme) const
{
    return resolveTheme(configuredTheme, m_colorScheme);
}

SystemAppearanceController::ColorScheme
SystemAppearanceController::colorSchemeFromPortalValue(
    const QVariant &value)
{
    QVariant unwrapped = value;
    if (unwrapped.metaType().id()
        == qMetaTypeId<QDBusVariant>()) {
        unwrapped = unwrapped.value<QDBusVariant>().variant();
    }

    bool ok = false;
    const uint rawValue = unwrapped.toUInt(&ok);
    if (!ok)
        return ColorScheme::NoPreference;
    if (rawValue == static_cast<uint>(ColorScheme::PreferDark))
        return ColorScheme::PreferDark;
    if (rawValue == static_cast<uint>(ColorScheme::PreferLight))
        return ColorScheme::PreferLight;
    return ColorScheme::NoPreference;
}

QString SystemAppearanceController::resolveTheme(
    const QString &configuredTheme,
    ColorScheme)
{
    return configuredTheme;
}

void SystemAppearanceController::portalSettingChanged(
    const QString &nameSpace,
    const QString &key,
    const QDBusVariant &value)
{
    if (nameSpace != QString::fromLatin1(appearanceNamespace)
        || key != QString::fromLatin1(colorSchemeKey)) {
        return;
    }
    setColorScheme(colorSchemeFromPortalValue(value.variant()));
}

void SystemAppearanceController::requestPortalColorScheme()
{
    QDBusInterface settings(
        QString::fromLatin1(portalService),
        QString::fromLatin1(portalPath),
        QString::fromLatin1(settingsInterface),
        QDBusConnection::sessionBus());
    if (!settings.isValid())
        return;

    auto *watcher = new QDBusPendingCallWatcher(
        settings.asyncCall(
            QStringLiteral("ReadOne"),
            QString::fromLatin1(appearanceNamespace),
            QString::fromLatin1(colorSchemeKey)),
        this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, [this, watcher] {
        const QDBusPendingReply<QDBusVariant> reply = *watcher;
        if (!reply.isError()) {
            setColorScheme(colorSchemeFromPortalValue(
                reply.value().variant()));
        }
        watcher->deleteLater();
    });
}

void SystemAppearanceController::setColorScheme(
    ColorScheme colorScheme)
{
    if (m_colorScheme == colorScheme)
        return;
    m_colorScheme = colorScheme;
    emit colorSchemeChanged(m_colorScheme);
}
