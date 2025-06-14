#ifndef ENTRIESDATABASE_H
#define ENTRIESDATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QString>
#include "EntryUser.h"

class EntriesDatabase
{
public:

    static bool saveUserEntry(const QString &login, const EntryUser &entry);
    static bool deleteUserEntry(const QString &login, int entryId);
    static bool updateUserEntry(const QString &login, const EntryUser &entry);
    static QList<EntryUser> getUserEntries(const QString &login, int folderId, int year, int month);
    static QList<EntryUser> getUserEntriesByKeywords(const QString &login, const QStringList &keywords);
    static QList<EntryUser> getUserEntriesByTags(const QString &login, const QList<int> &tagIds, const QList<int> &emotionIds, const QList<int> &activityIds);
    static QList<EntryUser> getUserEntriesByDate(const QString &login, const QString &dateStr);
    static QList<int> getLastMoodIdsByDate(const QString &login, const QString &dateStr);


private:
    static QVector<UserItem> getTagsForEntry(int entryId);
    static QVector<UserItem> getActivitiesForEntry(int entryId);
    static QVector<UserItem> getEmotionsForEntry(int entryId);
};

#endif // ENTRIESDATABASE_H
