#ifndef FOLDERSMANAGER_H
#define FOLDERSMANAGER_H

#include <QObject>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

class FoldersManager : public QObject
{
    Q_OBJECT
public:
    explicit FoldersManager(QObject *parent = nullptr);

    void extracted(QJsonArray &foldersArray, QStringList &folders);
    QHttpServerResponse handleSaveFolder(const QHttpServerRequest &request);
    QHttpServerResponse handleGetUserFolder(const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteFolder(const QHttpServerRequest &request);
};


#endif // FOLDERSMANAGER_H
