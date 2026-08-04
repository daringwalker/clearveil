#include "formatcapabilitiesdialog.h"

#include "formatcapabilities.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

FormatCapabilitiesDialog::FormatCapabilitiesDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("formatCapabilitiesDialog"));
    setWindowTitle(tr("Supported image formats"));
    resize(820, 560);

    auto *layout = new QVBoxLayout(this);
    auto *description = new QLabel(
        tr("Format support is detected from the image plugins available to "
           "this Clearveil process. Installing a plugin requires restarting "
           "Clearveil before it appears here."),
        this);
    description->setWordWrap(true);
    layout->addWidget(description);

    const QList<ImageFormatCapability> capabilities =
        FormatCapabilities::capabilities();
    const int readable = std::count_if(
        capabilities.cbegin(), capabilities.cend(),
        [](const ImageFormatCapability &capability) {
            return capability.readable;
        });
    const int writable = std::count_if(
        capabilities.cbegin(), capabilities.cend(),
        [](const ImageFormatCapability &capability) {
            return capability.writable;
        });
    auto *summary = new QLabel(
        tr("Qt %1 · %2 readable families · %3 writable families")
            .arg(QString::fromLatin1(qVersion()))
            .arg(readable)
            .arg(writable),
        this);
    summary->setObjectName(QStringLiteral("formatCapabilitiesSummary"));
    layout->addWidget(summary);

    auto *tree = new QTreeWidget(this);
    tree->setObjectName(QStringLiteral("formatCapabilitiesTree"));
    tree->setAccessibleName(tr("Runtime image format support"));
    tree->setColumnCount(5);
    tree->setHeaderLabels({
        tr("Format"), tr("Extensions"), tr("Read"),
        tr("Write"), tr("Backend"),
    });
    tree->setRootIsDecorated(true);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(true);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->header()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(
        4, QHeaderView::ResizeToContents);

    QHash<QString, QTreeWidgetItem *> categories;
    QTreeWidgetItem *firstUnavailable = nullptr;
    for (const ImageFormatCapability &capability : capabilities) {
        QTreeWidgetItem *category =
            categories.value(capability.category);
        if (!category) {
            category = new QTreeWidgetItem(tree);
            category->setText(0, capability.category);
            QFont font = category->font(0);
            font.setBold(true);
            category->setFont(0, font);
            category->setFirstColumnSpanned(true);
            categories.insert(capability.category, category);
        }

        auto *item = new QTreeWidgetItem(category);
        item->setText(0, capability.name);
        QStringList extensions;
        for (const QString &extension : capability.extensions)
            extensions.append(QLatin1Char('.') + extension);
        item->setText(1, extensions.join(QStringLiteral(", ")));
        const auto setSupportMarker =
            [item](int column, bool supported) {
            item->setText(
                column,
                supported
                    ? QStringLiteral("✓")
                    : QStringLiteral("✕"));
            item->setTextAlignment(
                column, Qt::AlignCenter);
            item->setForeground(
                column,
                supported
                    ? QBrush(QColor(26, 145, 72))
                    : QBrush(QColor(196, 54, 54)));
            item->setToolTip(
                column,
                supported
                    ? tr("Supported")
                    : tr("Not supported"));
            item->setData(
                column, Qt::AccessibleTextRole,
                supported
                    ? tr("Supported")
                    : tr("Not supported"));
        };
        setSupportMarker(2, capability.readable);
        setSupportMarker(3, capability.writable);
        item->setText(4, capability.backend);
        item->setData(
            0, Qt::UserRole,
            capability.extensions.value(0));
        item->setData(
            0, Qt::UserRole + 1,
            capability.readable);
        if (!capability.readable) {
            if (!firstUnavailable)
                firstUnavailable = item;
            const QBrush disabled =
                palette().brush(QPalette::Disabled, QPalette::Text);
            for (const int column : {0, 1, 4})
                item->setForeground(column, disabled);
        }
    }
    tree->expandAll();
    layout->addWidget(tree, 1);

    auto *installGroup = new QGroupBox(
        tr("Install support for the selected format"), this);
    installGroup->setObjectName(
        QStringLiteral("formatInstallAdviceGroup"));
    auto *installLayout = new QHBoxLayout(installGroup);
    auto *installAdvice = new QLabel(installGroup);
    installAdvice->setObjectName(
        QStringLiteral("formatInstallAdvice"));
    installAdvice->setWordWrap(true);
    installAdvice->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        | Qt::TextSelectableByKeyboard);
    installLayout->addWidget(installAdvice, 1);
    auto *copyInstallCommand = new QPushButton(
        tr("Copy installation command"), installGroup);
    copyInstallCommand->setObjectName(
        QStringLiteral("copyFormatInstallCommandButton"));
    copyInstallCommand->setAccessibleName(
        tr("Copy package installation command"));
    copyInstallCommand->setEnabled(false);
    installLayout->addWidget(copyInstallCommand);
    layout->addWidget(installGroup);

    const auto updateInstallAdvice =
        [installAdvice, copyInstallCommand](
            QTreeWidgetItem *item) {
        if (!item || !item->parent()) {
            installAdvice->setText(
                tr("Select a format to see its package and installation command."));
            copyInstallCommand->setEnabled(false);
            copyInstallCommand->setProperty(
                "installationCommand", QString());
            return;
        }
        if (item->data(0, Qt::UserRole + 1).toBool()) {
            installAdvice->setText(
                tr("This format is already readable. No additional decoder package is required."));
            copyInstallCommand->setEnabled(false);
            copyInstallCommand->setProperty(
                "installationCommand", QString());
            return;
        }

        const ImageFormatInstallationAdvice advice =
            FormatCapabilities::installationAdvice(
                item->data(0, Qt::UserRole).toString());
        QString text;
        if (!advice.command.isEmpty()) {
            text = tr("%1: install %2\nCommand: %3")
                       .arg(advice.distribution,
                            advice.packages.join(
                                QStringLiteral(", ")),
                            advice.command);
        } else {
            text = tr("Look for these package names in your distribution: %1")
                       .arg(advice.packages.join(
                           QStringLiteral(", ")));
        }
        if (!advice.note.isEmpty())
            text += QLatin1String("\n") + advice.note;
        installAdvice->setText(text);
        copyInstallCommand->setProperty(
            "installationCommand", advice.command);
        copyInstallCommand->setEnabled(
            !advice.command.isEmpty());
    };
    connect(tree, &QTreeWidget::currentItemChanged,
            this,
            [updateInstallAdvice](QTreeWidgetItem *current,
                                  QTreeWidgetItem *) {
        updateInstallAdvice(current);
    });
    connect(copyInstallCommand, &QPushButton::clicked,
            this, [copyInstallCommand] {
        QApplication::clipboard()->setText(
            copyInstallCommand
                ->property("installationCommand")
                .toString());
    });
    if (firstUnavailable)
        tree->setCurrentItem(firstUnavailable);
    else if (tree->topLevelItemCount() > 0
             && tree->topLevelItem(0)->childCount() > 0) {
        tree->setCurrentItem(
            tree->topLevelItem(0)->child(0));
    } else {
        updateInstallAdvice(nullptr);
    }

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Close, this);
    auto *copyButton = buttons->addButton(
        tr("Copy report"), QDialogButtonBox::ActionRole);
    copyButton->setObjectName(
        QStringLiteral("copyFormatReportButton"));
    copyButton->setAccessibleName(tr("Copy format support report"));
    connect(copyButton, &QPushButton::clicked, this,
            [capabilities] {
        QStringList lines{
            QStringLiteral("Clearveil image format support"),
            QStringLiteral("Qt %1").arg(
                QString::fromLatin1(qVersion())),
        };
        for (const ImageFormatCapability &capability : capabilities) {
            lines.append(
                QStringLiteral("%1: %2 | read=%3 | write=%4 | %5")
                    .arg(capability.name,
                         capability.extensions.join(QLatin1Char(',')),
                         capability.readable
                             ? QStringLiteral("yes")
                             : QStringLiteral("no"),
                         capability.writable
                             ? QStringLiteral("yes")
                             : QStringLiteral("no"),
                         capability.backend));
        }
        QApplication::clipboard()->setText(
            lines.join(QLatin1Char('\n')));
    });
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    layout->addWidget(buttons);
}
