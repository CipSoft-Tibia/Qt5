#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QtGui>
#include <QtQuick3D/qquick3d.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QSurfaceFormat::setDefaultFormat(QQuick3D::idealSurfaceFormat(4));
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
