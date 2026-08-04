// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrtextselectionmodel.h"

#include <algorithm>
#include <limits>

namespace {
bool isCjkText(const QString &text)
{
    for (const QChar character : text) {
        switch (character.script()) {
        case QChar::Script_Han:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
        case QChar::Script_Hangul:
            return true;
        default:
            break;
        }
    }
    return false;
}

bool hasVisibleWordGap(
    const OcrSymbol &previous, const OcrSymbol &current)
{
    const qreal gap = current.bounds.left()
        - previous.bounds.right();
    const qreal characterHeight = std::min(
        previous.bounds.height(), current.bounds.height());
    const qreal relativeThreshold =
        isCjkText(previous.text) && isCjkText(current.text)
        ? 0.9 : 0.18;
    return gap > std::max<qreal>(1.0,
                                 characterHeight * relativeThreshold);
}

void normalizeWordSymbolBounds(QVector<OcrSymbol> &symbols)
{
    for (int first = 0; first < symbols.size();) {
        int last = first;
        while (last + 1 < symbols.size()
               && symbols.at(last + 1).lineIndex
                   == symbols.at(first).lineIndex
               && symbols.at(last + 1).wordIndex
                   == symbols.at(first).wordIndex) {
            ++last;
        }
        const int count = last - first + 1;
        if (count < 2) {
            first = last + 1;
            continue;
        }

        QRectF wordBounds;
        int textUnits = 0;
        for (int index = first; index <= last; ++index) {
            const QRectF bounds = symbols.at(index).bounds.normalized();
            wordBounds = wordBounds.isNull()
                ? bounds : wordBounds.united(bounds);
            textUnits += std::max(
                1, static_cast<int>(symbols.at(index).text.size()));
        }
        const bool leftToRight = symbols.at(first).bounds.left()
            <= symbols.at(last).bounds.left();
        int precedingUnits = 0;
        for (int offset = 0; offset < count; ++offset) {
            const int symbolUnits = std::max(
                1, static_cast<int>(
                    symbols.at(first + offset).text.size()));
            const qreal firstRatio = static_cast<qreal>(precedingUnits)
                / textUnits;
            const qreal lastRatio = static_cast<qreal>(
                precedingUnits + symbolUnits) / textUnits;
            QRectF normalized = symbols.at(first + offset).bounds;
            if (leftToRight) {
                normalized.setLeft(
                    wordBounds.left() + wordBounds.width() * firstRatio);
                normalized.setRight(
                    wordBounds.left() + wordBounds.width() * lastRatio);
            } else {
                normalized.setRight(
                    wordBounds.right() - wordBounds.width() * firstRatio);
                normalized.setLeft(
                    wordBounds.right() - wordBounds.width() * lastRatio);
            }
            symbols[first + offset].bounds = normalized;
            precedingUnits += symbolUnits;
        }
        first = last + 1;
    }
}

void normalizeAdjacentSymbolBounds(QVector<OcrSymbol> &symbols)
{
    // Tesseract can assign every Han character a different word index and
    // still return overlapping (or implausibly narrow) symbol rectangles.
    // Word-level normalization cannot repair that case. Partition visually
    // adjacent CJK symbols at the midpoint between their original centres so
    // every character has an independent, non-overlapping hit cell.
    for (int first = 0; first < symbols.size();) {
        int last = first;
        while (last + 1 < symbols.size()
               && symbols.at(last + 1).lineIndex
                   == symbols.at(first).lineIndex) {
            ++last;
        }

        QVector<qreal> centres;
        centres.reserve(last - first + 1);
        for (int index = first; index <= last; ++index)
            centres.append(symbols.at(index).bounds.center().x());

        for (int index = first; index < last; ++index) {
            QRectF current = symbols.at(index).bounds.normalized();
            QRectF next = symbols.at(index + 1).bounds.normalized();
            const qreal currentCentre = centres.at(index - first);
            const qreal nextCentre = centres.at(index + 1 - first);
            if (nextCentre <= currentCentre)
                continue;

            const bool cjkPair = isCjkText(symbols.at(index).text)
                && isCjkText(symbols.at(index + 1).text);
            const qreal characterHeight = std::min(
                current.height(), next.height());
            const bool visuallyAdjacent = cjkPair
                && nextCentre - currentCentre
                    <= std::max<qreal>(18.0,
                                       characterHeight * 1.4);
            const bool overlaps = current.right() > next.left();
            if (!visuallyAdjacent && !overlaps)
                continue;

            const qreal boundary = (currentCentre + nextCentre) / 2.0;
            if (boundary <= current.left() + 0.5
                || boundary >= next.right() - 0.5) {
                continue;
            }
            current.setRight(boundary);
            next.setLeft(boundary);
            symbols[index].bounds = current;
            symbols[index + 1].bounds = next;
        }
        first = last + 1;
    }
}

qreal distanceToInterval(qreal value, qreal start, qreal end)
{
    if (value < start)
        return start - value;
    if (value > end)
        return value - end;
    return 0.0;
}
}

