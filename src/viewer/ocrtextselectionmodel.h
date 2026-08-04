// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ocrresult.h"

#include <QObject>
#include <QPointF>
#include <QVector>

class OcrTextSelectionModel final : public QObject
{
    Q_OBJECT

public:
    explicit OcrTextSelectionModel(QObject *parent = nullptr);

    void setResult(const OcrResult &result);
    void clear();
    [[nodiscard]] bool hasText() const;
    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] QString selectedText() const;
    [[nodiscard]] int hitTest(const QPointF &imagePosition) const;
    [[nodiscard]] QVector<QRectF> selectedBounds() const;
    [[nodiscard]] const QVector<OcrSymbol> &symbols() const;
    [[nodiscard]] bool isSymbolSelected(int index) const;

    bool beginSelection(const QPointF &imagePosition);
    bool updateSelection(const QPointF &imagePosition);
    bool selectWordAt(const QPointF &imagePosition);
    void selectAll();
    void clearSelection();

signals:
    void selectionChanged(bool hasSelection);

private:
    void setSelection(int anchor, int current);
    [[nodiscard]] int symbolAtPosition(
        const QPointF &imagePosition,
        bool allowNearestLine) const;
    [[nodiscard]] int firstSelectedIndex() const;
    [[nodiscard]] int lastSelectedIndex() const;

    OcrResult m_result;
    int m_anchor = -1;
    int m_current = -1;
};
