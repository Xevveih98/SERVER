#include "main.h"
#include "EntriesManager.h"
#include "database.h"
#include "TodoManager.h"
#include "AuthManager.h"
#include "FoldersManager.h"
#include "CategoriesManager.h"
#include "ComputeManager.h"

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
    TodoManager todoManager;
    AuthManager authManager;
    FoldersManager foldersManager;
    CategoriesManager categoriesManager;
    EntriesManager entriesManager;
    ComputeManager computeManager;

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
    server.route("/recoverpassword", QHttpServerRequest::Method::Post,
                 [&authManager](const QHttpServerRequest &request) {
                     return authManager.handlePasswordRecover(request);
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

    server.route("/savefolder", QHttpServerRequest::Method::Post,
                 [&foldersManager](const QHttpServerRequest &request) {
                     return foldersManager.handleSaveFolder(request);
                 });
    server.route("/getuserfolders", QHttpServerRequest::Method::Get,
                 [&foldersManager](const QHttpServerRequest &request) {
                     return foldersManager.handleGetUserFolders(request);
                 });
    server.route("/deletefolder", QHttpServerRequest::Method::Post,
                 [&foldersManager](const QHttpServerRequest &request) {
                     return foldersManager.handleDeleteFolder(request);
                 });
    server.route("/changefolder", QHttpServerRequest::Method::Post,
                 [&foldersManager](const QHttpServerRequest &request) {
                     return foldersManager.handleFolderChange(request);
                 });

    server.route("/savetodo", QHttpServerRequest::Method::Post,
                 [&todoManager](const QHttpServerRequest &request) {
                     return todoManager.handleSaveTodo(request);
                 });
    server.route("/getusertodoos", QHttpServerRequest::Method::Get,
                 [&todoManager](const QHttpServerRequest &request) {
                     return todoManager.handleGetUserTodoos(request);
                 });
    server.route("/deletetodo", QHttpServerRequest::Method::Post,
                 [&todoManager](const QHttpServerRequest &request) {
                     return todoManager.handleDeleteTodo(request);
                 });

    server.route("/saveentry", QHttpServerRequest::Method::Post,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleSaveEntry(request);
                 });
    server.route("/getuserentries", QHttpServerRequest::Method::Get,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleGetUserEntries(request);
                 });
    server.route("/searchentriesbywords", QHttpServerRequest::Method::Post,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleSearchEntriesByKeywords(request);
                 });
    server.route("/searchentriesbytags", QHttpServerRequest::Method::Post,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleSearchEntriesByTags(request);
                 });
    server.route("/searchentriesbydate", QHttpServerRequest::Method::Post,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleSearchEntriesByDate(request);
                 });
    server.route("/getmoodidies", QHttpServerRequest::Method::Post,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleSearchEntriesMoodIdies(request);
                 });
    server.route("/deleteentry", QHttpServerRequest::Method::Post,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleDeleteEntry(request);
                 });
    server.route("/updateentry", QHttpServerRequest::Method::Post,
                 [&entriesManager](const QHttpServerRequest &request) {
                     return entriesManager.handleUpdateEntry(request);
                 });

    server.route("/loadentriesbymonth", QHttpServerRequest::Method::Post,
                 [&computeManager](const QHttpServerRequest &request) {
                     return computeManager.handleLoadEntriesByMonth(request);
                 });

    startServer(server);

    return app.exec();
}
