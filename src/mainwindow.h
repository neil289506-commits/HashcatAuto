#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QProcess>
#include <QNetworkAccessManager>
#include "multithreaddownloader.h" // 必須引入新下載器標頭

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void startHashcat();
    void showResult();
    void clearLog();
    void exportLog();          // 補上對應 cpp 第 174 行的實作宣告
    void clearResultFile();
    void sendCommand(const QString &cmd, const QString &btnName);

private:
    void appendLog(const QString &msg, const QString &color = "white");

    // UI 元件
    QLineEdit *hashFileEdit;
    QSpinBox *modeSpin;
    QLineEdit *outPathEdit;
    QComboBox *attackModeCombo;
    QLineEdit *dict1Edit;
    QLineEdit *dict2Edit;
    QLineEdit *maskEdit;
    QTextEdit *logView;

    // 後台進程與異步網路
    QProcess *process;
    QNetworkAccessManager *navManager;
    MultiThreadDownloader *idmDownloader; // 補上核心類別指標成員
};

#endif // MAINWINDOW_H