#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "tablemodel.h"
#include <QQmlContext>
#include "DBChandler.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);

    //qmlregistertypes

   qmlRegisterType<tablemodel>("TableModel",0, 1, "TableModel");
   qmlRegisterType<tablemodel>("TableModel2",0, 1, "TableModelSignal");
    //class definitions

    DBCHandler interface(&app) ;
    tablemodel table1,table2;
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);


    QQmlContext * Context1 = engine.rootContext();

    Context1->setContextProperty("comObj", &interface);

    engine.load(url);

    return app.exec();
}
