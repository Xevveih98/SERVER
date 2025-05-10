#include "AuthManager.h"
#include "Database.h"
#include "AuthDatabase.h"
#include <QDebug>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
}

QHttpServerResponse AuthManager::handleRegister(const QHttpServerRequest &request)
{
    // Попытка разобрать JSON из тела запроса
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();

    QString login = json.value("login").toString();
    QString email = json.value("email").toString();
    QString password = json.value("password").toString();

    if (login.isEmpty() || email.isEmpty() || password.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponder::StatusCode::BadRequest);
    }

    if (AuthDatabase::addUser(login, password, email)) {
        return QHttpServerResponse("User registered", QHttpServerResponse::StatusCode::Ok);
    } else {
        return QHttpServerResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse AuthManager::handleLogin(const QHttpServerRequest &request)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received in login:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    const QString login = json["login"].toString();
    const QString email = json.value("email").toString();
    const QString password = json["password"].toString();

    if (login.isEmpty() || email.isEmpty() || password.isEmpty()) {
        return QHttpServerResponse("Missing login or password", QHttpServerResponder::StatusCode::BadRequest);
    }

    bool success = AuthDatabase::checkUserCredentials(login, password, email);

    if (success) {
        qInfo() << "User" << login << "authenticated successfully.";
        return QHttpServerResponse("Login successful", QHttpServerResponder::StatusCode::Ok);
    } else {
        qWarning() << "Login failed for user:" << login;
        return QHttpServerResponse("Invalid credentials", QHttpServerResponder::StatusCode::Unauthorized);
    }
}

QHttpServerResponse AuthManager::handlePasswordChange(const QHttpServerRequest &request)
{
    // Попытка разобрать JSON из тела запроса
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in password change:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString email = json.value("email").toString();
    QString password = json.value("password").toString();
    QString passwordCheck = json.value("password_check").toString();

    if (email.isEmpty() || password.isEmpty() || passwordCheck.isEmpty()) {
        return QHttpServerResponse("Missing fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (password != passwordCheck) {
        return QHttpServerResponse("Passwords do not match", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (AuthDatabase::changeUserPassword(email, password)) {
        qInfo() << "Password successfully changed for email:" << email;
        return QHttpServerResponse("Password changed successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Password change failed for email:" << email;
        return QHttpServerResponse("Password change failed", QHttpServerResponse::StatusCode::InternalServerError);
    }
}

QHttpServerResponse AuthManager::handleEmailChange(const QHttpServerRequest &request)
{
    // Попытка разобрать JSON из тела запроса
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON in Email change:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();
    QString login = json.value("login").toString();
    QString email = json.value("email").toString();

    if (email.isEmpty() || login.isEmpty()) {
        return QHttpServerResponse("Missing field", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (AuthDatabase::changeUserEmail(login, email)) {
        qInfo() << "Email successfully changed for user:" << login;
        return QHttpServerResponse("Email changed successfully", QHttpServerResponse::StatusCode::Ok);
    } else {
        qWarning() << "Email change failed for user:" << login;
        return QHttpServerResponse("Email change failed", QHttpServerResponse::StatusCode::InternalServerError);
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
