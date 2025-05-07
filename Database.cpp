#include "Database.h"


bool Database::connect() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("MindTraceMainDB");
    db.setUserName("postgres");  // замените на нужное имя пользователя
    db.setPassword("123");

    if (!db.open()) {
        qCritical() << "Failed to connect to database:" << db.lastError().text();
        return false;
    }

    qInfo() << "Successfully connected to database.";
    return true;
}

bool Database::addUser(const QString &login, const QString &password, const QString &email) {
    QString hashedPassword = hashPassword(password);  // Хешируем пароль

    qDebug() << "Hashed password before insertion:" << hashedPassword;  // Выводим хеш для проверки

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

    qInfo() << "User added successfully:" << login;
    return true;
}

bool Database::checkUserCredentials(const QString &login, const QString &password) {
    QString hashedPassword = hashPassword(password);  // Хешируем введенный пароль
    QSqlQuery query;
    query.prepare(R"(
        SELECT COUNT(*) FROM users
        WHERE user_login = :login AND user_passhach = :password
    )");
    query.bindValue(":login", login);
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

bool Database::changeUserPassword(const QString &email, const QString &newPassword) {
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

QString Database::hashPassword(const QString &password) {
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);  // Хешируем с использованием SHA-256
    return QString(hash.toHex());  // Конвертируем хеш в шестнадцатеричную строку
}
