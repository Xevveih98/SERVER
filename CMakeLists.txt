cmake_minimum_required(VERSION 3.16)

project(PSQLSERVER LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network HttpServer Sql)
find_package(Qt6 REQUIRED COMPONENTS Core Network HttpServer Sql)

add_executable(PSQLSERVER
  main.cpp
  main.h
  AuthManager.h
  AuthManager.cpp
  Database.h
  Database.cpp
)
target_link_libraries(PSQLSERVER
  Qt6::Core
  Qt6::Network
  Qt6::HttpServer
  Qt6::Sql)

include(GNUInstallDirs)
install(TARGETS PSQLSERVER
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
