#ifndef CATEGORIESMANAGER_H
#define CATEGORIESMANAGER_H

#include <QObject>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

class CategoriesManager : public QObject
{
    Q_OBJECT
public:
    explicit CategoriesManager(QObject *parent = nullptr);

    void extracted(QJsonArray &tagsArray, QStringList &tags);
    QHttpServerResponse handleSaveTags(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserTags(const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteTag(const QHttpServerRequest &request);

};

#endif // CATEGORIESMANAGER_H
