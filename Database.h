#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QCryptographicHash>


#ifndef DATABASE_H
#define DATABASE_H

class Database {
public:
    static bool connect();
    static bool addUser(const QString &login, const QString &password, const QString &email);
    static bool checkUserCredentials(const QString &login, const QString &password);
    static bool changeUserPassword(const QString &email, const QString &newPassword);

private:
    static QString hashPassword(const QString &password);

};

#endif // DATABASE_H
