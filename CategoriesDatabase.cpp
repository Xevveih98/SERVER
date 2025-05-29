#include "CategoriesDatabase.h"

bool CategoriesDatabase::saveUserTag(const QString &login, const QString &tag, QString &errorMessage)
{
    if (login.trimmed().isEmpty() || tag.trimmed().isEmpty()) {
        errorMessage = "Login или тег не могут быть пустыми";
        return false;
    }

    QSqlQuery checkQuery;
    checkQuery.prepare(R"(
        SELECT 1 FROM user_tags WHERE user_login = :login AND name = :tag
    )");
    checkQuery.bindValue(":login", login.trimmed());
    checkQuery.bindValue(":tag", tag.trimmed());

    if (!checkQuery.exec()) {
        errorMessage = "Ошибка проверки существующего тега: " + checkQuery.lastError().text();
        return false;
    }

    if (checkQuery.next()) {
        errorMessage = "* Такой тег уже существует";
        return false;
    }
    QSqlQuery insertQuery;
    insertQuery.prepare(R"(
        INSERT INTO user_tags (name, user_login)
        VALUES (:name, :login)
    )");
    insertQuery.bindValue(":name", tag.trimmed());
    insertQuery.bindValue(":login", login.trimmed());

    if (!insertQuery.exec()) {
        errorMessage = "Ошибка вставки тега: " + insertQuery.lastError().text();
        return false;
    }

    return true;
}

QList<CategoriesDatabase::UserItem> CategoriesDatabase::getUserTags(const QString &login)
{
    QList<UserItem> tags;

    QSqlQuery query;
    query.prepare(R"(
        SELECT id, name
        FROM user_tags
        WHERE user_login = :login
    )");
    query.bindValue(":login", login.trimmed());

    if (query.exec()) {
        while (query.next()) {
            UserItem item;
            item.id = query.value("id").toInt();
            item.label = query.value("name").toString().trimmed();
            tags.append(item);
        }
    } else {
        qWarning() << "Failed to fetch tags for user" << login << ":" << query.lastError().text();
    }

    return tags;
}

bool CategoriesDatabase::deleteTag(const QString &login, const QString &tag)
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

bool CategoriesDatabase::saveUserActivity(const QString &login, const QString &iconId, const QString &iconLabel)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO user_activities (user_login, icon_id, icon_label)
        VALUES (:login, :icon_id, :icon_label)
        ON CONFLICT (user_login, icon_label) DO NOTHING
    )");
    query.bindValue(":login", login.trimmed());
    query.bindValue(":icon_id", iconId.toInt());
    query.bindValue(":icon_label", iconLabel.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to save user activity:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<CategoriesDatabase::UserItem> CategoriesDatabase::getUserActivities(const QString &login)
{
    QList<UserItem> activities;

    QSqlQuery query;
    query.prepare(R"(
        SELECT id, icon_id, icon_label
        FROM user_activities
        WHERE user_login = :login
        ORDER BY id DESC
    )");
    query.bindValue(":login", login);

    if (query.exec()) {
        while (query.next()) {
            UserItem item;
            item.id = query.value("id").toInt();
            item.iconId = query.value("icon_id").toInt();
            item.label = query.value("icon_label").toString().trimmed();
            activities.append(item);
        }
    } else {
        qWarning() << "Failed to get user activities:" << query.lastError().text();
    }

    return activities;
}


bool CategoriesDatabase::deleteActivity(const QString &login, const QString &activity)
{
    QSqlQuery query;
    query.prepare(R"(
        DELETE FROM user_activities
        WHERE user_login = :login AND icon_label = :activity
    )");
    query.bindValue(":login", login.trimmed());
    query.bindValue(":activity", activity.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to delete activity:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No activity found to delete for user" << login << "and activity" << activity;
        return false;
    }

    return true;
}

bool CategoriesDatabase::saveUserEmotion(const QString &login, const QString &iconId, const QString &iconLabel)
{

    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO user_emotions (user_login, icon_id, icon_label)
        VALUES (:login, :icon_id, :icon_label)
        ON CONFLICT (user_login, icon_label) DO NOTHING
    )");
    query.bindValue(":login", login.trimmed());
    query.bindValue(":icon_id", iconId.toInt());
    query.bindValue(":icon_label", iconLabel);

    if (!query.exec()) {
        qWarning() << "Failed to save user emotion:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<CategoriesDatabase::UserItem> CategoriesDatabase::getUserEmotions(const QString &login)
{
    QList<UserItem> emotions;

    QSqlQuery query;
    query.prepare(R"(
        SELECT id, icon_id, icon_label
        FROM user_emotions
        WHERE user_login = :login
        ORDER BY id DESC
    )");
    query.bindValue(":login", login);

    if (query.exec()) {
        while (query.next()) {
            UserItem item;
            item.id = query.value("id").toInt();
            item.iconId = query.value("icon_id").toInt();
            item.label = query.value("icon_label").toString().trimmed();
            emotions.append(item);
        }
    } else {
        qWarning() << "Failed to get user emotions:" << query.lastError().text();
    }

    return emotions;
}

bool CategoriesDatabase::deleteEmotion(const QString &login, const QString &emotion)
{
    QSqlQuery query;
    query.prepare(R"(
        DELETE FROM user_emotions
        WHERE user_login = :login AND icon_label = :emotion
    )");
    query.bindValue(":login", login.trimmed());
    query.bindValue(":emotion", emotion.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to delete emotion:" << query.lastError().text();
        return false;
    }

    // Проверим, удалилось ли что-то
    if (query.numRowsAffected() == 0) {
        qWarning() << "No emotion found to delete for user" << login << "and emotion" << emotion;
        return false;
    }

    return true;
}
