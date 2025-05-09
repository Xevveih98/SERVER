#ifndef CATEGORIESDATABASE_H
#define CATEGORIESDATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QCryptographicHash>

class CategoriesDatabase {

public:
    static bool saveUserTags(const QString &login, const QStringList &tags);
    static QStringList getUserTags(const QString &login);
    static bool deleteTag(const QString &login, const QString &tag);

    static bool saveUserActivity(const QString &login, const QString &iconId, const QString &iconlabel);
    static QList<QPair<QString, QString>> getUserActivities(const QString &login);
    static bool deleteActivity(const QString &login, const QString &activity);

    static bool saveUserEmotion(const QString &login, const QString &iconId, const QString &iconlabel);
    static QList<QPair<QString, QString>> getUserEmotions(const QString &login);
    static bool deleteEmotion(const QString &login, const QString &emotion);
};

#endif // CATEGORIESDATABASE_H
