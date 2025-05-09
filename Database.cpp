#include "Database.h"


bool Database::connect() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("MindTraceMainDB");
    db.setUserName("postgres");  // замените на нужное имя пользователя
    db.setPassword("123");

    if (!db.open()) {
        qCritical() << "Failed to connect to database:" << db.lastError().text();
        return false;
    }

    qInfo() << "Successfully connected to database.";
    return true;
}
