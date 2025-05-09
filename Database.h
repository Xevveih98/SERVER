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

};

#endif // DATABASE_H
