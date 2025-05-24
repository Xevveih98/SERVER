#include "EntriesDatabase.h"

bool EntriesDatabase::saveUserEntry(const QString &login, const EntryUser &entry)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO entries (user_login, entry_title, entry_content, entry_mood_id, entry_folder_id, entry_date, entry_time)
        VALUES (:login, :title, :content, :moodId, :folderId, :date, :time)
        RETURNING id;
    )");

    query.bindValue(":login", login);
    query.bindValue(":title", entry.title);
    query.bindValue(":content", entry.content);
    query.bindValue(":moodId", entry.moodId);
    query.bindValue(":folderId", entry.folderId);
    query.bindValue(":date", entry.date);
    query.bindValue(":time", entry.time);

    if (!query.exec()) {
        qWarning() << "Ошибка при вставке в entries:" << query.lastError().text();
        return false;
    }

    int entryId = -1;
    if (query.next()) {
        entryId = query.value(0).toInt();
    }
    if (entryId <= 0) {
        qWarning() << "Не удалось получить ID новой записи из entries.";
        return false;
    }

    auto insertRelation = [&](const QString &tableName, const QString &columnName, const QVector<UserItem> &items) -> bool {
        for (const UserItem &item : items) {
            if (item.id <= 0) {
                qWarning() << QString("Пропущен недопустимый ID (%1) для связи %2").arg(item.id).arg(tableName);
                continue;
            }

            QSqlQuery linkQuery;
            QString sql = QString("INSERT INTO %1 (entry_id, %2) VALUES (:entryId, :itemId);")
                              .arg(tableName, columnName);
            linkQuery.prepare(sql);
            linkQuery.bindValue(":entryId", entryId);
            linkQuery.bindValue(":itemId", item.id);

            if (!linkQuery.exec()) {
                qWarning() << QString("Ошибка при вставке в %1 (entry_id=%2, %3=%4): %5")
                                  .arg(tableName)
                                  .arg(entryId)
                                  .arg(columnName)
                                  .arg(item.id)
                                  .arg(linkQuery.lastError().text());
                return false;
            }
        }
        return true;
    };

    if (!insertRelation("entry_tags", "tag_id", entry.tags)) return false;
    if (!insertRelation("entry_user_activities", "user_activity_id", entry.activities)) return false;
    if (!insertRelation("entry_user_emotions", "user_emotion_id", entry.emotions)) return false;

    QSqlQuery updateFolderQuery;
    updateFolderQuery.prepare(R"(
        UPDATE folders
        SET itemcount = itemcount + 1
        WHERE id = :folderId;
    )");
    updateFolderQuery.bindValue(":folderId", entry.folderId);

    if (!updateFolderQuery.exec()) {
        qWarning() << "Ошибка при увеличении itemcount в folders:" << updateFolderQuery.lastError().text();
        return false;
    }

    return true;
}


//--------- загрузка записей -------------------------

QList<EntryUser> EntriesDatabase::getUserEntries(const QString &login, int folderId, int year, int month)
{
    QList<EntryUser> entries;

    QSqlQuery query;
    query.prepare(R"(
        SELECT id, entry_title, entry_content, entry_mood_id, entry_folder_id, entry_date, entry_time
        FROM entries
        WHERE user_login = :login
          AND entry_folder_id = :folderId
          AND EXTRACT(YEAR FROM entry_date) = :year
          AND EXTRACT(MONTH FROM entry_date) = :month
        ORDER BY id ASC
    )");
    query.bindValue(":login", login);
    query.bindValue(":folderId", folderId);
    query.bindValue(":year", year);
    query.bindValue(":month", month);

    if (!query.exec()) {
        qWarning() << "Failed to get entries:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        int entryId = query.value("id").toInt();
        QString title = query.value("entry_title").toString();
        QString content = query.value("entry_content").toString();
        int moodId = query.value("entry_mood_id").toInt();
        int folderId = query.value("entry_folder_id").toInt();
        QDate date = query.value("entry_date").toDate();
        QTime time = query.value("entry_time").toTime();

        QVector<UserItem> tags = getTagsForEntry(entryId);
        QVector<UserItem> activities = getActivitiesForEntry(entryId);
        QVector<UserItem> emotions = getEmotionsForEntry(entryId);

        EntryUser entry(entryId, login, title, content, moodId, folderId, date, time, tags, activities, emotions);
        entries.append(entry);
    }

    return entries;
}

