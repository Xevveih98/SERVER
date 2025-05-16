#ifndef CATEGORIESDATABASE_H
#define CATEGORIESDATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QCryptographicHash>

class CategoriesDatabase {

public:
    struct UserItem {
        int id;
        int iconId;
        QString label;
    };

public:
    static bool saveUserTags(const QString &login, const QStringList &tags);
    static QList<UserItem> getUserTags(const QString &login);
    static bool deleteTag(const QString &login, const QString &tag);

    static bool saveUserActivity(const QString &login, const QString &iconId, const QString &iconlabel);
    static QList<UserItem> getUserActivities(const QString &login);
    static bool deleteActivity(const QString &login, const QString &activity);

    static bool saveUserEmotion(const QString &login, const QString &iconId, const QString &iconlabel);
    static QList<UserItem> getUserEmotions(const QString &login);
    static bool deleteEmotion(const QString &login, const QString &emotion);
};

#endif // CATEGORIESDATABASE_H
