#include "ComputeDatabase.h"

QList<EntryUser> ComputeDatabase::getEntriesByLastMonth(const QString &login, const QString &lastMonth)
{
    QList<EntryUser> entries;

    if (login.isEmpty() || lastMonth.isEmpty()) {
        qWarning() << "Login or lastMonth is empty.";
        return entries;
    }

    QString queryStr = R"(
        SELECT e.id, e.entry_mood_id, e.entry_date
        FROM entries e
        WHERE e.user_login = ?
          AND TO_CHAR(e.entry_date, 'YYYY-MM') = ?
        ORDER BY e.entry_date ASC
    )";

    QSqlQuery query;
    if (!query.prepare(queryStr)) {
        qWarning() << "Query prepare failed:" << query.lastError().text();
        return entries;
    }

    query.addBindValue(login);
    query.addBindValue(lastMonth);  // Пример: "2025-04"

    if (!query.exec()) {
        qWarning() << "Failed to get entries by last month:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        int entryId = query.value("id").toInt();
        int moodId = query.value("entry_mood_id").toInt();
        QDate date = query.value("entry_date").toDate();

        QString emptyStr;
        QTime emptyTime;
        QVector<UserItem> emptyTags;
        QVector<UserItem> emptyActivities;
        QVector<UserItem> emptyEmotions;

        EntryUser entry(entryId, login, emptyStr, emptyStr, moodId, -1, date, emptyTime,
                        emptyTags, emptyActivities, emptyEmotions);
        entries.append(entry);
    }

    return entries;
}

QList<EntryUser> ComputeDatabase::getEntriesByCurrentMonth(const QString &login, const QString &currentMonth)
{
    QList<EntryUser> entries;

    if (login.isEmpty() || currentMonth.isEmpty()) {
        qWarning() << "Login or currentMonth is empty.";
        return entries;
    }

    QString queryStr = R"(
        SELECT e.id, e.entry_mood_id, e.entry_date
        FROM entries e
        WHERE e.user_login = ?
          AND TO_CHAR(e.entry_date, 'YYYY-MM') = ?
        ORDER BY e.entry_date ASC
    )";

    QSqlQuery query;
    if (!query.prepare(queryStr)) {
        qWarning() << "Query prepare failed:" << query.lastError().text();
        return entries;
    }

    query.addBindValue(login);
    query.addBindValue(currentMonth);  // Пример: "2025-05"

    if (!query.exec()) {
        qWarning() << "Failed to get entries by current month:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        int entryId = query.value("id").toInt();
        int moodId = query.value("entry_mood_id").toInt();
        QDate date = query.value("entry_date").toDate();

        QString emptyStr;
        QTime emptyTime;
        QVector<UserItem> emptyTags;
        QVector<UserItem> emptyActivities;
        QVector<UserItem> emptyEmotions;

        EntryUser entry(entryId, login, emptyStr, emptyStr, moodId, -1, date, emptyTime,
                        emptyTags, emptyActivities, emptyEmotions);
        entries.append(entry);
    }

    return entries;
}

