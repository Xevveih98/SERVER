#ifndef TODOMANAGER_H
#define TODOMANAGER_H

#include <QObject>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

class TodoManager : public QObject
{
    Q_OBJECT
public:
    explicit TodoManager(QObject *parent = nullptr);

    QHttpServerResponse handleSaveTodo(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserTodoos(const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteTodo(const QHttpServerRequest &request);
};

#endif // TODOMANAGER_H
