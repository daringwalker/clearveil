#pragma once

#include "actionregistry.h"
#include "interfacelayoutpage.h"

#include <QDialog>
#include <QImage>
#include <QList>
#include <QPair>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
class QSpinBox;
class QTableWidget;
class QTabWidget;

class CropPreviewWidget;

class CropDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CropDialog(const QImage &image, QWidget *parent = nullptr);
    [[nodiscard]] QRect cropRectangle() const;
    void setOperationText(const QString &windowTitle, const QString &actionText);

private:
    void syncEditors(const QRect &rectangle);
    void syncPreview();

    QImage m_image;
    CropPreviewWidget *m_preview = nullptr;
    QSpinBox *m_x = nullptr;
    QSpinBox *m_y = nullptr;
    QSpinBox *m_width = nullptr;
    QSpinBox *m_height = nullptr;
    QPushButton *m_actionButton = nullptr;
};

class ResizeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResizeDialog(const QSize &currentSize, QWidget *parent = nullptr);
    [[nodiscard]] QSize targetSize() const;

private:
    QSize m_originalSize;
    QSpinBox *m_width = nullptr;
    QSpinBox *m_height = nullptr;
    QCheckBox *m_keepAspect = nullptr;
    bool m_updating = false;
};

class AdjustDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdjustDialog(const QImage &image, QWidget *parent = nullptr);

    [[nodiscard]] int brightness() const;
    [[nodiscard]] int contrast() const;
    [[nodiscard]] qreal gamma() const;

private:
    void updatePreview();

    QImage m_previewSource;
    QLabel *m_preview = nullptr;
    QSlider *m_brightness = nullptr;
    QSlider *m_contrast = nullptr;
    QDoubleSpinBox *m_gamma = nullptr;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const QString &theme, const QString &language,
                            const QString &toolbarPosition,
                            const QString &filmstripPosition, int slideshowSeconds,
                            bool showFilmstrip,
                            bool showFilmstripFileNames,
                            int filmstripThumbnailExtent,
                            int filmstripVerticalColumns,
                            const QString &directoryThumbnailSortKey,
                            bool directoryThumbnailSortAscending,
                            bool randomSlideshow,
                            bool fullscreenSlideshow,
                            int imageMemoryCacheMiB,
                            bool persistentThumbnailCacheEnabled,
                            int persistentThumbnailCacheMiB,
                            qint64 persistentThumbnailCacheUsageBytes,
                            const QList<ActionRegistry::ToolbarItemDefinition> &toolbarItems,
                            const QStringList &toolbarLayout,
                            const QStringList &defaultToolbarLayout,
                            const QList<QPair<QString, QString>> &shortcutItems,
                            const QStringList &shortcutLayout,
                            const QStringList &defaultShortcutLayout,
                            const QString &wheelAction,
                            const QString &ctrlWheelAction,
                            const QString &doubleClickAction,
                            const QString &middleButtonAction,
                            const QString &backButtonAction,
                            const QString &forwardButtonAction,
                            const InterfaceLayoutState &interfaceLayout = {},
                            QWidget *parent = nullptr);

    [[nodiscard]] QString theme() const;
    [[nodiscard]] QString language() const;
    [[nodiscard]] InterfaceLayoutState interfaceLayout() const;
    void showInterfaceLayoutPage();
    [[nodiscard]] QString toolbarPosition() const;
    [[nodiscard]] QString filmstripPosition() const;
    [[nodiscard]] int slideshowSeconds() const;
    [[nodiscard]] bool showFilmstrip() const;
    [[nodiscard]] bool showFilmstripFileNames() const;
    [[nodiscard]] int filmstripThumbnailExtent() const;
    [[nodiscard]] int filmstripVerticalColumns() const;
    [[nodiscard]] QString directoryThumbnailSortKey() const;
    [[nodiscard]] bool directoryThumbnailSortAscending() const;
    [[nodiscard]] bool randomSlideshow() const;
    [[nodiscard]] bool fullscreenSlideshow() const;
    [[nodiscard]] int imageMemoryCacheMiB() const;
    [[nodiscard]] bool persistentThumbnailCacheEnabled() const;
    [[nodiscard]] int persistentThumbnailCacheMiB() const;
    [[nodiscard]] QStringList toolbarLayout() const;
    [[nodiscard]] QStringList shortcutLayout() const;
    [[nodiscard]] QString wheelAction() const;
    [[nodiscard]] QString ctrlWheelAction() const;
    [[nodiscard]] QString doubleClickAction() const;
    [[nodiscard]] QString middleButtonAction() const;
    [[nodiscard]] QString backButtonAction() const;
    [[nodiscard]] QString forwardButtonAction() const;

signals:
    void applyRequested();

private:
    void populateToolbarItems(const QStringList &layout);
    void moveToolbarItem(int offset);
    void populateShortcutItems(const QStringList &layout);
    [[nodiscard]] bool validateShortcutConflicts();

    QComboBox *m_theme = nullptr;
    QComboBox *m_language = nullptr;
    QTabWidget *m_tabs = nullptr;
    InterfaceLayoutPage *m_interfaceLayoutPage = nullptr;
    QSpinBox *m_slideshowSeconds = nullptr;
    QCheckBox *m_showFilmstripFileNames = nullptr;
    QSpinBox *m_filmstripThumbnailExtent = nullptr;
    QSpinBox *m_filmstripVerticalColumns = nullptr;
    QComboBox *m_directoryThumbnailSortKey = nullptr;
    QComboBox *m_directoryThumbnailSortDirection = nullptr;
    QCheckBox *m_randomSlideshow = nullptr;
    QCheckBox *m_fullscreenSlideshow = nullptr;
    QSpinBox *m_imageMemoryCacheMiB = nullptr;
    QCheckBox *m_persistentThumbnailCacheEnabled = nullptr;
    QSpinBox *m_persistentThumbnailCacheMiB = nullptr;
    QLabel *m_persistentThumbnailCacheUsage = nullptr;
    QListWidget *m_toolbarItems = nullptr;
    QList<ActionRegistry::ToolbarItemDefinition> m_toolbarItemDefinitions;
    QStringList m_defaultToolbarLayout;
    QTableWidget *m_shortcutItems = nullptr;
    QList<QPair<QString, QString>> m_shortcutItemDefinitions;
    QStringList m_defaultShortcutLayout;
    QComboBox *m_wheelAction = nullptr;
    QComboBox *m_ctrlWheelAction = nullptr;
    QComboBox *m_doubleClickAction = nullptr;
    QComboBox *m_middleButtonAction = nullptr;
    QComboBox *m_backButtonAction = nullptr;
    QComboBox *m_forwardButtonAction = nullptr;
};
