// SPDX-FileCopyrightText: 2026 daringwalker
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ocrsupportdialog.h"

#include "ocrengine.h"
#include "ocrsupport.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

OcrSupportDialog::OcrSupportDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("ocrSupportDialog"));
    setWindowTitle(tr("OCR support"));
    resize(680, 430);

    auto *layout = new QVBoxLayout(this);
    auto *description = new QLabel(
        tr("Clearveil uses the local Tesseract engine and installed language models. No image or recognized text is uploaded."),
        this);
    description->setWordWrap(true);
    layout->addWidget(description);

    const bool engineAvailable = OcrEngine::isAvailable();
    const QStringList languages = OcrEngine::availableLanguages();
    const QString activeLanguages =
        OcrEngine::recognitionLanguages(languages);

    auto *statusGroup = new QGroupBox(tr("Current OCR status"), this);
    auto *statusLayout = new QFormLayout(statusGroup);
    auto *engineStatus = new QLabel(
        engineAvailable ? tr("✓ Available") : tr("✕ Not included in this build"),
        statusGroup);
    engineStatus->setObjectName(QStringLiteral("ocrEngineStatus"));
    engineStatus->setStyleSheet(engineAvailable
        ? QStringLiteral("color: #24934f; font-weight: 600;")
        : QStringLiteral("color: #c43b3b; font-weight: 600;"));
    statusLayout->addRow(tr("Recognition engine:"), engineStatus);
    auto *models = new QLabel(
        languages.isEmpty() ? tr("None") : languages.join(QStringLiteral(", ")),
        statusGroup);
    models->setObjectName(QStringLiteral("ocrInstalledLanguages"));
    models->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    models->setWordWrap(true);
    statusLayout->addRow(tr("Installed language models:"), models);
    auto *active = new QLabel(
        activeLanguages.isEmpty() ? tr("None") : activeLanguages,
        statusGroup);
    active->setObjectName(QStringLiteral("ocrActiveLanguages"));
    active->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    active->setWordWrap(true);
    statusLayout->addRow(tr("Used for recognition:"), active);
    layout->addWidget(statusGroup);

    const OcrInstallationAdvice advice =
        OcrSupport::installationAdvice({}, !engineAvailable);
    auto *installGroup = new QGroupBox(
        engineAvailable && !languages.isEmpty()
            ? tr("Install additional recognition languages")
            : tr("Install OCR support"),
        this);
    auto *installLayout = new QVBoxLayout(installGroup);
    auto *packages = new QLabel(
        advice.distribution.isEmpty()
            ? tr("Packages: %1").arg(advice.packages.join(QStringLiteral(", ")))
            : tr("%1 packages: %2")
                  .arg(advice.distribution,
                       advice.packages.join(QStringLiteral(", "))),
        installGroup);
    packages->setObjectName(QStringLiteral("ocrPackageAdvice"));
    packages->setWordWrap(true);
    packages->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    installLayout->addWidget(packages);

    QPlainTextEdit *command = nullptr;
    if (!advice.command.isEmpty()) {
        command = new QPlainTextEdit(advice.command, installGroup);
        command->setObjectName(QStringLiteral("ocrInstallCommand"));
        command->setReadOnly(true);
        command->setMaximumHeight(
            command->fontMetrics().lineSpacing() * 3);
        command->setAccessibleName(tr("OCR package installation command"));
        installLayout->addWidget(command);
    }
    auto *note = new QLabel(advice.note, installGroup);
    note->setObjectName(QStringLiteral("ocrInstallNote"));
    note->setWordWrap(true);
    installLayout->addWidget(note);
    layout->addWidget(installGroup);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Close, this);
    if (!advice.command.isEmpty()) {
        auto *copy = buttons->addButton(
            tr("Copy installation command"),
            QDialogButtonBox::ActionRole);
        copy->setObjectName(QStringLiteral("copyOcrInstallCommandButton"));
        connect(copy, &QPushButton::clicked, this,
                [command] {
            QApplication::clipboard()->setText(command->toPlainText());
        });
    }
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    layout->addWidget(buttons);
}
