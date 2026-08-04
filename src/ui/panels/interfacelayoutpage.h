#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QComboBox;

struct InterfaceLayoutState
{
    bool showMenuBar = false;
    bool showToolbar = true;
    bool showStatusBar = true;
    bool layoutLocked = true;
    bool showToolbarInFullscreen = false;
    bool showThumbnailsInFullscreen = false;
    bool showStatusBarInFullscreen = false;
    bool showInformationInFullscreen = false;

    QString toolbarPosition = QStringLiteral("top");

    bool showThumbnails = true;
    QString thumbnailsPlacement = QStringLiteral("bottom");
    QString floatingThumbnailLayout = QStringLiteral("auto");

    bool showInformation = false;
    QString informationPlacement = QStringLiteral("right");

    bool showColorPicker = false;
    QString colorPickerPlacement = QStringLiteral("overlay");
    QStringList panelOrder{
        QStringLiteral("thumbnails"),
        QStringLiteral("information"),
        QStringLiteral("colorPicker")};
};

class InterfaceLayoutPage final : public QWidget
{
    Q_OBJECT

public:
    explicit InterfaceLayoutPage(
        const InterfaceLayoutState &state,
        QWidget *parent = nullptr);

    [[nodiscard]] InterfaceLayoutState state() const;
    void setState(const InterfaceLayoutState &state);

private:
    void updatePreview();
    void updateControlAvailability();
    void applyPreviewDrop(const QString &panelId,
                          const QString &placement,
                          int placementIndex);

    QCheckBox *m_showMenuBar = nullptr;
    QCheckBox *m_showToolbar = nullptr;
    QCheckBox *m_showStatusBar = nullptr;
    QCheckBox *m_layoutLocked = nullptr;
    QCheckBox *m_showToolbarInFullscreen = nullptr;
    QCheckBox *m_showThumbnailsInFullscreen = nullptr;
    QCheckBox *m_showStatusBarInFullscreen = nullptr;
    QCheckBox *m_showInformationInFullscreen = nullptr;
    QComboBox *m_toolbarPosition = nullptr;

    QCheckBox *m_showThumbnails = nullptr;
    QComboBox *m_thumbnailsPlacement = nullptr;
    QComboBox *m_floatingThumbnailLayout = nullptr;
    QCheckBox *m_showInformation = nullptr;
    QComboBox *m_informationPlacement = nullptr;
    QCheckBox *m_showColorPicker = nullptr;
    QComboBox *m_colorPickerPlacement = nullptr;
    QStringList m_panelOrder;
    QWidget *m_preview = nullptr;
};
