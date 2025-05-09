#include "main.h"
#include "database.h"
#include "AuthManager.h"
#include "CategoriesManager.h"


void startServer(QHttpServer &server)
{
    auto tcpserver = std::make_unique<QTcpServer>();

    if (!tcpserver->listen(QHostAddress::Any, 8080)) {
        qCritical() << "Failed to start QTcpServer:" << tcpserver->errorString();
        return;
    }

    if (!server.bind(tcpserver.get())) {
        qCritical() << "Failed to bind QHttpServer.";
        return;
    }

    qInfo() << "Server is listening on port" << tcpserver->serverPort();
    tcpserver.release();  // управление передано QHttpServer
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    if (!Database::connect()) {
        qCritical() << "Database connection failed.";
        return -1;
    }

    qInfo() << "Database connected successfully.";

    QHttpServer server;
    AuthManager authManager;
    CategoriesManager categoriesManager;

    server.route("/register", QHttpServerRequest::Method::Post,
                 [&authManager](const QHttpServerRequest &request) {
                     return authManager.handleRegister(request);
                 });
    server.route("/login", QHttpServerRequest::Method::Post,
                 [&authManager](const QHttpServerRequest &request) {
                     return authManager.handleLogin(request);
                 });
    server.route("/changepassword", QHttpServerRequest::Method::Post,
                 [&authManager](const QHttpServerRequest &request) {
                     return authManager.handlePasswordChange(request);
                 });
    server.route("/deleteuser", QHttpServerRequest::Method::Post,
                 [&authManager](const QHttpServerRequest &request) {
                     return authManager.handleLoginToDelete(request);
                 });
    server.route("/changemail", QHttpServerRequest::Method::Post,
                 [&authManager](const QHttpServerRequest &request) {
                     return authManager.handleEmailChange(request);
                 });


    server.route("/savetags", QHttpServerRequest::Method::Post,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleSaveTags(request);
                 });
    server.route("/getusertags", QHttpServerRequest::Method::Get,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleGetUserTags(request);
                 });
    server.route("/deletetag", QHttpServerRequest::Method::Post,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleDeleteTag(request);
                 });


    server.route("/saveactivity", QHttpServerRequest::Method::Post,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleSaveActivity(request);
                 });
    server.route("/getuseractivity", QHttpServerRequest::Method::Get,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleGetUserActivity(request);
                 });
    server.route("/deleteactivity", QHttpServerRequest::Method::Post,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleDeleteActivity(request);
                 });

    server.route("/saveemotion", QHttpServerRequest::Method::Post,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleSaveEmotion(request);
                 });
    server.route("/getuseremotions", QHttpServerRequest::Method::Get,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleGetUserEmotions(request);
                 });
    server.route("/deleteemotion", QHttpServerRequest::Method::Post,
                 [&categoriesManager](const QHttpServerRequest &request) {
                     return categoriesManager.handleDeleteEmotion(request);
                 });



    startServer(server);

    return app.exec();
}
