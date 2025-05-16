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


QList<FoldersDatabase::FolderItem> FoldersDatabase::getUserFolders(const QString &login)
{
    QList<FolderItem> folders;

    QSqlQuery query;
    query.prepare(R"(
        SELECT id, name, itemcount FROM folders
        WHERE user_login = :login
        ORDER BY id ASC
    )");
    query.bindValue(":login", login);

    if (query.exec()) {
        while (query.next()) {
            FolderItem folder;
            folder.id = query.value("id").toInt();
            folder.name = query.value("name").toString();
            folder.itemCount = query.value("itemcount").toInt();
            folders.append(folder);
        }
    } else {
        qWarning() << "Failed to get user folders:" << query.lastError().text();
    }

    return folders;
}


bool FoldersDatabase::deleteFolder(const QString &login, const QString &folder)
{
    // Проверка количества папок у пользователя
    QSqlQuery countQuery;
    countQuery.prepare(R"(
        SELECT COUNT(*) FROM folders WHERE user_login = :login
    )");

    countQuery.bindValue(":login", login);
    if (!countQuery.exec() || !countQuery.next()) {
        qWarning() << "Failed to count folders for user:" << countQuery.lastError().text();
        return false;
    }

    int folderCount = countQuery.value(0).toInt();
    if (folderCount <= 1) {
        qWarning() << "Cannot delete the last remaining folder for user:" << login;
        return false;
    }

    // Удаление папки
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