QList<EntryUser> EntriesDatabase::getUserEntriesByKeywords(const QString &login, const QStringList &keywords)
{
    QList<EntryUser> entries;

    if (keywords.isEmpty()) {
        qWarning() << "No keywords provided.";
        return entries;
    }

    QStringList conditions;
    for (int i = 0; i < keywords.size(); ++i) {
        QString key = QString("%%%1%%").arg(keywords[i]);
        conditions << QString(R"(
            (entry_title ILIKE :kw%1 OR
             regexp_replace(
                 regexp_replace(entry_content, '<[^>]*>', '', 'g'),
                 E'&[#a-zA-Z0-9]+;', '', 'g'
             ) ILIKE :kw%1)
        )").arg(i);

    }

    QString keywordCondition = conditions.join(" OR ");

    QString queryStr = QString(R"(
        SELECT id, entry_title, entry_content, entry_mood_id, entry_folder_id, entry_date, entry_time
        FROM entries
        WHERE user_login = :login
          AND (%1)
        ORDER BY id ASC
    )").arg(keywordCondition);

    QSqlQuery query;
    query.prepare(queryStr);
    query.bindValue(":login", login);
    for (int i = 0; i < keywords.size(); ++i) {
        query.bindValue(QString(":kw%1").arg(i), "%" + keywords[i] + "%");
    }

    if (!query.exec()) {
        qWarning() << "Failed to get entries by keywords:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        int entryId = query.value("id").toInt();
        QString title = query.value("entry_title").toString();
        QString content = query.value("entry_content").toString();
        int moodId = query.value("entry_mood_id").toInt();
        int folderId = query.value("entry_folder_id").toInt();
        QDate date = query.value("entry_date").toDate();
        QTime time = query.value("entry_time").toTime();

        QVector<UserItem> tags = getTagsForEntry(entryId);
        QVector<UserItem> activities = getActivitiesForEntry(entryId);
        QVector<UserItem> emotions = getEmotionsForEntry(entryId);

        EntryUser entry(entryId, login, title, content, moodId, folderId, date, time, tags, activities, emotions);
        entries.append(entry);
    }

    return entries;
}

QList<EntryUser> EntriesDatabase::getUserEntriesByTags(const QString &login, const QList<int> &tagIds)
{
    QList<EntryUser> entries;

    if (tagIds.isEmpty()) {
        qWarning() << "No tag IDs provided.";
        return entries;
    }

    QStringList placeholders;
    for (int i = 0; i < tagIds.size(); ++i) {
        placeholders << "?";
    }

    QString queryStr = QString(R"(
        SELECT DISTINCT e.id, e.entry_title, e.entry_content, e.entry_mood_id, e.entry_folder_id, e.entry_date, e.entry_time
        FROM entries e
        JOIN entry_tags et ON e.id = et.entry_id
        JOIN users u ON e.user_login = u.user_login
        WHERE u.user_login = ?
          AND et.tag_id IN (%1)
        ORDER BY e.id ASC
    )").arg(placeholders.join(", "));

    QSqlQuery query;
    if (!query.prepare(queryStr)) {
        qWarning() << "Query prepare failed:" << query.lastError().text();
        return entries;
    }

    query.addBindValue(login);
    for (int tagId : tagIds) {
        query.addBindValue(tagId);
    }

    if (!query.exec()) {
        qWarning() << "Failed to get entries by tags:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        int entryId = query.value("id").toInt();
        QString title = query.value("entry_title").toString();
        QString content = query.value("entry_content").toString();
        int moodId = query.value("entry_mood_id").toInt();
        int folderId = query.value("entry_folder_id").toInt();
        QDate date = query.value("entry_date").toDate();
        QTime time = query.value("entry_time").toTime();

        QVector<UserItem> tags = getTagsForEntry(entryId);
        QVector<UserItem> activities = getActivitiesForEntry(entryId);
        QVector<UserItem> emotions = getEmotionsForEntry(entryId);

        EntryUser entry(entryId, login, title, content, moodId, folderId, date, time, tags, activities, emotions);
        entries.append(entry);
    }

    return entries;
}

