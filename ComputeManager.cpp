#include "ComputeManager.h"
#include "ComputeDatabase.h"

QHttpServerResponse ComputeManager::handleLoadEntriesByMonth(const QHttpServerRequest &request)
{
    qDebug() << "Запрос на загрузку записей за (прошлый и текущий) месяц вызван!.";

    if (request.method() != QHttpServerRequest::Method::Post) {
        qWarning() << "Invalid method:" << request.method();
        return QHttpServerResponse("Invalid method", QHttpServerResponse::StatusCode::MethodNotAllowed);
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    const QString login = obj.value("login").toString();
    const QString lastMonth = obj.value("lastMonth").toString();
    const QString currentMonth = obj.value("currentMonth").toString();

    if (login.isEmpty() || lastMonth.isEmpty() || currentMonth.isEmpty()) {
        qWarning() << "Missing login or month values.";
        return QHttpServerResponse("Missing login or month", QHttpServerResponse::StatusCode::BadRequest);
    }

    qDebug() << " | Загрузка для пользователя:" << login;
    qDebug() << " | Прошлый месяц:" << lastMonth << ", текущий месяц:" << currentMonth;

    QList<EntryUser> entriesLast = ComputeDatabase::getEntriesByLastMonth(login, lastMonth);
    QList<EntryUser> entriesCurrent = ComputeDatabase::getEntriesByCurrentMonth(login, currentMonth);

    QJsonArray lastArray;
    for (const EntryUser &entry : entriesLast) {
        QJsonObject obj;
        obj["id"] = entry.id;
        obj["moodId"] = entry.moodId;
        obj["date"] = entry.date.toString(Qt::ISODate);
        lastArray.append(obj);
    }

    QJsonArray currentArray;
    for (const EntryUser &entry : entriesCurrent) {
        QJsonObject obj;
        obj["id"] = entry.id;
        obj["moodId"] = entry.moodId;
        obj["date"] = entry.date.toString(Qt::ISODate);
        currentArray.append(obj);
    }

    QJsonObject response;
    response["lastMonthEntries"] = lastArray;
    response["currentMonthEntries"] = currentArray;

    return QHttpServerResponse("application/json", QJsonDocument(response).toJson());
}
