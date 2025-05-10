#include "FoldersDatabase.h"
#include "FoldersManager.h"

FoldersManager::FoldersManager(QObject *parent)
    : QObject(parent)
{
}

QHttpServerResponse FoldersManager::handleSaveFolder(const QHttpServerRequest &request)
{
    qDebug() << "handleSaveFolder вызван.";

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Некорректный JSON в запросе на сохранение папки:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    qDebug() << "JSON успешно распарсен. Объект: " << jsonDoc.object();

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QJsonValue folderValue = json.value("folder");

    // Преобразуем одиночную папку в массив папок, если она есть
    QJsonArray foldersArray;
    if (folderValue.isString()) {
        foldersArray.append(folderValue.toString());
        qDebug() << "Получена одиночная папка, преобразована в массив:" << foldersArray;
    } else {
        foldersArray = json.value("folders").toArray();  // Получаем массив папок
        qDebug() << "Получен массив папок:" << foldersArray;
    }

    qDebug() << "Получены данные из запроса:" << "login:" << login << "folders:" << foldersArray;

    if (login.isEmpty() || foldersArray.isEmpty()) {
        qWarning() << "Отсутствуют необходимые поля в запросе: login или folders.";
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    QStringList folders;
    for (const QJsonValue &val : foldersArray) {
        if (val.isString()) {
            QString folder = val.toString().trimmed();
            qDebug() << "Добавление папки:" << folder;
            folders << folder;
        }
    }

    qDebug() << "Список папок после обработки:" << folders;

    if (folders.isEmpty()) {
        qWarning() << "Нет валидных папок для сохранения.";
        return QHttpServerResponse("No valid folders", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (FoldersDatabase::saveUserFolder(login, folders)) {
        qDebug() << "Папки успешно сохранены для пользователя:" << login;
        return QHttpServerResponse("Folders saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Не удалось сохранить папки для пользователя:" << login;
        return QHttpServerResponse("Failed to save folders", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse FoldersManager::handleGetUserFolders(const QHttpServerRequest &request)
{
    qDebug() << "Received request at /getuserfolders";

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
    QList<QPair<QString, QString>> folders = FoldersDatabase::getUserFolders(login);
    qDebug() << "Folders fetched from DB:" << folders;

    if (folders.isEmpty()) {
        qWarning() << "No folder found for login:" << login;
        return QHttpServerResponse("No folder found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Формируем JSON-ответ с массивом "activities"
    QJsonObject response;
    QJsonArray foldersArray;

    for (const auto &folder : folders) {
        QJsonObject folderObj;
        folderObj["name"] = folder.first;
        folderObj["itemCount"] = folder.second;

        foldersArray.append(folderObj);
    }

    qDebug() << "Folders fetched from DB:" << foldersArray;


    response["folders"] = foldersArray;

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse FoldersManager::handleDeleteFolder(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in folder deletion:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString folder = json.value("folder").toString();  // Изменил на "folder" вместо "tag"

    if (login.isEmpty() || folder.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (FoldersDatabase::deleteFolder(login, folder)) {  // Теперь удаляем папку
        return QHttpServerResponse("Folder deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete folder", QHttpServerResponse::StatusCode::InternalServerError);
    }
}
