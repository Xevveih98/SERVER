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
};

#endif // AUTHMANAGER_H
