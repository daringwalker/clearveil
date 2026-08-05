#include "mainwindow.h"
#include "imagedecoder.h"
#include "singleinstance.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("clearveil"));
    QApplication::setApplicationVersion(QStringLiteral(CLEARVEIL_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Clearveil"));
    QApplication::setDesktopFileName(
        QStringLiteral("io.github.daringwalker.clearveil"));
    ImageDecoder::configureProcessImageIo();

    QSettings settings;
    const QString language = settings.value(QStringLiteral("ui/language"),
                                             QStringLiteral("system")).toString();
    const bool useChinese = language == QStringLiteral("zh_CN")
        || (language == QStringLiteral("system")
            && QLocale::system().name().startsWith(QStringLiteral("zh"),
                                                   Qt::CaseInsensitive));
    QApplication::setApplicationDisplayName(
        useChinese
            ? QStringLiteral("云开见月明")
            : QStringLiteral("Clearveil"));
    QTranslator qtTranslator;
    QTranslator clearveilTranslator;
    if (useChinese) {
        if (qtTranslator.load(QStringLiteral("qtbase_zh_CN"),
                              QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
            application.installTranslator(&qtTranslator);
        if (clearveilTranslator.load(QStringLiteral(":/i18n/clearveil_zh_CN.qm")))
            application.installTranslator(&clearveilTranslator);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("A fast, friendly image viewer for Linux"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("image"),
                                 QObject::tr("Images or folder to open."),
                                 QStringLiteral("[image...]"));
    parser.process(application);

    SingleInstance singleInstance;
    if (!singleInstance.startOrForward(parser.positionalArguments()))
        return 0;

    MainWindow window;
    QObject::connect(&singleInstance, &SingleInstance::pathsReceived,
                     &window, [&window](const QStringList &paths) {
        if (paths.isEmpty())
            window.present();
        else
            window.openPaths(paths);
    });
    window.show();
    if (!parser.positionalArguments().isEmpty())
        window.openPaths(parser.positionalArguments());

    return application.exec();
}
