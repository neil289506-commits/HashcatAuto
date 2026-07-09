cmake_minimum_required(VERSION 3.16)

project(HashcatTool VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自動處理 Qt 的 Meta-Object System (MOC) 與 UI 檔案
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 尋找 Qt6 必要組件 (包含網路與視窗系統)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# 執行檔的源檔案清單 (在此必須加入全新的多線程下載器)
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/multithreaddownloader.cpp
    src/multithreaddownloader.h
    src/app.rc
)

# 視窗程式設定 (避免在 Windows 背景啟動多餘的 CMD 視窗)
if(WIN32)
    add_executable(HashcatTool WIN32 ${PROJECT_SOURCES})
else()
    add_executable(HashcatTool ${PROJECT_SOURCES})
endif()

# 連結 Qt6 函式庫
target_link_libraries(HashcatTool PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)