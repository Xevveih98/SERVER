#ifndef TODODATABASE_H
#define TODODATABASE_H

#include <QString>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class TodoDatabase
{
public:
    static bool saveUserTodo(const QString &login, const QString &name);
    static QStringList getUserTodoos(const QString &login);
    static bool deleteTodo(const QString &login, const QString &name);
};

#endif // TODODATABASE_H
