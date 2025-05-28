#ifndef AUTHDATABASE_H
#define AUTHDATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QCryptographicHash>

class AuthDatabase {
public:
    enum class RegisterResult {
        Success,
        UserAlreadyExists,
        DatabaseError
    };

    struct UserInfo {
        QString login;
        QString hashedPassword;
        QString email;
        bool isValid = false;
    };

    static RegisterResult addUser(const QString &login, const QString &password, const QString &email);
    static UserInfo getUserInfoByLogin(const QString &login);
    static QString changeUserPassword(const QString &lgoin, const QString &oldPassword, const QString &newPassword);
    static std::pair<AuthDatabase::UserInfo, QString> recoverUserPasswordByEmail(const QString &email, const QString &newPassword);
    static bool changeUserEmail(const QString &login, const QString &email);
    static bool deleteUserByLogin(const QString &login);

private:
    static QString hashPassword(const QString &password);
};

#endif // AUTHDATABASE_H