QList<EntryUser> EntriesDatabase::getUserEntriesByDate(const QString &login, const QString &dateStr)
{
    QList<EntryUser> entries;

    if (login.isEmpty() || dateStr.isEmpty()) {
        qWarning() << "Login or date is empty.";
        return entries;
    }

    QString queryStr = R"(
        SELECT e.id, e.entry_title, e.entry_content, e.entry_mood_id, e.entry_folder_id, e.entry_date, e.entry_time
        FROM entries e
        JOIN users u ON e.user_login = u.user_login
        WHERE u.user_login = ?
          AND e.entry_date = ?
        ORDER BY e.id ASC
    )";

    QSqlQuery query;
    if (!query.prepare(queryStr)) {
        qWarning() << "Query prepare failed:" << query.lastError().text();
        return entries;
    }

    query.addBindValue(login);
    query.addBindValue(dateStr);

    if (!query.exec()) {
        qWarning() << "Failed to get entries by date:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        int entryId = query.value("id").toInt();
        QString title = query.value("entry_title").toString();
        QString content = query.value("entry_content").toString();
        int moodId = query.value("entry_mood_id").toInt();
        int folderId = query.value("entry_folder_id").toInt();
        QDate date = query.value("entry_date").toDate();
        QTime time = query.value("entry_time").toTime();

        QVector<UserItem> tags = getTagsForEntry(entryId);
        QVector<UserItem> activities = getActivitiesForEntry(entryId);
        QVector<UserItem> emotions = getEmotionsForEntry(entryId);

        EntryUser entry(entryId, login, title, content, moodId, folderId, date, time, tags, activities, emotions);
        entries.append(entry);
    }

    return entries;
}

bool EntriesDatabase::deleteUserEntry(const QString &login, int entryId)
{
    QSqlDatabase::database().transaction();
    QStringList relatedTables = {
        "entry_tags",
        "entry_user_activities",
        "entry_user_emotions"
    };

    for (const QString &table : relatedTables) {
        QSqlQuery deleteRel;
        deleteRel.prepare(QString("DELETE FROM %1 WHERE entry_id = :entryId;").arg(table));
        deleteRel.bindValue(":entryId", entryId);
        if (!deleteRel.exec()) {
            qWarning() << "Ошибка при удалении из " << table << ":" << deleteRel.lastError().text();
            QSqlDatabase::database().rollback();
            return false;
        }
    }
    QSqlQuery deleteEntryQuery;
    deleteEntryQuery.prepare(R"(
        DELETE FROM entries WHERE id = :entryId AND user_login = :login;
    )");
    deleteEntryQuery.bindValue(":entryId", entryId);
    deleteEntryQuery.bindValue(":login", login);

    if (!deleteEntryQuery.exec()) {
        qWarning() << "Ошибка при удалении записи:" << deleteEntryQuery.lastError().text();
        QSqlDatabase::database().rollback();
        return false;
    }

    QSqlDatabase::database().commit();
    return true;
}


