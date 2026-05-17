#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    // 解決高解析度螢幕縮放問題
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}