#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QFile>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
private slots:
    void startHashcat();
    void sendCommand(const QString &cmd, const QString &btnName);
    void appendLog(const QString &msg, const QString &color = "white");
    void clearLog();
    void clearResultFile();
    void showResult();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(QNetworkReply* reply);
private:
    QLineEdit *hashFileEdit, *dict1Edit, *dict2Edit, *maskEdit, *outPathEdit;
    QSpinBox *modeSpin;
    QComboBox *attackModeCombo;
    QTextEdit *logView;
    QProcess *process;
    QNetworkAccessManager *navManager;
    QElapsedTimer downloadTimer;
};
#endif