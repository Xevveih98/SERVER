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

bool Database::changeUserEmail(const QString &login, const QString &email) {

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

bool Database::deleteUserByLogin(const QString &login)
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



QString Database::hashPassword(const QString &password) {
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);  // Хешируем с использованием SHA-256
    return QString(hash.toHex());  // Конвертируем хеш в шестнадцатеричную строку
}

bool Database::saveUserTags(const QString &login, const QStringList &tags)
{
    QSqlQuery query;
    // Получаем user_id
    query.prepare(R"(SELECT id FROM users WHERE user_login = :login)");
    query.bindValue(":login", login);
    if (!query.exec() || !query.next()) {
        qCritical() << "User not found for login:" << login;
        return false;
    }

    int userId = query.value(0).toInt();  // переместили вверх

    QSqlQuery deleteQuery;
    deleteQuery.prepare(R"(DELETE FROM user_tags WHERE user_id = :user_id)");
    deleteQuery.bindValue(":user_id", userId);
    if (!deleteQuery.exec()) {
        qWarning() << "Failed to delete old user_tags:" << deleteQuery.lastError().text();
        return false;
    }

    for (const QString &tag : tags) {
        int tagId = -1;

        // Проверка: есть ли уже такой тег
        QSqlQuery tagQuery;
        tagQuery.prepare(R"(SELECT id FROM tags WHERE name = :tag)");
        tagQuery.bindValue(":tag", tag);
        if (tagQuery.exec() && tagQuery.next()) {
            tagId = tagQuery.value(0).toInt();
        } else {
            // Вставляем новый тег
            QSqlQuery insertTag;
            insertTag.prepare(R"(INSERT INTO tags (name) VALUES (:tag) RETURNING id)");
            insertTag.bindValue(":tag", tag);
            if (!insertTag.exec() || !insertTag.next()) {
                qWarning() << "Failed to insert tag:" << tag;
                continue;
            }
            tagId = insertTag.value(0).toInt();
        }

        // Связываем user_id и tag_id
        QSqlQuery linkQuery;
        linkQuery.prepare(R"(INSERT INTO user_tags (user_id, tag_id)
                             VALUES (:user_id, :tag_id)
                             ON CONFLICT DO NOTHING)");
        linkQuery.bindValue(":user_id", userId);
        linkQuery.bindValue(":tag_id", tagId);
        if (!linkQuery.exec()) {
            qWarning() << "Failed to link user and tag:" << linkQuery.lastError().text();
        }
    }

    return true;
}

QStringList Database::getUserTags(const QString &login)
{
    QStringList tags;

    QSqlQuery query;
    query.prepare(R"(
        SELECT t.name FROM tags t
        JOIN user_tags ut ON ut.tag_id = t.id
        JOIN users u ON u.id = ut.user_id
        WHERE u.user_login = :login
    )");
    query.bindValue(":login", login);

    if (query.exec()) {
        while (query.next())
            tags << query.value(0).toString();
    } else {
        qWarning() << "Failed to fetch tags for user" << login << ":" << query.lastError().text();
    }

    return tags;
}

bool Database::deleteTag(const QString &login, const QString &tag)
{
    // Получаем user_id
    QSqlQuery query;
    query.prepare(R"(
        SELECT id FROM users WHERE user_login = :login
    )");
    query.bindValue(":login", login);
    if (!query.exec() || !query.next()) {
        qCritical() << "User not found for login:" << login;
        return false;
    }

    int userId = query.value(0).toInt();

    // Получаем tag_id
    query.prepare(R"(
        SELECT id FROM tags WHERE name = :tag
    )");
    query.bindValue(":tag", tag);
    if (!query.exec() || !query.next()) {
        qWarning() << "Tag not found:" << tag;
        return false;
    }

    int tagId = query.value(0).toInt();

    // Удаляем тег для данного пользователя
    query.prepare(R"(
        DELETE FROM user_tags WHERE user_id = :user_id AND tag_id = :tag_id
    )");
    query.bindValue(":user_id", userId);
    query.bindValue(":tag_id", tagId);
    if (!query.exec()) {
        qWarning() << "Failed to delete tag for user:" << query.lastError().text();
        return false;
    }

    return true;
}
