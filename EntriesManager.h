#ifndef ENTRIESMANAGER_H
#define ENTRIESMANAGER_H

#pragma once

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QVector>
#include <QDate>
#include <QTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "EntryUser.h"

class EntriesManager
{
public:
    EntriesManager() = default;

    // Основной метод — обработчик запроса
    QHttpServerResponse handleSaveEntry(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserEntries(const QHttpServerRequest &request);
    QHttpServerResponse handleSearchEntriesByKeywords(const QHttpServerRequest &request);

private:

    static QVector<UserItem> parseUserItemsArray(const QJsonValue &jsonValue);
    static QDate parseDate(const QString &dateStr);
    static QTime parseTime(const QString &timeStr);
};


#endif // ENTRIESMANAGER_H
