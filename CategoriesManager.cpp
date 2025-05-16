#include "CategoriesManager.h"
#include "CategoriesDatabase.h"

CategoriesManager::CategoriesManager(QObject *parent)
    : QObject(parent)
{
}

QHttpServerResponse CategoriesManager::handleSaveTags(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in tag saving:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QJsonArray tagsArray = json.value("tags").toArray();

    if (login.isEmpty() || tagsArray.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    QStringList tags;
    for (const QJsonValue &val : tagsArray) {
        if (val.isString()) {
            tags << val.toString().trimmed();
        }
    }

    if (CategoriesDatabase::saveUserTags(login, tags)) {
        return QHttpServerResponse("Tags saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to save tags", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse CategoriesManager::handleGetUserTags(const QHttpServerRequest &request)
{
    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Теперь возвращается QList<UserItem>
    QList<CategoriesDatabase::UserItem> tags = CategoriesDatabase::getUserTags(login);

    if (tags.isEmpty()) {
        return QHttpServerResponse("No tags found", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonArray tagArray;
    for (const auto &tag : tags) {
        QJsonObject tagObj;
        tagObj["id"] = tag.id;
        tagObj["tag"] = tag.label;
        tagArray.append(tagObj);
    }

    QJsonObject response;
    response["tags"] = tagArray;

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse CategoriesManager::handleDeleteTag(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in tag deletion:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString tag = json.value("tag").toString();

    if (login.isEmpty() || tag.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (CategoriesDatabase::deleteTag(login, tag)) {
        return QHttpServerResponse("Tag deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete tag", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse CategoriesManager::handleSaveEmotion(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in emotion saving:" << parseError.errorString();
        qWarning() << "Received data:" << QString(request.body());
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString iconId = json.value("icon_id").toString();
    QString iconlabel = json.value("icon_label").toString();

    if (login.isEmpty() || iconId.isEmpty() || iconlabel.isEmpty()) {
        qWarning() << "Missing fields in request. Login:" << login << "iconId:" << iconId << "iconlabel:" << iconlabel;
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (CategoriesDatabase::saveUserEmotion(login, iconId, iconlabel)) {
        return QHttpServerResponse("Emotion saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to save emotion", QHttpServerResponse::StatusCode::InternalServerError);
    }
}


QHttpServerResponse CategoriesManager::handleGetUserEmotions(const QHttpServerRequest &request)
{
    if (request.method() != QHttpServerRequest::Method::Get) {
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    QList<CategoriesDatabase::UserItem> emotions = CategoriesDatabase::getUserEmotions(login);

    if (emotions.isEmpty()) {
        return QHttpServerResponse("No emotions found", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonArray emotionsArray;
    for (const auto &emotion : emotions) {
        QJsonObject emotionObj;
        emotionObj["id"] = emotion.id;
        emotionObj["emotion"] = emotion.label;
        emotionObj["iconId"] = emotion.iconId;
        emotionsArray.append(emotionObj);
    }

    QJsonObject response;
    response["emotions"] = emotionsArray;

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse CategoriesManager::handleDeleteEmotion(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in emotion deletion:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString emotion = json.value("emotion").toString();

    if (login.isEmpty() || emotion.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (CategoriesDatabase::deleteEmotion(login, emotion)) {
        return QHttpServerResponse("Emotion deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete emotion", QHttpServerResponse::StatusCode::InternalServerError);
    }
}


QHttpServerResponse CategoriesManager::handleSaveActivity(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in activity saving:" << parseError.errorString();
        qWarning() << "Received data:" << QString(request.body());
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString iconId = json.value("icon_id").toString();
    QString iconlabel = json.value("icon_label").toString();

    if (login.isEmpty() || iconId.isEmpty() || iconlabel.isEmpty()) {
        qWarning() << "Missing fields in request. Login:" << login << "iconId:" << iconId << "iconLabel:" << iconlabel;
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (CategoriesDatabase::saveUserActivity(login, iconId, iconlabel)) {
        return QHttpServerResponse("Activity saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to save activity", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse CategoriesManager::handleGetUserActivity(const QHttpServerRequest &request)
{
    if (request.method() != QHttpServerRequest::Method::Get) {
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    QList<CategoriesDatabase::UserItem> activities = CategoriesDatabase::getUserActivities(login);

    if (activities.isEmpty()) {
        return QHttpServerResponse("No activities found", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonArray activitiesArray;
    for (const auto &activity : activities) {
        QJsonObject activityObj;
        activityObj["id"] = activity.id;
        activityObj["activity"] = activity.label;
        activityObj["iconId"] = activity.iconId;
        activitiesArray.append(activityObj);
    }

    QJsonObject response;
    response["activities"] = activitiesArray;

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}


QHttpServerResponse CategoriesManager::handleDeleteActivity(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in activity deletion:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString activity = json.value("activity").toString();

    if (login.isEmpty() || activity.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (CategoriesDatabase::deleteActivity(login, activity)) {
        return QHttpServerResponse("Activity deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete activity", QHttpServerResponse::StatusCode::InternalServerError);
    }
}
