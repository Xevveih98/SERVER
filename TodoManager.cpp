#include "TodoDatabase.h"
#include "TodoManager.h"

TodoManager::TodoManager(QObject *parent)
    : QObject(parent)
{
}

QHttpServerResponse TodoManager::handleSaveTodo(const QHttpServerRequest &request)
{
    qDebug() << "Вызов сохранения задачи на стороне сервера.";

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Некорректный JSON в запросе на сохранение задачи:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QJsonValue todoValue = json.value("todo");

    if (login.isEmpty() || !todoValue.isString()) {
        qWarning() << "Отсутствуют необходимые поля или неверный формат todo.";
        return QHttpServerResponse("Missing or invalid fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    QString name = todoValue.toString();
    qDebug() << "Пользователь:" << login << " | Задача:" << name;

    if (TodoDatabase::saveUserTodo(login, name)) {
        qDebug() << "Задача успешно сохранена для пользователя:" << login;
        return QHttpServerResponse("Todo saved successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Не удалось сохранить задачу для пользователя:" << login;
        return QHttpServerResponse("Failed to save todo", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse TodoManager::handleGetUserTodoos(const QHttpServerRequest &request)
{
    qDebug() << "Вызов выгрузки списка задач на стороне сервера.";

    if (request.method() != QHttpServerRequest::Method::Get) {
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    const QUrlQuery query(request.url());
    const QString login = query.queryItemValue("login");

    qDebug() << "Extracted login parameter:" << login;

    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    QStringList todoos = TodoDatabase::getUserTodoos(login);
    qDebug() << "Список задач полученный из базы данных:" << todoos;

    if (todoos.isEmpty()) {
        qWarning() << "Задачи не найдены для пользователя:" << login;
        return QHttpServerResponse("Задачи не найдены. Список пустой", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonObject response;
    QJsonArray todoosArray;
    for (const QString &todo : todoos) {
        todoosArray.append(todo);
    }
    qDebug() << "Готовый к отправке клиенту список задач:" << todoosArray;
    response["todoos"] = todoosArray;
    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}

QHttpServerResponse TodoManager::handleDeleteTodo(const QHttpServerRequest &request)
{
    qDebug() << "Вызов удаления задачи на стороне сервера.";

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in folder deletion:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString name = json.value("todo").toString();

    if (login.isEmpty() || name.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (TodoDatabase::deleteTodo(login, name)) {
        return QHttpServerResponse("Todo deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Failed to delete todo", QHttpServerResponse::StatusCode::InternalServerError);
    }
}
