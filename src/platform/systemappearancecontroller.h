#pragma once

#include <QDBusVariant>
#include <QObject>
#include <QString>
#include <QVariant>

class SystemAppearanceController final : public QObject
{
    Q_OBJECT

public:
    enum class ColorScheme {
        NoPreference = 0,
        PreferDark = 1,
        PreferLight = 2,
    };
    Q_ENUM(ColorScheme)

    explicit SystemAppearanceController(QObject *parent = nullptr);

    [[nodiscard]] ColorScheme colorScheme() const;
    [[nodiscard]] QString resolvedTheme(
        const QString &configuredTheme) const;

    [[nodiscard]] static ColorScheme colorSchemeFromPortalValue(
        const QVariant &value);
    [[nodiscard]] static QString resolveTheme(
        const QString &configuredTheme,
        ColorScheme colorScheme);

signals:
    void colorSchemeChanged(
        SystemAppearanceController::ColorScheme colorScheme);

private slots:
    void portalSettingChanged(
        const QString &nameSpace,
        const QString &key,
        const QDBusVariant &value);

private:
    void requestPortalColorScheme();
    void setColorScheme(ColorScheme colorScheme);

    ColorScheme m_colorScheme = ColorScheme::NoPreference;
};
