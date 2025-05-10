#include "FoldersDatabase.h"

bool FoldersDatabase::saveUserFolder(const QString &login, const QStringList &folders)
{
    if (folders.isEmpty())
        return false;

    // Добавляем новую папку (или папки — если список всё же содержит несколько)
    for (const QString &folderName : folders) {
        QSqlQuery insertQuery;
        insertQuery.prepare(R"(
            INSERT INTO folders (name, user_login)
            VALUES (:name, :login)
            ON CONFLICT DO NOTHING
        )");
        insertQuery.bindValue(":name", folderName);
        insertQuery.bindValue(":login", login);
        if (!insertQuery.exec()) {
            qWarning() << "Failed to insert folder:" << insertQuery.lastError().text();
            return false;
        }
    }

    return true;
}

QList<QPair<QString, QString>> FoldersDatabase::getUserFolders(const QString &login)
{
    QList<QPair<QString, QString>> folders;

    QSqlQuery query;
    query.prepare(R"(
        SELECT name, itemcount FROM folders
        WHERE user_login = :login
        ORDER BY id ASC
    )");
    query.bindValue(":login", login);

    if (query.exec()) {
        while (query.next()) {
            QString name = query.value("name").toString();
            QString itemCount = query.value("itemCount").toString();
            folders.append(qMakePair(name, itemCount));
        }
    } else {
        qWarning() << "Failed to get user folders:" << query.lastError().text();
    }

    return folders;
}



bool FoldersDatabase::deleteFolder(const QString &login, const QString &folder)
{
    QSqlQuery query;
    query.prepare(R"(
        DELETE FROM folders
        WHERE user_login = :login AND name = :folder
    )");
    query.bindValue(":login", login);
    query.bindValue(":folder", folder);
    if (!query.exec()) {
        qWarning() << "Failed to delete folder for user:" << query.lastError().text();
        return false;
    }

    return true;
}