bool EntriesDatabase::updateUserEntry(const QString &login, const EntryUser &entry)
{
    if (entry.id <= 0) {
        qWarning() << "Invalid entry id for update:" << entry.id;
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);

    int oldFolderId = -1;
    query.prepare("SELECT entry_folder_id FROM entries WHERE id = :id AND user_login = :login");
    query.bindValue(":id", entry.id);
    query.bindValue(":login", login);

    if (!query.exec() || !query.next()) {
        qWarning() << "Не удалось получить старую папку для записи id:" << entry.id << query.lastError().text();
        return false;
    }
    oldFolderId = query.value(0).toInt();

    query.prepare(R"(
        UPDATE entries
        SET entry_title = :title,
            entry_content = :content,
            entry_mood_id = :moodId,
            entry_date = :date,
            entry_time = :time,
            entry_folder_id = :folderId
        WHERE id = :id AND user_login = :login
    )");

    query.bindValue(":title", entry.title);
    query.bindValue(":content", entry.content);
    query.bindValue(":moodId", entry.moodId);
    query.bindValue(":folderId", entry.folderId);
    query.bindValue(":date", entry.date);
    query.bindValue(":time", entry.time);
    query.bindValue(":id", entry.id);
    query.bindValue(":login", login);

    if (!query.exec()) {
        qWarning() << "Ошибка при обновлении записи в entries:" << query.lastError().text();
        return false;
    }

    // 3) Удаляем старые связи с тегами, активностями и эмоциями
    auto deleteRelations = [&](const QString &tableName) -> bool {
        QSqlQuery delQuery(db);
        delQuery.prepare(QString("DELETE FROM %1 WHERE entry_id = :entryId").arg(tableName));
        delQuery.bindValue(":entryId", entry.id);
        if (!delQuery.exec()) {
            qWarning() << "Ошибка при удалении связей в" << tableName << ":" << delQuery.lastError().text();
            return false;
        }
        return true;
    };

    if (!deleteRelations("entry_tags")) return false;
    if (!deleteRelations("entry_user_activities")) return false;
    if (!deleteRelations("entry_user_emotions")) return false;

    // 4) Вставляем новые связи
    auto insertRelations = [&](const QString &tableName, const QString &columnName, const QVector<UserItem> &items) -> bool {
        QSqlQuery insQuery(db);
        QString sql = QString("INSERT INTO %1 (entry_id, %2) VALUES (:entryId, :itemId)").arg(tableName, columnName);
        insQuery.prepare(sql);
        for (const UserItem &item : items) {
            if (item.id <= 0) {
                qWarning() << "Пропущен недопустимый ID" << item.id << "для таблицы" << tableName;
                continue;
            }
            insQuery.bindValue(":entryId", entry.id);
            insQuery.bindValue(":itemId", item.id);
            if (!insQuery.exec()) {
                qWarning() << "Ошибка при вставке связи в" << tableName << ":" << insQuery.lastError().text();
                return false;
            }
        }
        return true;
    };

    if (!insertRelations("entry_tags", "tag_id", entry.tags)) return false;
    if (!insertRelations("entry_user_activities", "user_activity_id", entry.activities)) return false;
    if (!insertRelations("entry_user_emotions", "user_emotion_id", entry.emotions)) return false;
    if (oldFolderId != entry.folderId && oldFolderId > 0 && entry.folderId > 0) {
        QSqlQuery folderQuery(db);
        folderQuery.prepare(R"(
            UPDATE folders
            SET itemcount = GREATEST(itemcount - 1, 0)
            WHERE id = :oldFolderId
        )");
        folderQuery.bindValue(":oldFolderId", oldFolderId);
        if (!folderQuery.exec()) {
            qWarning() << "Ошибка при уменьшении itemcount в старой папке:" << folderQuery.lastError().text();
            return false;
        }

        // Увеличиваем счетчик в новой папке
        folderQuery.prepare(R"(
            UPDATE folders
            SET itemcount = itemcount + 1
            WHERE id = :newFolderId
        )");
        folderQuery.bindValue(":newFolderId", entry.folderId);
        if (!folderQuery.exec()) {
            qWarning() << "Ошибка при увеличении itemcount в новой папке:" << folderQuery.lastError().text();
            return false;
        }
    }

    return true;
}

//--------- все остальное ----------

QVector<UserItem> EntriesDatabase::getTagsForEntry(int entryId)
{
    QVector<UserItem> tags;

    QSqlQuery query;
    query.prepare(R"(
        SELECT t.id, t.name, 0 AS icon_id
        FROM entry_tags et
        JOIN user_tags t ON et.tag_id = t.id
        WHERE et.entry_id = :entryId
    )");
    query.bindValue(":entryId", entryId);

    if (query.exec()) {
        while (query.next()) {
            tags.append(UserItem{
                query.value("id").toInt(),
                query.value("icon_id").toInt(),
                query.value("name").toString()
            });
        }
    } else {
        qWarning() << "Failed to get tags for entry" << entryId << ":" << query.lastError().text();
    }

    return tags;
}

QVector<UserItem> EntriesDatabase::getActivitiesForEntry(int entryId)
{
    QVector<UserItem> activities;

    QSqlQuery query;
    query.prepare(R"(
        SELECT a.id, a.icon_id, a.icon_label
        FROM entry_user_activities eua
        JOIN user_activities a ON eua.user_activity_id = a.id
        WHERE eua.entry_id = :entryId
    )");
    query.bindValue(":entryId", entryId);

    if (query.exec()) {
        while (query.next()) {
            activities.append(UserItem{
                query.value("id").toInt(),
                query.value("icon_id").toInt(),
                query.value("icon_label").toString()
            });
        }
    } else {
        qWarning() << "Failed to get activities for entry" << entryId << ":" << query.lastError().text();
    }

    return activities;
}

QVector<UserItem> EntriesDatabase::getEmotionsForEntry(int entryId)
{
    QVector<UserItem> emotions;

    QSqlQuery query;
    query.prepare(R"(
        SELECT e.id, e.icon_id, e.icon_label
        FROM entry_user_emotions eue
        JOIN user_emotions e ON eue.user_emotion_id = e.id
        WHERE eue.entry_id = :entryId
    )");
    query.bindValue(":entryId", entryId);

    if (query.exec()) {
        while (query.next()) {
            emotions.append(UserItem{
                query.value("id").toInt(),
                query.value("icon_id").toInt(),
                query.value("icon_label").toString()
            });
        }
    } else {
        qWarning() << "Failed to get emotions for entry" << entryId << ":" << query.lastError().text();
    }

    return emotions;
}
