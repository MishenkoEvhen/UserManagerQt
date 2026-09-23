#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QDir>
#include "usermodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    UserModel userModel;

    const QString dataDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    userModel.setAutoSavePath(dataDir + "/users.json");

    // Загружаем предыдущие данные при старте
    userModel.loadFromFile(userModel.autoSavePath());

    // Прокси-модель для фильтрации по имени
    UserFilterModel filterModel;
    filterModel.setSourceModel(&userModel);

    engine.rootContext()->setContextProperty("userModel",   &userModel);
    engine.rootContext()->setContextProperty("filterModel", &filterModel);

    engine.loadFromModule("UserManager", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}