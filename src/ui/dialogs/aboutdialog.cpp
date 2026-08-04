#include "aboutdialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QSysInfo>
#include <QVBoxLayout>

namespace {
constexpr auto kProjectUrl =
    "https://github.com/daringwalker/clearveil";
}

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("aboutDialog"));
    setWindowTitle(tr("About Clearveil"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setModal(true);
    setMinimumWidth(480);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(16);

    auto *header = new QHBoxLayout;
    header->setSpacing(18);
    auto *iconLabel = new QLabel(this);
    iconLabel->setObjectName(QStringLiteral("aboutApplicationIcon"));
    iconLabel->setPixmap(
        QPixmap(QStringLiteral(":/icons/clearveil.svg"))
            .scaled(72, 72, Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
    iconLabel->setFixedSize(72, 72);
    iconLabel->setAccessibleName(tr("Clearveil application icon"));
    header->addWidget(iconLabel, 0, Qt::AlignTop);

    auto *identity = new QVBoxLayout;
    identity->setSpacing(3);
    auto *nameLabel = new QLabel(tr("Clearveil"), this);
    nameLabel->setObjectName(QStringLiteral("aboutApplicationName"));
    QFont nameFont = nameLabel->font();
    nameFont.setPointSizeF(nameFont.pointSizeF() + 5.0);
    nameFont.setWeight(QFont::DemiBold);
    nameLabel->setFont(nameFont);
    identity->addWidget(nameLabel);

    const QString version = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral(CLEARVEIL_VERSION)
        : QCoreApplication::applicationVersion();
    auto *versionLabel = new QLabel(tr("Version %1").arg(version), this);
    versionLabel->setObjectName(QStringLiteral("aboutVersionLabel"));
    versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    identity->addWidget(versionLabel);

    auto *descriptionLabel = new QLabel(
        tr("A fast, friendly image viewer for Linux."), this);
    descriptionLabel->setObjectName(QStringLiteral("aboutDescriptionLabel"));
    descriptionLabel->setWordWrap(true);
    identity->addSpacing(5);
    identity->addWidget(descriptionLabel);
    header->addLayout(identity, 1);
    root->addLayout(header);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);
    root->addWidget(separator);

    auto *details = new QVBoxLayout;
    details->setSpacing(8);
    auto *runtimeLabel = new QLabel(
        tr("Built with Qt %1 · Running on %2")
            .arg(QString::fromLatin1(qVersion()),
                 QSysInfo::prettyProductName()),
        this);
    runtimeLabel->setObjectName(QStringLiteral("aboutRuntimeLabel"));
    runtimeLabel->setWordWrap(true);
    runtimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    details->addWidget(runtimeLabel);

    auto *licenseLabel = new QLabel(
        tr("Free and open-source software licensed under GNU GPL v3.0 or later."),
        this);
    licenseLabel->setObjectName(QStringLiteral("aboutLicenseLabel"));
    licenseLabel->setWordWrap(true);
    details->addWidget(licenseLabel);

    auto *projectLabel = new QLabel(
        tr("Project repository: <a href=\"%1\">%1</a>")
            .arg(projectUrl().toHtmlEscaped()),
        this);
    projectLabel->setObjectName(QStringLiteral("aboutProjectLink"));
    projectLabel->setAccessibleName(tr("Project repository"));
    projectLabel->setOpenExternalLinks(true);
    projectLabel->setTextInteractionFlags(
        Qt::TextBrowserInteraction);
    projectLabel->setWordWrap(true);
    details->addWidget(projectLabel);
    root->addLayout(details);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Close, this);
    buttons->setObjectName(QStringLiteral("aboutButtonBox"));
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    root->addWidget(buttons);
}

QString AboutDialog::projectUrl()
{
    return QString::fromLatin1(kProjectUrl);
}
