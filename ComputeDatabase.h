#ifndef COMPUTEDATABASE_H
#define COMPUTEDATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QString>
#include "EntryUser.h"

class ComputeDatabase
{
public:
    static QList<EntryUser> getEntriesByLastMonth(const QString &login, const QString &lastMonth);
    static QList<EntryUser> getEntriesByCurrentMonth(const QString &login, const QString &currentMonth);

};

#endif // COMPUTEDATABASE_H
