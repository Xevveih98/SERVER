#pragma once

#include <QString>
#include <QVector>
#include <QDate>
#include <QTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

struct UserItem {
    int id = 0;
    int iconId = 0;
    QString label;

    static UserItem fromJson(const QJsonValue &val) {
        UserItem item;
        if (val.isObject()) {
            QJsonObject obj = val.toObject();
            item.id = obj.value("id").toInt();
            item.iconId = obj.value("iconId").toInt();
            item.label = obj.value("label").toString();
        } else if (val.isDouble()) {
            item.id = val.toInt();
        }
        return item;
    }
};

class EntryUser {
public:
    int id = 0;
    QString userLogin;
    QString title;
    QString content;
    int moodId = -1;
    int folderId = -1;
    QDate date;
    QTime time;
    QVector<UserItem> tags;
    QVector<UserItem> activities;
    QVector<UserItem> emotions;

    EntryUser() = default;

    EntryUser(int id,
              const QString &userLogin,
              const QString &title,
              const QString &content,
              int moodId,
              int folderId,
              const QDate &date,
              const QTime &time,
              const QVector<UserItem> &tags,
              const QVector<UserItem> &activities,
              const QVector<UserItem> &emotions)
        : id(id),
        userLogin(userLogin),
        title(title),
        content(content),
        moodId(moodId),
        folderId(folderId),
        date(date),
        time(time),
        tags(tags),
        activities(activities),
        emotions(emotions)
    {}

    static EntryUser fromJson(const QJsonObject &json) {
        EntryUser e;
        e.userLogin = json.value("login").toString();
        e.title = json.value("title").toString();
        e.content = json.value("content").toString();
        e.moodId = json.value("moodId").toInt();
        e.folderId = json.value("folder").toInt();
        e.date = QDate::fromString(json.value("date").toString(), Qt::ISODate);
        e.time = QTime::fromString(json.value("time").toString(), Qt::ISODate);

        auto parseUserItemArray = [](const QJsonValue &val) -> QVector<UserItem> {
            QVector<UserItem> result;
            if (!val.isArray()) return result;
            for (auto v : val.toArray())
                result.append(UserItem::fromJson(v));
            return result;
        };

        e.tags = parseUserItemArray(json.value("tags"));
        e.activities = parseUserItemArray(json.value("activities"));
        e.emotions = parseUserItemArray(json.value("emotions"));

        return e;
    }
};


