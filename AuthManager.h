#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>

class AuthManager : public QObject
{
    Q_OBJECT
public:
    explicit AuthManager(QObject *parent = nullptr);

    QHttpServerResponse handleRegister(const QHttpServerRequest &request);
    QHttpServerResponse handleLogin(const QHttpServerRequest &request);
    QHttpServerResponse handlePasswordChange(const QHttpServerRequest &request);
    QHttpServerResponse handlePasswordRecover(const QHttpServerRequest &request);
    QHttpServerResponse handleEmailChange(const QHttpServerRequest &request);
    QHttpServerResponse handleLoginToDelete(const QHttpServerRequest &request);

private:
    bool deleteUserFromDatabase(const QString &login);
    static QString hashPassword(const QString &password);

};

#endif // AUTHMANAGER_H
