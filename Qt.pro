QT       += core gui widgets network

TARGET = HashcatTool
TEMPLATE = app

# 【核心強制設定】編譯時必須存在 icon.ico，否則報錯
RC_ICONS = icon.ico

SOURCES += main.cpp \
           mainwindow.cpp

HEADERS += mainwindow.h

# 強制 C++17 標準
CONFIG += c++17