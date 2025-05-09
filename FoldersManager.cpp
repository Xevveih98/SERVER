#include "FoldersDatabase.h"
#include "FoldersManager.h"

FoldersManager::FoldersManager(QObject *parent)
    : QObject(parent)
{
}

QHttpServerResponse FoldersManager::handleSaveFolder(const QHttpServerRequest &request)
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

    if (FoldersDatabase::saveUserFolder(login, tags)) {
        return QHttpServerResponse("Tags saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to save tags", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse FoldersManager::handleGetUserFolder(const QHttpServerRequest &request)
{
    // Извлечение параметра "login" из строки запроса URL
    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    // Проверка на наличие параметра login
    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Получаем теги пользователя из базы данных
    QStringList tags = FoldersDatabase::getUserFolder(login);

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

QHttpServerResponse FoldersManager::handleDeleteFolder(const QHttpServerRequest &request)
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

    if (FoldersDatabase::deleteFolder(login, tag)) {
        return QHttpServerResponse("Tag deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete tag", QHttpServerResponse::StatusCode::InternalServerError);
    }
}
