#include "AuthDatabase.h"

AuthDatabase::RegisterResult AuthDatabase::addUser(const QString &login, const QString &password, const QString &email) {
    QString hashedPassword = hashPassword(password);

    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM users WHERE user_login = :login");
    checkQuery.bindValue(":login", login);
    if (!checkQuery.exec() || !checkQuery.next()) {
        qCritical() << "Login check failed:" << checkQuery.lastError().text();
        return RegisterResult::DatabaseError;
    }

    if (checkQuery.value(0).toInt() > 0) {
        qInfo() << "Login already exists:" << login;
        return RegisterResult::UserAlreadyExists;
    }

    // Добавление пользователя
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
        return RegisterResult::DatabaseError;
    }

    // Добавление папки
    QSqlQuery folderQuery;
    folderQuery.prepare(R"(
        INSERT INTO folders (name, user_login)
        VALUES (:name, :login)
    )");
    folderQuery.bindValue(":name", "Главная");
    folderQuery.bindValue(":login", login);

    if (!folderQuery.exec()) {
        qCritical() << "Failed to insert default folder:" << folderQuery.lastError().text();
        return RegisterResult::DatabaseError;
    }

    return RegisterResult::Success;
}

AuthDatabase::UserInfo AuthDatabase::getUserInfoByLogin(const QString &login) {
    UserInfo userInfo;
    QSqlQuery query;
    query.prepare(R"(
        SELECT user_login, user_passhach, user_email FROM users WHERE user_login = :login
    )");
    query.bindValue(":login", login);

    if (!query.exec()) {
        qCritical() << "Failed to get user info:" << query.lastError().text();
        return userInfo;
    }

    if (query.next()) {
        userInfo.login = query.value(0).toString();
        userInfo.hashedPassword = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.isValid = true;
    }

    return userInfo;
}


QString AuthDatabase::changeUserPassword(const QString &login, const QString &oldPassword, const QString &newPassword)
{
    QSqlQuery query;
    query.prepare(R"(SELECT user_passhach FROM users WHERE user_login = :login)");
    query.bindValue(":login", login);

    if (!query.exec() || !query.next()) {
        qWarning() << "User not found or query failed:" << query.lastError().text();
        return "* Пользователь не найден";
    }

    QString storedHash = query.value(0).toString();
    if (storedHash != hashPassword(oldPassword)) {
        return "* Неверный пароль";
    }

    QString hashedNewPassword = hashPassword(newPassword);
    query.prepare(R"(UPDATE users SET user_passhach = :newPassword WHERE user_login = :login)");
    query.bindValue(":newPassword", hashedNewPassword);
    query.bindValue(":login", login);

    if (!query.exec()) {
        qCritical() << "Failed to update password:" << query.lastError().text();
        return "* Ошибка при изменении пароля";
    }

    if (query.numRowsAffected() == 0) {
        return "* Пароль не обновлён";
    }

    qInfo() << "Password successfully changed for" << login;
    return "ok";
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
