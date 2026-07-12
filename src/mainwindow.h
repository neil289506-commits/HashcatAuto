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
#include "cmd.h" // 破解開始時彈出的美化終端機
#include "logwindow.h" // 破解開始時彈出的獨立日誌視窗
#include "pseudoconsole.h" // 透過 ConPTY 啟動 hashcat,讓單鍵指令能真正生效

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void startHashcat();
    void showResult();
    void clearLog();
    void exportLog();
    void clearResultFile();
    void sendCommand(const QString &cmd, const QString &btnName);

private:
    void appendLog(const QString &msg, const QString &color = "#e6e6e6");
    void applyStyleSheet();
    QPushButton* makeActionButton(const QString &text, const QString &cssClass);
    void relayToTerminal(const QString &text, const QString &color); // 同步輸出到 logView + CmdTerminal
    void parseAndSyncStatus(const QString &text); // 解析 hashcat 輸出的 Speed/Progress 並同步到終端機狀態列

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
    QNetworkAccessManager *navManager;
    MultiThreadDownloader *idmDownloader; // 核心下載器指標成員
    CmdTerminal *cmdTerminal; // 破解開始時彈出的美化終端機
    LogWindow *logWindow;     // 破解開始時彈出的獨立日誌視窗
    PseudoConsoleProcess *hashcatProcess = nullptr; // 透過 ConPTY 執行的 hashcat 子行程
};

#endif // MAINWINDOW_H
