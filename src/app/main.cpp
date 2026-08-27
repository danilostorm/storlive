#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

#include "core/AppController.h"
#include "core/obs/ObsPreviewProvider.h"

namespace {
void appendStartupLog(const QString &message)
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dirPath);
    QFile file(QDir(dirPath).filePath(QStringLiteral("storlive-startup.log")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << QDateTime::currentDateTime().toString(Qt::ISODate)
           << "  " << message << '\n';
    stream.flush();
}
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("StorLive"));
    QCoreApplication::setApplicationName(QStringLiteral("StorLive"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.1"));

    appendStartupLog(QStringLiteral("StorLive 0.1.1 iniciando"));
    appendStartupLog(QStringLiteral("Criando AppController"));
    AppController controller;
    appendStartupLog(QStringLiteral("AppController criado"));

    QQmlApplicationEngine engine;
    appendStartupLog(QStringLiteral("QQmlApplicationEngine criado"));

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &engine,
                     [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings)
            appendStartupLog(QStringLiteral("QML: %1").arg(warning.toString()));
    });

    appendStartupLog(QStringLiteral("Criando preview provider"));
    auto *previewProvider = new ObsPreviewProvider;
    const bool previewStarted = previewProvider->start();
    appendStartupLog(QStringLiteral("Preview provider criado; ativo=%1").arg(previewStarted));
    engine.addImageProvider(QStringLiteral("preview"), previewProvider);

    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("previewProvider"), previewProvider);

    appendStartupLog(QStringLiteral("Carregando qrc:/StorLive/Main.qml"));
    engine.load(QUrl(QStringLiteral("qrc:/StorLive/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        appendStartupLog(QStringLiteral("Falha fatal: a janela QML não foi criada"));
        return EXIT_FAILURE;
    }

    appendStartupLog(QStringLiteral("Janela principal criada com sucesso"));
    return app.exec();
}
