#include "AuthDatabase.h"

bool AuthDatabase::addUser(const QString &login, const QString &password, const QString &email) {
    QString hashedPassword = hashPassword(password);  // Хешируем пароль

    qDebug() << "Hashed password before insertion:" << hashedPassword;

    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO users (user_login, user_email, user_passhach)
        VALUES (:login, :email, :password)
    )");
    query.bindValue(":login", login);
    query.bindValue(":email", email);
    query.bindValue(":password", hashedPassword);

    if (!query.exec()) {
        qCritical() << "Failed to insert user:" << query.lastError().text();
        return false;
    }

    QSqlQuery folderQuery;
    folderQuery.prepare(R"(
        INSERT INTO folders (name, user_login)
        VALUES (:name, :login)
    )");
    folderQuery.bindValue(":name", "Главная");
    folderQuery.bindValue(":login", login);

    if (!folderQuery.exec()) {
        qCritical() << "Failed to insert default folder:" << folderQuery.lastError().text();
        return false;
    }

    qInfo() << "User and default folder added successfully:" << login;
    return true;
}


bool AuthDatabase::checkUserCredentials(const QString &login, const QString &password, const QString &email) {
    QString hashedPassword = hashPassword(password);  // Хешируем введенный пароль
    QSqlQuery query;
    query.prepare(R"(
        SELECT COUNT(*) FROM users
        WHERE user_login = :login AND user_email = :email AND user_passhach = :password
    )");
    query.bindValue(":login", login);
    query.bindValue(":email", email);
    query.bindValue(":password", hashedPassword);

    if (!query.exec()) {
        qCritical() << "Failed to check credentials:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        int count = query.value(0).toInt();
        return count > 0;
    }

    return false;
}

bool AuthDatabase::changeUserPassword(const QString &email, const QString &newPassword) {
    QString hashedPassword = hashPassword(newPassword);  // Хешируем новый пароль
    QSqlQuery query;
    query.prepare(R"(UPDATE users SET user_passhach = :password WHERE user_email = :email)");
    query.bindValue(":password", hashedPassword);
    query.bindValue(":email", email);

    if (!query.exec()) {
        qCritical() << "Failed to change password for" << email << ":" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No user found with email:" << email;
        return false;
    }

    qInfo() << "Password updated for user:" << email;
    return true;
}

bool AuthDatabase::changeUserEmail(const QString &login, const QString &email) {

    QSqlQuery query;
    query.prepare(R"(UPDATE users SET user_email = :email WHERE user_login = :login)");
    query.bindValue(":email", email);
    query.bindValue(":login", login);

    if (!query.exec()) {
        qCritical() << "Failed to change email for" << login << ":" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No user found with login:" << login;
        return false;
    }

    qInfo() << "Email updated for user:" << login;
    return true;
}

bool AuthDatabase::deleteUserByLogin(const QString &login)
{
    QSqlQuery query;

    // SQL запрос для удаления пользователя по логину
    QString deleteQuery = QString("DELETE FROM users WHERE user_login = '%1'").arg(login);

    if (!query.exec(deleteQuery)) {
        qCritical() << "Failed to delete user:" << query.lastError().text();
        return false;
    }

    qInfo() << "User with login" << login << "deleted successfully.";
    return true;
}



QString AuthDatabase::hashPassword(const QString &password) {
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);  // Хешируем с использованием SHA-256
    return QString(hash.toHex());  // Конвертируем хеш в шестнадцатеричную строку
}
