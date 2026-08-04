#pragma once

#include <QList>
#include <QObject>
#include <QSize>
#include <QString>

class QAction;
class QLabel;
class QWidget;

class ViewerUiStateController final : public QObject
{
    Q_OBJECT

public:
    struct Actions {
        QAction *previous = nullptr;
        QAction *next = nullptr;
        QAction *slideshow = nullptr;
        QAction *saveAs = nullptr;
        QAction *copyImage = nullptr;
        QAction *trash = nullptr;
        QList<QAction *> fileActions;
        QAction *print = nullptr;
        QAction *wallpaper = nullptr;
        QAction *undo = nullptr;
        QAction *redo = nullptr;
        QList<QAction *> editActions;
        QList<QAction *> viewActions;
        QAction *exportFrame = nullptr;
        QAction *metadata = nullptr;
        QAction *colorPicker = nullptr;
        QAction *textSelection = nullptr;
        QAction *filmstripSource = nullptr;
    };

    struct Targets {
        QWidget *window = nullptr;
        QLabel *fileLabel = nullptr;
        QLabel *detailLabel = nullptr;
        QLabel *zoomLabel = nullptr;
    };

    struct Text {
        QString ready;
        QString applicationTitle;
        QString titledApplication;
        QString imageCount;
        QString clipboardImage;
        QString notSaved;
        QString imageDetails;
        QString lockedSuffix;
        QString modifiedTitle;
    };

    struct DocumentActionState {
        bool hasImage = false;
        bool loading = false;
        bool canEdit = false;
        bool canUndo = false;
        bool canRedo = false;
        bool fileExists = false;
        bool framesActive = false;
        bool directoryImagesAvailable = false;
        bool fullRasterTransferAvailable = true;
    };

    struct ImagePresentation {
        QString fileName;
        bool modified = false;
        QSize imageSize;
        bool fileExists = false;
        qint64 fileSize = 0;
        QString position;
        int count = 0;
        qreal zoom = 1.0;
        bool zoomLocked = false;
    };

    ViewerUiStateController(
        Actions actions, Targets targets, Text text,
        QObject *parent = nullptr);

    void applyDocumentActions(
        const DocumentActionState &state);
    void applyNavigationActions(int index, int count);
    void showReady();
    void showCollection(const QString &name, int imageCount);
    void showImage(const ImagePresentation &presentation);

    [[nodiscard]] static QString humanFileSize(qint64 bytes);

private:
    static void setLabelText(QLabel *label, const QString &text);
    static void setEnabled(QAction *action, bool enabled);
    static void setEnabled(const QList<QAction *> &actions,
                           bool enabled);

    Actions m_actions;
    Targets m_targets;
    Text m_text;
};
