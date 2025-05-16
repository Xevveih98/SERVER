#include "EntriesManager.h"
#include "EntriesDatabase.h"

QDate EntriesManager::parseDate(const QString &dateStr) {
    QDate d = QDate::fromString(dateStr, Qt::ISODate);
    return d.isValid() ? d : QDate::currentDate();
}

QTime EntriesManager::parseTime(const QString &timeStr) {
    QTime t = QTime::fromString(timeStr, Qt::ISODate);
    return t.isValid() ? t : QTime::currentTime();
}

QVector<UserItem> EntriesManager::parseUserItemsArray(const QJsonValue &jsonValue) {
    QVector<UserItem> items;
    if (!jsonValue.isArray())
        return items;

    for (const QJsonValue &val : jsonValue.toArray()) {
        if (val.isDouble()) {
            int id = val.toInt();
            if (id > 0) {
                items.append({ id, 0, "" });
            }
        }
    }
    return items;
}

QHttpServerResponse EntriesManager::handleSaveEntry(const QHttpServerRequest &request)
{
    qDebug() << "handleSaveEntry вызван.";

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Некорректный JSON в запросе на сохранение записи:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString().trimmed();

    if (login.isEmpty()) {
        qWarning() << "Отсутствует login в запросе.";
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    QString title = json.value("title").toString().trimmed();
    QString content = json.value("content").toString().trimmed();
    int moodId = json.value("moodId").toInt(-1);
    int folderId = json.value("folder").toInt(-1);
    QString dateStr = json.value("date").toString();
    QString timeStr = json.value("time").toString();

    if (title.isEmpty() || content.isEmpty() || moodId < 0 || folderId < 0) {
        qWarning() << "Неверные или отсутствующие поля в записи.";
        return QHttpServerResponse("Missing or invalid fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    QDate date = parseDate(dateStr);
    QTime time = parseTime(timeStr);

    QVector<UserItem> tags = parseUserItemsArray(json.value("tags"));
    QVector<UserItem> activities = parseUserItemsArray(json.value("activities"));
    QVector<UserItem> emotions = parseUserItemsArray(json.value("emotions"));

    EntryUser entry = EntryUser::fromJson(json);

    if (EntriesDatabase::saveUserEntry(login, entry)) {
        qDebug() << "Запись успешно сохранена для пользователя:" << login;
        return QHttpServerResponse("Entry saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Ошибка при сохранении записи для пользователя:" << login;
        return QHttpServerResponse("Failed to save entry", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse EntriesManager::handleGetUserEntries(const QHttpServerRequest &request)
{
    qDebug() << "Received request at /getuserentries";
    if (request.method() != QHttpServerRequest::Method::Get) {
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    qDebug() << "Extracted login parameter:" << login;

    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    QList<EntryUser> entries = EntriesDatabase::getUserEntries(login);
    qDebug() << "Entries fetched from DB:" << entries.size();

    if (entries.isEmpty()) {
        qWarning() << "No entries found for login:" << login;
        return QHttpServerResponse("No entries found", QHttpServerResponse::StatusCode::NotFound);
    }
    QJsonArray entriesArray;

    for (const EntryUser &entry : entries) {
        QJsonObject entryObj;
        entryObj["id"] = entry.id;
        entryObj["title"] = entry.title;
        entryObj["content"] = entry.content;
        entryObj["moodId"] = entry.moodId;
        entryObj["folderId"] = entry.folderId;
        entryObj["date"] = entry.date.toString(Qt::ISODate);
        entryObj["time"] = entry.time.toString("HH:mm");

        // Формируем массив tags
        QJsonArray tagsArray;
        for (const auto &tag : entry.tags) {
            QJsonObject tagObj;
            tagObj["id"] = tag.id;
            tagObj["iconId"] = tag.iconId;
            tagObj["label"] = tag.label;
            tagsArray.append(tagObj);
        }
        entryObj["tags"] = tagsArray;

        // Формируем массив activities
        QJsonArray activitiesArray;
        for (const auto &act : entry.activities) {
            QJsonObject actObj;
            actObj["id"] = act.id;
            actObj["iconId"] = act.iconId;
            actObj["label"] = act.label;
            activitiesArray.append(actObj);
        }
        entryObj["activities"] = activitiesArray;

        // Формируем массив emotions
        QJsonArray emotionsArray;
        for (const auto &emo : entry.emotions) {
            QJsonObject emoObj;
            emoObj["id"] = emo.id;
            emoObj["iconId"] = emo.iconId;
            emoObj["label"] = emo.label;
            emotionsArray.append(emoObj);
        }
        entryObj["emotions"] = emotionsArray;

        entriesArray.append(entryObj);
    }

    QJsonObject response;
    response["entries"] = entriesArray;

    // Возвращаем JSON с типом application/json
    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}
