#include "main.h"
#include "database.h"
#include "AuthManager.h"


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


    startServer(server);

    return app.exec();
}