OcrTextSelectionModel::OcrTextSelectionModel(QObject *parent)
    : QObject(parent)
{
}

void OcrTextSelectionModel::setResult(const OcrResult &result)
{
    const bool hadSelection = hasSelection();
    m_result = result;
    // Tesseract occasionally reports overlapping or even word-wide boxes for
    // individual symbols. Build stable, non-overlapping character cells from
    // each word box so the painted selection and copied text share exactly the
    // same boundaries.
    normalizeWordSymbolBounds(m_result.symbols);
    normalizeAdjacentSymbolBounds(m_result.symbols);
    // Character boxes from OCR engines normally follow the ink itself. That
    // makes adjacent characters in one visual row look like a staircase when
    // highlighted. Normalize only their vertical geometry to the text line;
    // horizontal geometry remains character-accurate for caret hit testing.
    for (int first = 0; first < m_result.symbols.size();) {
        int last = first;
        qreal top = m_result.symbols.at(first).bounds.top();
        qreal bottom = m_result.symbols.at(first).bounds.bottom();
        while (last + 1 < m_result.symbols.size()
               && m_result.symbols.at(last + 1).lineIndex
                   == m_result.symbols.at(first).lineIndex) {
            ++last;
            top = std::min(top,
                           m_result.symbols.at(last).bounds.top());
            bottom = std::max(bottom,
                              m_result.symbols.at(last).bounds.bottom());
        }
        for (int index = first; index <= last; ++index) {
            QRectF bounds = m_result.symbols.at(index).bounds;
            bounds.setTop(top);
            bounds.setBottom(bottom);
            m_result.symbols[index].bounds = bounds;
        }
        first = last + 1;
    }
    m_anchor = -1;
    m_current = -1;
    if (hadSelection)
        emit selectionChanged(false);
}

void OcrTextSelectionModel::clear()
{
    setResult({});
}

bool OcrTextSelectionModel::hasText() const
{
    return !m_result.symbols.isEmpty();
}

bool OcrTextSelectionModel::hasSelection() const
{
    return m_anchor >= 0 && m_current >= 0
        && m_anchor < m_result.symbols.size()
        && m_current < m_result.symbols.size();
}

int OcrTextSelectionModel::firstSelectedIndex() const
{
    return hasSelection() ? std::min(m_anchor, m_current) : -1;
}

int OcrTextSelectionModel::lastSelectedIndex() const
{
    return hasSelection() ? std::max(m_anchor, m_current) : -1;
}

QString OcrTextSelectionModel::selectedText() const
{
    if (!hasSelection())
        return {};
    QString result;
    const int first = firstSelectedIndex();
    const int last = lastSelectedIndex();
    for (int index = first; index <= last; ++index) {
        const OcrSymbol &symbol = m_result.symbols.at(index);
        if (index > first) {
            const OcrSymbol &previous = m_result.symbols.at(index - 1);
            if (symbol.lineIndex != previous.lineIndex)
                result.append(QLatin1Char('\n'));
            else if (symbol.wordIndex != previous.wordIndex
                     && hasVisibleWordGap(previous, symbol))
                result.append(QLatin1Char(' '));
        }
        result.append(symbol.text);
    }
    return result;
}

int OcrTextSelectionModel::hitTest(
    const QPointF &imagePosition) const
{
    int nearest = -1;
    qreal nearestDistance = std::numeric_limits<qreal>::max();
    for (int index = 0; index < m_result.symbols.size(); ++index) {
        const QRectF bounds = m_result.symbols.at(index).bounds;
        if (bounds.contains(imagePosition))
            return index;
        const QRectF forgiving = bounds.adjusted(-3, -3, 3, 3);
        if (!forgiving.contains(imagePosition))
            continue;
        const QPointF delta = imagePosition - bounds.center();
        const qreal distance = delta.x() * delta.x()
            + delta.y() * delta.y();
        if (distance < nearestDistance) {
            nearest = index;
            nearestDistance = distance;
        }
    }
    return nearest;
}

