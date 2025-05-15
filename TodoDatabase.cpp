// TodoDatabase.cpp

#include "TodoDatabase.h"

bool TodoDatabase::saveUserTodo(const QString &login, const QString &name)
{
    if (login.isEmpty() || name.isEmpty())
        return false;

    QSqlQuery checkQuery;
    checkQuery.prepare(R"(
        SELECT 1 FROM user_todo
        WHERE user_login = :login AND name = :name
        LIMIT 1
    )");
    checkQuery.bindValue(":login", login);
    checkQuery.bindValue(":name", name);
    if (!checkQuery.exec()) {
        qWarning() << "Failed to check existing todo:" << checkQuery.lastError().text();
        return false;
    }
    if (checkQuery.next()) {
        qWarning() << "Todo already exists for user:" << login << " name:" << name;
        return false;
    }

    QSqlQuery insertQuery;
    insertQuery.prepare(R"(
        INSERT INTO user_todo (user_login, name)
        VALUES (:login, :name)
    )");
    insertQuery.bindValue(":login", login);
    insertQuery.bindValue(":name", name);
    if (!insertQuery.exec()) {
        qWarning() << "Failed to insert todo:" << insertQuery.lastError().text();
        return false;
    }

    return true;
}

QStringList TodoDatabase::getUserTodoos(const QString &login)
{
    QStringList todos;

    QSqlQuery query;
    query.prepare(R"(
        SELECT name FROM user_todo
        WHERE user_login = :login
        ORDER BY id ASC
    )");
    query.bindValue(":login", login);
    if (!query.exec()) {
        qWarning() << "Failed to load todos:" << query.lastError().text();
        return todos;
    }

    while (query.next()) {
        todos.append(query.value("name").toString());
    }

    return todos;
}

bool TodoDatabase::deleteTodo(const QString &login, const QString &name)
{
    QSqlQuery query;
    query.prepare(R"(
        DELETE FROM user_todo
        WHERE user_login = :login AND name = :name
    )");
    query.bindValue(":login", login);
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "Failed to delete todo:" << query.lastError().text();
        return false;
    }

    return true;
}
