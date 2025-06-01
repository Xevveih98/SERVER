#include "AuthDatabase.h"
#include <qdatetime.h>

AuthDatabase::RegisterResult AuthDatabase::addUser(const QString &login, const QString &password, const QString &email) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction:" << db.lastError().text();
        return RegisterResult::DatabaseError;
    }

    QString hashedPassword = hashPassword(password);
    QSqlQuery query;

    // Проверка существования пользователя
    query.prepare("SELECT COUNT(*) FROM users WHERE user_login = :login");
    query.bindValue(":login", login);
    if (!query.exec() || !query.next()) {
        qCritical() << "Login check failed:" << query.lastError().text();
        db.rollback();
        return RegisterResult::DatabaseError;
    }

    if (query.value(0).toInt() > 0) {
        qInfo() << "Login already exists:" << login;
        db.rollback();
        return RegisterResult::UserAlreadyExists;
    }

    // Добавление пользователя
    query.prepare(R"(
        INSERT INTO users (user_login, user_email, user_passhach)
        VALUES (:login, :email, :password)
    )");
    query.bindValue(":login", login);
    query.bindValue(":email", email);
    query.bindValue(":password", hashedPassword);

    if (!query.exec()) {
        qCritical() << "Failed to insert user:" << query.lastError().text();
        db.rollback();
        return RegisterResult::DatabaseError;
    }

    // Добавление папки
    query.prepare(R"(
        INSERT INTO folders (name, user_login)
        VALUES (:name, :login)
        RETURNING id
    )");
    query.bindValue(":name", "Главная");
    query.bindValue(":login", login);

    if (!query.exec() || !query.next()) {
        qCritical() << "Failed to insert default folder:" << query.lastError().text();
        db.rollback();
        return RegisterResult::DatabaseError;
    }

    int folderId = query.value(0).toInt();

    // Добавление первой записи
    QDate currentDate = QDate::currentDate();
    QTime currentTime = QTime::currentTime();

    query.prepare(R"(
        INSERT INTO entries (
            user_login,
            entry_title,
            entry_content,
            entry_mood_id,
            entry_folder_id,
            entry_date,
            entry_time
        ) VALUES (
            :login,
            :title,
            :content,
            :mood_id,
            :folder_id,
            :date,
            :time
        )
    )");

    query.bindValue(":login", login);
    query.bindValue(":title", "Моя первая запись");
    query.bindValue(":content",
                    "Это мой первый день в Дневнике Эмоций! Сегодня всё кажется новым и немного волнующим, "
                    "но в то же время очень тёплым и уютным. Я начинаю своё путешествие по миру чувств — "
                    "готов(а) замечать маленькие радости и учиться лучше понимать себя. Пусть этот дневник "
                    "станет местом, где я смогу хранить свои мысли, настроение и важные моменты. Вперёд к новым открытиям!");
    query.bindValue(":mood_id", 1);
    query.bindValue(":folder_id", folderId);
    query.bindValue(":date", currentDate);
    query.bindValue(":time", currentTime);

    if (!query.exec()) {
        qCritical() << "Failed to insert first entry:" << query.lastError().text();
        db.rollback();
        return RegisterResult::DatabaseError;
    }

    // Добавление задач
    QStringList todoTasks = {
        "Ознакомиться с пунктом меню \"Настройки\"",
        "Добавить свои события, впечатления и теги"
    };

    query.prepare(R"(
        INSERT INTO user_todo (user_login, name)
        VALUES (:login, :name)
    )");

    for (const auto& task : todoTasks) {
        query.bindValue(":login", login);
        query.bindValue(":name", task);
        if (!query.exec()) {
            qCritical() << "Failed to insert todo task:" << query.lastError().text();
            db.rollback();
            return RegisterResult::DatabaseError;
        }
    }

    // Добавление активностей
    QVector<QPair<int, QString>> activities = {
        {12, "моё первое событие"}
    };

    query.prepare(R"(
        INSERT INTO user_activities (user_login, icon_id, icon_label)
        VALUES (:login, :icon_id, :icon_label)
    )");

    for (const auto& activity : activities) {
        query.bindValue(":login", login);
        query.bindValue(":icon_id", activity.first);
        query.bindValue(":icon_label", activity.second);
        if (!query.exec()) {
            qCritical() << "Failed to insert activity:" << query.lastError().text();
            db.rollback();
            return RegisterResult::DatabaseError;
        }
    }

    // Добавление эмоций
    QVector<QPair<int, QString>> emotions = {
        {13, "моя вторая эмоция"},
        {14, "моя первая эмоция"}
    };

    query.prepare(R"(
        INSERT INTO user_emotions (user_login, icon_id, icon_label)
        VALUES (:login, :icon_id, :icon_label)
    )");

    for (const auto& emotion : emotions) {
        query.bindValue(":login", login);
        query.bindValue(":icon_id", emotion.first);
        query.bindValue(":icon_label", emotion.second);
        if (!query.exec()) {
            qCritical() << "Failed to insert emotion:" << query.lastError().text();
            db.rollback();
            return RegisterResult::DatabaseError;
        }
    }

    // Добавление тегов
    QVector<QString> tags = {
        {"мой_первый_тег"}
    };

    query.prepare(R"(
        INSERT INTO user_tags (user_login, name)
        VALUES (:login, :name)
    )");

    for (const auto& tag : tags) {
        query.bindValue(":login", login);
        query.bindValue(":name", tag);
        if (!query.exec()) {
            qCritical() << "Failed to insert tag:" << query.lastError().text();
            db.rollback();
            return RegisterResult::DatabaseError;
        }
    }

    if (!db.commit()) {
        qCritical() << "Failed to commit transaction:" << db.lastError().text();
        db.rollback();
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

std::pair<AuthDatabase::UserInfo, QString> AuthDatabase::recoverUserPasswordByEmail(const QString &email, const QString &newPassword)
{
    UserInfo userInfo;
    QSqlQuery query;
    query.prepare(R"(SELECT id, user_login FROM users WHERE user_email = :email)");
    query.bindValue(":email", email);

    if (!query.exec()) {
        qCritical() << "Ошибка при поиске пользователя по email:" << query.lastError().text();
        return {userInfo, "* Ошибка при поиске пользователя"};
    }

    if (!query.next()) {
        return {userInfo, "* Пользователь с таким email не найден"};
    }

    int userId = query.value(0).toInt();
    userInfo.login = query.value(1).toString();
    userInfo.email = email;
    userInfo.hashedPassword = hashPassword(newPassword);

    QSqlQuery updateQuery;
    updateQuery.prepare(R"(UPDATE users SET user_passhach = :newPassword WHERE id = :id)");
    updateQuery.bindValue(":newPassword", userInfo.hashedPassword);
    updateQuery.bindValue(":id", userId);

    if (!updateQuery.exec()) {
        qCritical() << "Ошибка при обновлении пароля:" << updateQuery.lastError().text();
        return {userInfo, "* Ошибка при изменении пароля"};
    }

    if (updateQuery.numRowsAffected() == 0) {
        return {userInfo, "* Пароль не обновлён"};
    }

    userInfo.isValid = true;
    qInfo() << "Пароль успешно изменён для пользователя с email:" << email;
    return {userInfo, "ok"};
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

bool AuthDatabase::changeUserLogin(const QString &login, const QString &newlogin) {

    QSqlQuery query;
    query.prepare(R"(UPDATE users SET user_login = :newlogin WHERE user_login = :login)");
    query.bindValue(":newlogin", newlogin);
    query.bindValue(":login", login);

    if (!query.exec()) {
        qCritical() << "Failed to change login for" << login << ":" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "No user found with login:" << login;
        return false;
    }

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