QVector<QRectF> OcrTextSelectionModel::selectedBounds() const
{
    QVector<QRectF> result;
    if (!hasSelection())
        return result;
    const int first = firstSelectedIndex();
    const int last = lastSelectedIndex();
    result.reserve(last - first + 1);
    QRectF lineBounds = m_result.symbols.at(first).bounds;
    int lineIndex = m_result.symbols.at(first).lineIndex;
    for (int index = first + 1; index <= last; ++index) {
        const OcrSymbol &symbol = m_result.symbols.at(index);
        if (symbol.lineIndex != lineIndex) {
            result.append(lineBounds.normalized());
            lineBounds = symbol.bounds;
            lineIndex = symbol.lineIndex;
            continue;
        }
        lineBounds = lineBounds.united(symbol.bounds);
    }
    result.append(lineBounds.normalized());
    return result;
}

const QVector<OcrSymbol> &OcrTextSelectionModel::symbols() const
{
    return m_result.symbols;
}

bool OcrTextSelectionModel::isSymbolSelected(int index) const
{
    return hasSelection() && index >= firstSelectedIndex()
        && index <= lastSelectedIndex();
}

int OcrTextSelectionModel::symbolAtPosition(
    const QPointF &imagePosition, bool allowNearestLine) const
{
    if (m_result.symbols.isEmpty())
        return -1;

    int chosenFirst = -1;
    int chosenLast = -1;
    qreal chosenDistance = std::numeric_limits<qreal>::max();
    for (int first = 0; first < m_result.symbols.size();) {
        int last = first;
        QRectF lineBounds = m_result.symbols.at(first).bounds;
        while (last + 1 < m_result.symbols.size()
               && m_result.symbols.at(last + 1).lineIndex
                   == m_result.symbols.at(first).lineIndex) {
            ++last;
            lineBounds = lineBounds.united(
                m_result.symbols.at(last).bounds);
        }

        const qreal verticalDistance = distanceToInterval(
            imagePosition.y(), lineBounds.top(), lineBounds.bottom());
        const bool insideLine = verticalDistance <= 3.0
            && imagePosition.x() >= lineBounds.left() - 3.0
            && imagePosition.x() <= lineBounds.right() + 3.0;
        if (insideLine) {
            chosenFirst = first;
            chosenLast = last;
            break;
        }
        if (allowNearestLine && verticalDistance < chosenDistance) {
            chosenDistance = verticalDistance;
            chosenFirst = first;
            chosenLast = last;
        }
        first = last + 1;
    }
    if (chosenFirst < 0)
        return -1;

    int nearestSymbol = chosenFirst;
    qreal nearestDistance = std::numeric_limits<qreal>::max();
    for (int index = chosenFirst; index <= chosenLast; ++index) {
        const QRectF bounds = m_result.symbols.at(index).bounds;
        const qreal horizontalDistance = distanceToInterval(
            imagePosition.x(), bounds.left(), bounds.right());
        const qreal centerDistance = std::abs(
            imagePosition.x() - bounds.center().x());
        const qreal distance = horizontalDistance * 1'000.0
            + centerDistance;
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestSymbol = index;
        }
    }
    return nearestSymbol;
}

bool OcrTextSelectionModel::beginSelection(
    const QPointF &imagePosition)
{
    const int index = hitTest(imagePosition);
    if (index < 0)
        return false;
    setSelection(index, index);
    return true;
}

bool OcrTextSelectionModel::updateSelection(
    const QPointF &imagePosition)
{
    if (m_anchor < 0)
        return false;
    int index = hitTest(imagePosition);
    if (index < 0)
        index = symbolAtPosition(imagePosition, true);
    if (index < 0)
        return false;
    setSelection(m_anchor, index);
    return true;
}

bool OcrTextSelectionModel::selectWordAt(
    const QPointF &imagePosition)
{
    const int index = hitTest(imagePosition);
    if (index < 0)
        return false;
    const int word = m_result.symbols.at(index).wordIndex;
    int first = index;
    int last = index;
    while (first > 0
           && m_result.symbols.at(first - 1).wordIndex == word) {
        --first;
    }
    while (last + 1 < m_result.symbols.size()
           && m_result.symbols.at(last + 1).wordIndex == word) {
        ++last;
    }
    setSelection(first, last);
    return true;
}

void OcrTextSelectionModel::selectAll()
{
    if (m_result.symbols.isEmpty())
        return;
    setSelection(0, m_result.symbols.size() - 1);
}

void OcrTextSelectionModel::clearSelection()
{
    if (m_anchor < 0 && m_current < 0)
        return;
    m_anchor = -1;
    m_current = -1;
    emit selectionChanged(false);
}

void OcrTextSelectionModel::setSelection(
    int anchor, int current)
{
    const bool oldHasSelection = hasSelection();
    const int oldAnchor = m_anchor;
    const int oldCurrent = m_current;
    m_anchor = anchor;
    m_current = current;
    if (oldAnchor != m_anchor || oldCurrent != m_current
        || oldHasSelection != hasSelection()) {
        emit selectionChanged(hasSelection());
    }
}
