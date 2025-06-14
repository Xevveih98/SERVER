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
    const int folderId = query.queryItemValue("folderId").toInt();
    const int year = query.queryItemValue("year").toInt();
    const int month = query.queryItemValue("month").toInt();

    qDebug() << "Extracted params - login:" << login
             << "| folderId:" << folderId
             << "| year:" << year
             << "| month:" << month;

    if (login.isEmpty() || folderId == 0 || year == 0 || month == 0) {
        return QHttpServerResponse("Missing or invalid parameters", QHttpServerResponse::StatusCode::BadRequest);
    }

    QList<EntryUser> entries = EntriesDatabase::getUserEntries(login, folderId, year, month);
    qDebug() << "Entries fetched from DB:" << entries.size();

    if (entries.isEmpty()) {
        QJsonObject response;
        response["entries"] = QJsonArray();
        return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
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

        QJsonArray tagsArray;
        for (const auto &tag : entry.tags) {
            QJsonObject tagObj;
            tagObj["id"] = tag.id;
            tagObj["iconId"] = tag.iconId;
            tagObj["label"] = tag.label;
            tagsArray.append(tagObj);
        }
        entryObj["tags"] = tagsArray;

        QJsonArray activitiesArray;
        for (const auto &act : entry.activities) {
            QJsonObject actObj;
            actObj["id"] = act.id;
            actObj["iconId"] = act.iconId;
            actObj["label"] = act.label;
            activitiesArray.append(actObj);
        }
        entryObj["activities"] = activitiesArray;

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

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse EntriesManager::handleSearchEntriesByKeywords(const QHttpServerRequest &request)
{
    qDebug() << "[/searchentriesbywords] Request received.";

    if (request.method() != QHttpServerRequest::Method::Post) {
        qWarning() << "Invalid method:" << request.method();
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    QJsonParseError parseError;
    const QByteArray body = request.body();
    qDebug() << "Raw body:" << QString::fromUtf8(body);

    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    const QString login = obj.value("login").toString();
    const QJsonArray keywordsJson = obj.value("keywords").toArray();

    if (login.isEmpty() || keywordsJson.isEmpty()) {
        qWarning() << "Missing login or keywords. login:" << login << ", keywords:" << keywordsJson;
        return QHttpServerResponse("Missing login or keywords", QHttpServerResponse::StatusCode::BadRequest);
    }

    QStringList keywords;
    for (const QJsonValue &value : keywordsJson) {
        const QString word = value.toString().trimmed();
        if (!word.isEmpty()) {
            keywords << word;
        }
    }

    qDebug() << "Parsed login:" << login;
    qDebug() << "Parsed keywords:" << keywords;

    QList<EntryUser> entries = EntriesDatabase::getUserEntriesByKeywords(login, keywords);
    qDebug() << "Found entries count:" << entries.size();

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

        QJsonArray tagsArray;
        for (const auto &tag : entry.tags) {
            QJsonObject tagObj;
            tagObj["id"] = tag.id;
            tagObj["iconId"] = tag.iconId;
            tagObj["label"] = tag.label;
            tagsArray.append(tagObj);
        }
        entryObj["tags"] = tagsArray;

        QJsonArray activitiesArray;
        for (const auto &act : entry.activities) {
            QJsonObject actObj;
            actObj["id"] = act.id;
            actObj["iconId"] = act.iconId;
            actObj["label"] = act.label;
            activitiesArray.append(actObj);
        }
        entryObj["activities"] = activitiesArray;

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

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse EntriesManager::handleSearchEntriesByTags(const QHttpServerRequest &request)
{
    qDebug() << "[/searchentriesbytags] Request received.";

    if (request.method() != QHttpServerRequest::Method::Post) {
        qWarning() << "Invalid method:" << request.method();
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    QJsonParseError parseError;
    const QByteArray body = request.body();
    qDebug() << "Raw body:" << QString::fromUtf8(body);

    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    const QString login = obj.value("login").toString();
    const QJsonArray tagIdsJson = obj.value("tagIds").toArray();
    const QJsonArray emotionIdsJson = obj.value("emotionIds").toArray();
    const QJsonArray activityIdsJson = obj.value("activityIds").toArray();

    if (login.isEmpty()) {
        qWarning() << "Missing login or tag_ids. login:" << login;
        return QHttpServerResponse("Missing login or what", QHttpServerResponse::StatusCode::BadRequest);
    }

    QList<int> tagIds;
    for (const QJsonValue &value : tagIdsJson) {
        if (value.isDouble()) {
            tagIds.append(value.toInt());
        }
    }
    QList<int> emotionIds;
    for (const QJsonValue &value : emotionIdsJson) {
        if (value.isDouble()) {
            emotionIds.append(value.toInt());
        }
    }
    QList<int> axtivityIds;
    for (const QJsonValue &value : activityIdsJson) {
        if (value.isDouble()) {
            axtivityIds.append(value.toInt());
        }
    }

    qDebug() << "Parsed login:" << login;
    qDebug() << "Parsed tag IDs:" << tagIds;
    qDebug() << "Parsed emotion IDs:" << emotionIds;
    qDebug() << "Parsed activity IDs:" << axtivityIds;

    QList<EntryUser> entries = EntriesDatabase::getUserEntriesByTags(login, tagIds, emotionIds, axtivityIds);
    qDebug() << "Found entries count:" << entries.size();

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

        QJsonArray tagsArray;
        for (const auto &tag : entry.tags) {
            QJsonObject tagObj;
            tagObj["id"] = tag.id;
            tagObj["iconId"] = tag.iconId;
            tagObj["label"] = tag.label;
            tagsArray.append(tagObj);
        }
        entryObj["tags"] = tagsArray;

        QJsonArray activitiesArray;
        for (const auto &act : entry.activities) {
            QJsonObject actObj;
            actObj["id"] = act.id;
            actObj["iconId"] = act.iconId;
            actObj["label"] = act.label;
            activitiesArray.append(actObj);
        }
        entryObj["activities"] = activitiesArray;

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

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse EntriesManager::handleSearchEntriesByDate(const QHttpServerRequest &request)
{
    qDebug() << "[/searchentriesbydate] Request received.";

    if (request.method() != QHttpServerRequest::Method::Post) {
        qWarning() << "Invalid method:" << request.method();
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    QJsonParseError parseError;
    const QByteArray body = request.body();
    qDebug() << "Raw body:" << QString::fromUtf8(body);

    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    const QString login = obj.value("login").toString();
    const QString dateStr = obj.value("date").toString();

    if (login.isEmpty() || dateStr.isEmpty()) {
        qWarning() << "Missing login or date. login:" << login << ", date:" << dateStr;
        return QHttpServerResponse("Missing login or date", QHttpServerResponse::StatusCode::BadRequest);
    }

    qDebug() << "Parsed login:" << login;
    qDebug() << "Parsed date:" << dateStr;

    QList<EntryUser> entries = EntriesDatabase::getUserEntriesByDate(login, dateStr);
    qDebug() << "Found entries count:" << entries.size();

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

        QJsonArray tagsArray;
        for (const auto &tag : entry.tags) {
            QJsonObject tagObj;
            tagObj["id"] = tag.id;
            tagObj["iconId"] = tag.iconId;
            tagObj["label"] = tag.label;
            tagsArray.append(tagObj);
        }
        entryObj["tags"] = tagsArray;

        QJsonArray activitiesArray;
        for (const auto &act : entry.activities) {
            QJsonObject actObj;
            actObj["id"] = act.id;
            actObj["iconId"] = act.iconId;
            actObj["label"] = act.label;
            activitiesArray.append(actObj);
        }
        entryObj["activities"] = activitiesArray;

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

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse EntriesManager::handleSearchEntriesMoodIdies(const QHttpServerRequest &request)
{

    if (request.method() != QHttpServerRequest::Method::Post) {
        qWarning() << "Invalid method:" << request.method();
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    QJsonParseError parseError;
    const QByteArray body = request.body();

    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    const QString login = obj.value("login").toString();
    const QString dateStr = obj.value("date").toString();

    if (login.isEmpty() || dateStr.isEmpty()) {
        qWarning() << "Missing login or date. login:" << login << ", date:" << dateStr;
        return QHttpServerResponse("Missing login or date", QHttpServerResponse::StatusCode::BadRequest);
    }

    QList<int> moodIds = EntriesDatabase::getLastMoodIdsByDate(login, dateStr);

    QJsonArray moodIdsArray;
    for (int moodId : moodIds) {
        moodIdsArray.append(moodId);
    }

    QJsonObject response;
    response["moodIds"] = moodIdsArray;
    response["date"] = dateStr;

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse EntriesManager::handleDeleteEntry(const QHttpServerRequest &request)
{
    qDebug() << "handleDeleteEntry вызван.";

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Некорректный JSON в запросе на удаление записи:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();

    const int entryId = json.value("id").toInt(-1);
    const QString login = json.value("login").toString().trimmed();

    qDebug() << "айди записи" << entryId;
    qDebug() << "логин записи" << login;

    if (entryId < 0 || login.isEmpty()) {
        qWarning() << "Отсутствует login или некорректный id в запросе.";
        return QHttpServerResponse("Missing or invalid login/id", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (EntriesDatabase::deleteUserEntry(login, entryId)) {
        qDebug() << "Запись с id" << entryId << "успешно удалена для пользователя:" << login;
        return QHttpServerResponse("Entry deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Ошибка при удалении записи с id" << entryId << "для пользователя:" << login;
        return QHttpServerResponse("Failed to delete entry", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse EntriesManager::handleUpdateEntry(const QHttpServerRequest &request)
{
    qDebug() << "handleUpdateEntry вызван.";

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Некорректный JSON в запросе на обновление записи:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString().trimmed();

    if (login.isEmpty()) {
        qWarning() << "Отсутствует login в запросе.";
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Обязательное поле id для обновления
    int entryId = json.value("id").toInt(-1);
    if (entryId <= 0) {
        qWarning() << "Неверный или отсутствующий id записи для обновления.";
        return QHttpServerResponse("Missing or invalid entry id", QHttpServerResponse::StatusCode::BadRequest);
    }

    QString title = json.value("title").toString().trimmed();
    QString content = json.value("content").toString().trimmed();
    int moodId = json.value("moodId").toInt(-1);
    int folderId = json.value("folder").toInt(-1);
    QString dateStr = json.value("date").toString();
    QString timeStr = json.value("time").toString();

    if (title.isEmpty() || content.isEmpty() || moodId < 0) {
        qWarning() << "Неверные или отсутствующие поля в записи.";
        return QHttpServerResponse("Missing or invalid fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    QDate date = parseDate(dateStr);
    QTime time = parseTime(timeStr);

    QVector<UserItem> tags = parseUserItemsArray(json.value("tags"));
    QVector<UserItem> activities = parseUserItemsArray(json.value("activities"));
    QVector<UserItem> emotions = parseUserItemsArray(json.value("emotions"));

    EntryUser entry(entryId, login, title, content, moodId, folderId, date, time, tags, activities, emotions);

    // Предположим, что updateUserEntry умеет обновлять запись по id
    if (EntriesDatabase::updateUserEntry(login, entry)) {
        qDebug() << "Запись успешно обновлена для пользователя:" << login << "id:" << entryId;
        return QHttpServerResponse("Entry updated successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Ошибка при обновлении записи для пользователя:" << login << "id:" << entryId;
        return QHttpServerResponse("Failed to update entry", QHttpServerResponse::StatusCode::InternalServerError);
    }
}
