#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "core/AppController.h"
#include "core/obs/ObsPreviewProvider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("StorLive"));
    QCoreApplication::setApplicationName(QStringLiteral("StorLive"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0-dev"));

    AppController controller;
    QQmlApplicationEngine engine;

    auto *previewProvider = new ObsPreviewProvider;
    previewProvider->start();
    engine.addImageProvider(QStringLiteral("preview"), previewProvider);

    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("previewProvider"), previewProvider);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/StorLive/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
