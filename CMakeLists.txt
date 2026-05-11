cmake_minimum_required(VERSION 3.20)
project(PhoenixObe VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/EditableImageView.cpp
    src/EditPanel.cpp
)

set(HEADERS
    src/MainWindow.h
    src/EditableImageView.h
    src/EditPanel.h
)

add_executable(PhoenixObe WIN32
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(PhoenixObe PRIVATE Qt6::Widgets)

if(WIN32)
    add_custom_command(TARGET PhoenixObe POST_BUILD
        COMMAND "${Qt6_DIR}/../../../bin/windeployqt6.exe"
                --no-translations
                "$<TARGET_FILE:PhoenixObe>"
        COMMENT "Running windeployqt6..."
    )
endif()
