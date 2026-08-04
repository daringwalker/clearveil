#include "displaycolor.h"

#include <QElapsedTimer>
#include <QFile>

#ifdef CLEARVEIL_HAVE_COLORD
#pragma push_macro("signals")
#undef signals
#include <colord.h>
#pragma pop_macro("signals")
#endif

namespace {
DisplayColorTarget srgbTarget(const QString &outputName,
                              DisplayColorTargetSource source)
{
    DisplayColorTarget target;
    target.colorSpace = QColorSpace(QColorSpace::SRgb);
    target.source = source;
    target.outputName = outputName;
    return target;
}

#ifdef CLEARVEIL_HAVE_COLORD
DisplayColorTarget colordTarget(const QString &outputName)
{
    DisplayColorTarget fallback = srgbTarget(
        outputName, DisplayColorTargetSource::SrgbFallback);
    GError *error = nullptr;
    CdClient *client = cd_client_new();
    if (!cd_client_connect_sync(client, nullptr, &error)) {
        g_clear_error(&error);
        g_object_unref(client);
        return fallback;
    }

    const QByteArray outputUtf8 = outputName.toUtf8();
    CdDevice *device = cd_client_find_device_by_property_sync(
        client, CD_DEVICE_METADATA_XRANDR_NAME,
        outputUtf8.constData(), nullptr, &error);
    if (!device) {
        g_clear_error(&error);
        g_object_unref(client);
        return fallback;
    }
    if (!cd_device_connect_sync(device, nullptr, &error)) {
        g_clear_error(&error);
        g_object_unref(device);
        g_object_unref(client);
        return fallback;
    }

    CdProfile *borrowedProfile =
        cd_device_get_default_profile(device);
    if (!borrowedProfile) {
        g_object_unref(device);
        g_object_unref(client);
        return fallback;
    }
    CdProfile *profile = CD_PROFILE(g_object_ref(borrowedProfile));
    if (!cd_profile_get_connected(profile)
        && !cd_profile_connect_sync(profile, nullptr, &error)) {
        g_clear_error(&error);
        g_object_unref(profile);
        g_object_unref(device);
        g_object_unref(client);
        return fallback;
    }

    const gchar *filename = cd_profile_get_filename(profile);
    const QString profilePath = filename
        ? QString::fromUtf8(filename) : QString();
    QFile profileFile(profilePath);
    DisplayColorTarget target = fallback;
    if (profileFile.open(QIODevice::ReadOnly)) {
        const QByteArray profileData = profileFile.readAll();
        const QColorSpace colorSpace =
            QColorSpace::fromIccProfile(profileData);
        if (colorSpace.isValid()) {
            target.colorSpace = colorSpace;
            target.source = DisplayColorTargetSource::ColordProfile;
            target.profilePath = profilePath;
        }
    }

    g_object_unref(profile);
    g_object_unref(device);
    g_object_unref(client);
    return target;
}
#endif
}

DisplayColorTarget DisplayColor::resolveAutomaticTarget(
    const QString &outputName, const QString &platformName)
{
    // Wayland compositors treat ordinary application surfaces as sRGB and
    // perform the output conversion. Pre-converting to the monitor profile
    // here would apply the transform twice once compositor management is on.
    if (platformName.contains(QStringLiteral("wayland"),
                              Qt::CaseInsensitive)) {
        return srgbTarget(
            outputName, DisplayColorTargetSource::CompositorSrgb);
    }

#ifdef CLEARVEIL_HAVE_COLORD
    if (platformName == QStringLiteral("xcb"))
        return colordTarget(outputName);
#endif
    return srgbTarget(
        outputName, DisplayColorTargetSource::SrgbFallback);
}

DisplayColorTransformResult DisplayColor::transform(
    const QImage &source, const QColorSpace &targetColorSpace,
    std::stop_token stopToken)
{
    QElapsedTimer elapsed;
    elapsed.start();
    DisplayColorTransformResult result;
    const auto finish = [&result, &elapsed](
                            DisplayColorTransformError error) {
        result.error = error;
        result.elapsedNanoseconds = elapsed.nsecsElapsed();
        return result;
    };
    if (source.isNull())
        return finish(DisplayColorTransformError::EmptyImage);
    if (stopToken.stop_requested())
        return finish(DisplayColorTransformError::Cancelled);

    const QColorSpace target = targetColorSpace.isValid()
        ? targetColorSpace : QColorSpace(QColorSpace::SRgb);
    // Untagged pixels are already interpreted as sRGB by the viewer and the
    // Wayland compositor. Attaching metadata would detach the implicitly
    // shared QImage and duplicate hundreds of MiB for a large image.
    if (!source.colorSpace().isValid()
        && target == QColorSpace(QColorSpace::SRgb)) {
        result.image = source;
        result.assumedSrgb = true;
        return finish(DisplayColorTransformError::None);
    }
    QImage prepared = source;
    if (!prepared.colorSpace().isValid()) {
        prepared.setColorSpace(QColorSpace(QColorSpace::SRgb));
        result.assumedSrgb = true;
    }
    if (prepared.colorSpace() == target) {
        result.image = prepared;
        return finish(DisplayColorTransformError::None);
    }

    if (stopToken.stop_requested())
        return finish(DisplayColorTransformError::Cancelled);
    QImage converted = prepared.convertedToColorSpace(target);
    if (stopToken.stop_requested())
        return finish(DisplayColorTransformError::Cancelled);
    if (converted.isNull())
        return finish(DisplayColorTransformError::ConversionFailed);
    result.image = std::move(converted);
    result.converted = true;
    return finish(DisplayColorTransformError::None);
}
