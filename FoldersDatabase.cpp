#include "FoldersDatabase.h"

bool FoldersDatabase::saveUserFolder(const QString &login, const QStringList &folders)
{
    if (folders.isEmpty())
        return false;

    for (const QString &folderName : folders) {
        // Проверка: существует ли уже такая папка у этого пользователя
        QSqlQuery checkQuery;
        checkQuery.prepare(R"(
            SELECT 1 FROM folders
            WHERE name = :name AND user_login = :login
            LIMIT 1
        )");
        checkQuery.bindValue(":name", folderName);
        checkQuery.bindValue(":login", login);
        if (!checkQuery.exec()) {
            qWarning() << "Failed to check for existing folder:" << checkQuery.lastError().text();
            return false;
        }
        if (checkQuery.next()) {
            qWarning() << "Folder already exists for user:" << login << ", folder:" << folderName;
            return false; // или continue; если хочешь просто пропустить
        }

        // Вставка
        QSqlQuery insertQuery;
        insertQuery.prepare(R"(
            INSERT INTO folders (name, user_login)
            VALUES (:name, :login)
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

bool FoldersDatabase::changeUserFolder(const QString &login, const QString &oldName, const QString &newName) {

    QSqlQuery query;
    query.prepare(R"(UPDATE folders
    SET name = :newName
    WHERE ctid IN (
        SELECT ctid FROM folders
        WHERE user_login = :login AND name = :oldName
        LIMIT 1
    ))");

    query.bindValue(":newName", newName);
    query.bindValue(":login", login);
    query.bindValue(":oldName", oldName);

    if (!query.exec()) {
        qCritical() << "Failed to change folder for" << login << ":" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No folder found with login:" << login << "and name:" << oldName;
        return false;
    }

    qInfo() << "Folder name updated from" << oldName << "to" << newName << "for user:" << login;
    return true;
}
