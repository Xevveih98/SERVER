#include "CategoriesManager.h"
#include "Database.h"

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

    if (Database::saveUserTags(login, tags)) {
        return QHttpServerResponse("Tags saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to save tags", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse CategoriesManager::handleGetUserTags(const QHttpServerRequest &request)
{
    // Извлечение параметра "login" из строки запроса URL
    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    // Проверка на наличие параметра login
    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Получаем теги пользователя из базы данных
    QStringList tags = Database::getUserTags(login);

    // Создаем JSON массив с тегами
    QJsonArray tagArray;
    for (const QString &tag : tags) {
        tagArray.append(tag);
    }

    // Формируем JSON ответ
    QJsonObject response;
    response["tags"] = tagArray;

    // Возвращаем ответ с тегами в формате JSON
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

    if (Database::deleteTag(login, tag)) {
        return QHttpServerResponse("Tag deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete tag", QHttpServerResponse::StatusCode::InternalServerError);
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

    if (Database::saveUserActivity(login, iconId, iconlabel)) {
        return QHttpServerResponse("Activity saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to save activity", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse CategoriesManager::handleGetUserActivity(const QHttpServerRequest &request)
{
    qDebug() << "Received request at /getuseractivity";

    if (request.method() != QHttpServerRequest::Method::Get) {
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    qDebug() << "Extracted login parameter:" << login;

    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Получаем все активности пользователя из базы данных
    QList<QPair<QString, QString>> activities = Database::getUserActivities(login);
    qDebug() << "Activities fetched from DB:" << activities;


    if (activities.isEmpty()) {
        qWarning() << "No activity found for login:" << login;
        return QHttpServerResponse("No activity found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Формируем JSON-ответ с массивом "activities"
    QJsonObject response;
    QJsonArray activitiesArray;

    for (const auto &activity : activities) {
        QJsonObject activityObj;
        activityObj["activity"] = activity.second;
        activityObj["iconId"] = activity.first;

        activitiesArray.append(activityObj);
    }

    qDebug() << "Activities fetched from DB:" << activitiesArray;


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

    if (Database::deleteActivity(login, activity)) {
        return QHttpServerResponse("Activity deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete activity", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

