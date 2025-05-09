#include "FoldersDatabase.h"

bool FoldersDatabase::saveUserFolder(const QString &login, const QStringList &tags)
{
    // Удаляем старые теги пользователя
    QSqlQuery deleteQuery;
    deleteQuery.prepare(R"(DELETE FROM user_tags WHERE user_login = :login)");
    deleteQuery.bindValue(":login", login);
    if (!deleteQuery.exec()) {
        qWarning() << "Failed to delete old user_tags:" << deleteQuery.lastError().text();
        return false;
    }

    // Добавляем новые теги
    for (const QString &tag : tags) {
        QSqlQuery insertQuery;
        insertQuery.prepare(R"(
            INSERT INTO user_tags (name, user_login)
            VALUES (:name, :login)
            ON CONFLICT (user_login, name) DO NOTHING
        )");
        insertQuery.bindValue(":name", tag);
        insertQuery.bindValue(":login", login);
        if (!insertQuery.exec()) {
            qWarning() << "Failed to insert user tag:" << insertQuery.lastError().text();
        }
    }

    return true;
}

QStringList FoldersDatabase::getUserFolder(const QString &login)
{
    QStringList tags;

    QSqlQuery query;
    query.prepare(R"(
        SELECT name FROM user_tags
        WHERE user_login = :login
    )");
    query.bindValue(":login", login);

    if (query.exec()) {
        while (query.next())
            tags << query.value(0).toString();
    } else {
        qWarning() << "Failed to fetch tags for user" << login << ":" << query.lastError().text();
    }

    return tags;
}


bool FoldersDatabase::deleteFolder(const QString &login, const QString &tag)
{
    QSqlQuery query;
    query.prepare(R"(
        DELETE FROM user_tags
        WHERE user_login = :login AND name = :tag
    )");
    query.bindValue(":login", login);
    query.bindValue(":tag", tag);
    if (!query.exec()) {
        qWarning() << "Failed to delete tag for user:" << query.lastError().text();
        return false;
    }

    return true;
}
