#pragma once

#include "imagesource.h"

#include <QCoreApplication>
#include <QMutex>

struct _VipsImage;
using VipsImage = struct _VipsImage;

class VipsImageSource final : public ImageSource
{
    Q_DECLARE_TR_FUNCTIONS(VipsImageSource)

public:
    ~VipsImageSource() override;

    [[nodiscard]] static bool isAvailable();
    [[nodiscard]] static bool supportsFile(const QString &filePath);
    [[nodiscard]] static std::shared_ptr<VipsImageSource> open(
        const QString &filePath, int previewMaximumDimension,
        std::stop_token stopToken = {},
        QString *errorMessage = nullptr);

    [[nodiscard]] QSize logicalSize() const override;
    [[nodiscard]] QImage preview() const override;
    [[nodiscard]] QString filePath() const override;
    [[nodiscard]] bool isRegionBacked() const override;
    void releaseTransientResources() override;
    [[nodiscard]] QImage readRegion(
        const QRect &sourceRect, const QSize &outputSize,
        std::stop_token stopToken = {},
        QString *errorMessage = nullptr) override;
    [[nodiscard]] bool writeToFile(
        const QString &targetPath,
        std::stop_token stopToken = {},
        QString *errorMessage = nullptr) override;

private:
    VipsImageSource(QString filePath, QSize logicalSize,
                    QImage previewImage);

    [[nodiscard]] bool ensureRandomImageLocked(
        QString *errorMessage);

    QString m_filePath;
    QSize m_logicalSize;
    QImage m_preview;
    QMutex m_mutex;
    VipsImage *m_randomImage = nullptr;
};
