#ifndef COMPUTEMANAGER_H
#define COMPUTEMANAGER_H

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

class ComputeManager
{

public:
    ComputeManager() = default;
    QHttpServerResponse handleLoadEntriesByMonth(const QHttpServerRequest &request);

};

#endif // COMPUTEMANAGER_H
