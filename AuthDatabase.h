#ifndef AUTHDATABASE_H
#define AUTHDATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QCryptographicHash>

class AuthDatabase {

public:
    static bool addUser(const QString &login, const QString &password, const QString &email);
    static bool checkUserCredentials(const QString &login, const QString &password, const QString &email);
    static bool changeUserPassword(const QString &email, const QString &newPassword);
    static bool changeUserEmail(const QString &login, const QString &email);
    static bool deleteUserByLogin(const QString &login);

private:
    static QString hashPassword(const QString &password);

};

#endif // AUTHDATABASE_H
