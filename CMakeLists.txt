cmake_minimum_required(VERSION 3.16)

project(UserManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_add_executable(UserManager
    main.cpp
    usermodel.cpp
    usermodel.h
)

# 👇 ВОТ ЭТА СТРОКА УБИРАЕТ КОНСОЛЬ
set_target_properties(UserManager PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE   TRUE
)

qt_add_qml_module(UserManager
    URI UserManager
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(UserManager PRIVATE Qt6::Quick)