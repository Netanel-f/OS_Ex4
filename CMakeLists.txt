cmake_minimum_required(VERSION 3.8)
project(OS_Ex4)

set(CMAKE_CXX_STANDARD 11)

set(CLIENT whatsappClient.cpp)
set(SERVER OLD_whatsappServer.cpp)
set(NEW_SERVER whatsappServer1.cpp)
set(IO whatsappio.cpp whatsappio.h)

set(SERVER_SRC ${SERVER} ${IO})
add_executable(whatsappServer ${SERVER_SRC})

set(NEW_SERVER_SRC ${NEW_SERVER} ${IO})
add_executable(NewWhatsappServer ${NEW_SERVER_SRC})

set(CLIENT_SRC ${CLIENT} ${IO})
add_executable(whatsappClient ${CLIENT_SRC})