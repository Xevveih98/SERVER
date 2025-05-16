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

    //void extracted(QJsonArray &tagsArray, QStringList &tags);
    QHttpServerResponse handleSaveTags(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserTags(const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteTag(const QHttpServerRequest &request);

    QHttpServerResponse handleSaveActivity(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserActivity(const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteActivity(const QHttpServerRequest &request);

    QHttpServerResponse handleSaveEmotion(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserEmotions(const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteEmotion(const QHttpServerRequest &request);

};

#endif // CATEGORIESMANAGER_H
