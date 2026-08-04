#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>

class QListView;
class ThumbnailModel;

class FilmstripController final : public QObject
{
    Q_OBJECT

public:
    enum class Source {
        OpenedImages,
        CurrentDirectory
    };
    Q_ENUM(Source)

    FilmstripController(QListView *view,
                        ThumbnailModel *openedModel,
                        ThumbnailModel *directoryModel,
                        QObject *parent = nullptr);

    [[nodiscard]] Source source() const;
    void setSource(Source source);
    [[nodiscard]] ThumbnailModel *model() const;
    [[nodiscard]] int count() const;
    [[nodiscard]] int currentRow() const;
    [[nodiscard]] QString pathAt(int row) const;
    [[nodiscard]] int rowForPath(const QString &path) const;
    void selectRow(int row, bool ensureVisible = true);
    void syncSelection(const QString &path);

signals:
    void activationRequested(FilmstripController::Source source,
                             int row, const QString &path);
    void sourceChanged(FilmstripController::Source source);

private:
    void connectSelection();
    [[nodiscard]] int currentScrollValue() const;
    void restoreScrollValue(Source source, int value);

    QPointer<QListView> m_view;
    QPointer<ThumbnailModel> m_openedModel;
    QPointer<ThumbnailModel> m_directoryModel;
    QMetaObject::Connection m_selectionConnection;
    Source m_source = Source::OpenedImages;
    int m_openedScrollValue = 0;
    int m_directoryScrollValue = 0;
};
