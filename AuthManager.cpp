#include "AuthManager.h"
#include "Database.h"
#include "AuthDatabase.h"
#include <QDebug>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
}

QString AuthManager::hashPassword(const QString &password) {
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

QHttpServerResponse AuthManager::handleRegister(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        return QHttpServerResponse("Invalid JSON", QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();

    QString login = json.value("login").toString();
    QString email = json.value("email").toString();
    QString password = json.value("password").toString();

    if (login.isEmpty() || email.isEmpty() || password.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponder::StatusCode::BadRequest);
    }

    auto result = AuthDatabase::addUser(login, password, email);

    switch (result) {
    case AuthDatabase::RegisterResult::Success:
        return QHttpServerResponse("Пользователь зарегистрирован", QHttpServerResponse::StatusCode::Ok);
    case AuthDatabase::RegisterResult::UserAlreadyExists:
        return QHttpServerResponse("* Пользователь с данным логином уже существует", QHttpServerResponse::StatusCode::Conflict); // 409
    case AuthDatabase::RegisterResult::DatabaseError:
    default:
        return QHttpServerResponse("* Пользователь с такой почтой уже существует", QHttpServerResponse::StatusCode::InternalServerError);
    }
}


QHttpServerResponse AuthManager::handleLogin(const QHttpServerRequest &request)
{
    qDebug() << "Запрос на авторизацию вызван!";
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);


    QJsonObject obj = jsonDoc.object();
    const QString login = obj.value("login").toString().trimmed();
    const QString password = obj.value("password").toString().trimmed();

    QJsonObject responseObj;
    responseObj["loginError"] = "";
    responseObj["passwordError"] = "";

    // Получаем пользователя из базы
    AuthDatabase::UserInfo user = AuthDatabase::getUserInfoByLogin(login);

    if (!user.isValid) {
        responseObj["success"] = false;
        responseObj["loginError"] = "* Пользователь с таким логином не существует";
        return QHttpServerResponse("application/json", QJsonDocument(responseObj).toJson());
    }

    // Проверяем хэш пароля
    QString hashedPassword = hashPassword(password);
    if (hashedPassword != user.hashedPassword) {
        responseObj["success"] = false;
        responseObj["passwordError"] = "* Неверный пароль";
        return QHttpServerResponse("application/json", QJsonDocument(responseObj).toJson());
    }

    responseObj["success"] = true;
    responseObj["login"] = user.login;
    responseObj["email"] = user.email;
    responseObj["message"] = "Авторизация успешна";

    return QHttpServerResponse("application/json", QJsonDocument(responseObj).toJson());
}


QHttpServerResponse AuthManager::handlePasswordChange(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        return QHttpServerResponse(QJsonObject{{"status", "error"}, {"message", "Некорректный JSON"}},
                                   QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString oldPassword = json.value("oldPassword").toString();
    QString newPassword = json.value("newPassword").toString();

    if (login.isEmpty() || oldPassword.isEmpty() || newPassword.isEmpty()) {
        return QHttpServerResponse(QJsonObject{{"status", "error"}, {"message", "Заполните все поля"}},
                                   QHttpServerResponse::StatusCode::BadRequest);
    }

    QString result = AuthDatabase::changeUserPassword(login, oldPassword, newPassword);
    if (result == "ok") {
        return QHttpServerResponse(QJsonObject{{"status", "ok"}, {"message", "Пароль успешно изменён"}},
                                   QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse(QJsonObject{{"status", "error"}, {"message", result}},
                                   QHttpServerResponse::StatusCode::BadRequest);
    }
}

QHttpServerResponse AuthManager::handlePasswordRecover(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        return QHttpServerResponse(
            QJsonObject{{"status", "error"}, {"message", "Некорректный JSON"}},
            QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString email = json.value("email").toString();
    QString newPassword = json.value("newPassword").toString();

    if (email.isEmpty() || newPassword.isEmpty()) {
        return QHttpServerResponse(
            QJsonObject{{"status", "error"}, {"message", "Заполните все поля"}},
            QHttpServerResponse::StatusCode::BadRequest);
    }

    auto [userInfo, resultMessage] = AuthDatabase::recoverUserPasswordByEmail(email, newPassword);

    if (resultMessage == "ok") {
        QJsonObject response {
            {"status", "ok"},
            {"message", "Пароль успешно изменён"},
            {"login", userInfo.login},
            {"email", userInfo.email}
        };
        return QHttpServerResponse(response, QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse(
            QJsonObject{
                {"status", "error"},
                {"message", resultMessage}
            },
            QHttpServerResponse::StatusCode::BadRequest);
    }
}



QHttpServerResponse AuthManager::handleEmailChange(const QHttpServerRequest &request)
{
    qDebug() << "Запрос на смену email вызван!";

    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in email change request:" << parseError.errorString();
        return QHttpServerResponse("Неверный формат JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = jsonDoc.object();
    QString login = obj.value("login").toString().trimmed();
    QString email = obj.value("email").toString().trimmed();

    if (login.isEmpty()) {
        return QHttpServerResponse("Отсутствует логин", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (email.isEmpty()) {
        return QHttpServerResponse("Отсутствует email", QHttpServerResponse::StatusCode::BadRequest);
    }

    bool changed = AuthDatabase::changeUserEmail(login, email);
    if (changed) {
        qInfo() << "Email успешно изменён для пользователя:" << login;

        QJsonObject responseObj;
        responseObj["email"] = email;
        return QHttpServerResponse("application/json", QJsonDocument(responseObj).toJson(),
                                   QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Смена email не удалась для пользователя:" << login;
        return QHttpServerResponse("Не удалось изменить email", QHttpServerResponse::StatusCode::InternalServerError);
    }
}


QHttpServerResponse AuthManager::handleLoginToDelete(const QHttpServerRequest &request)
{
    // Попытка разобрать JSON из тела запроса
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received in delete user:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();

    if (login.isEmpty()) {
        return QHttpServerResponse("Missing login", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (deleteUserFromDatabase(login)) {
        qInfo() << "User" << login << "deleted successfully.";
        return QHttpServerResponse("User deleted successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Failed to delete user:" << login;
        return QHttpServerResponse("Failed to delete user", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

bool AuthManager::deleteUserFromDatabase(const QString &login)
{
    return AuthDatabase::deleteUserByLogin(login);  // Вызов метода в Database
}
