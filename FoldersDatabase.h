#ifndef FOLDERSDATABASE_H
#define FOLDERSDATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QCryptographicHash>

class FoldersDatabase {

public:
    static bool saveUserFolder(const QString &login, const QStringList &folders);
    static QList<QPair<QString, QString>> getUserFolders(const QString &login);
    static bool deleteFolder(const QString &login, const QString &folder);
};

#endif // FOLDERSDATABASE_H
