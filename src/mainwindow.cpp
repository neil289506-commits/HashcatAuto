#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextCursor>
#include <QIcon>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), process(nullptr) {
    navManager = new QNetworkAccessManager(this);
    idmDownloader = new MultiThreadDownloader(this); // 初始化 32 線程下載器
    
    QWidget *c = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(c);

    // --- 圖標與標題 ---
    setWindowIcon(QIcon("icon.ico"));
    setWindowTitle("Hashcat Ultra Professional v2026 [Neil Edition]");
    resize(1000, 850);

    // --- 區塊 1: 參數設定 ---
    QGroupBox *cfgGroup = new QGroupBox("⚙️ 指令參數設定 (MSVC + Qt)");
    QGridLayout *gl = new QGridLayout(cfgGroup);
    hashFileEdit = new QLineEdit();
    modeSpin = new QSpinBox(); modeSpin->setMaximum(99999);
    outPathEdit = new QLineEdit("result.txt");
    QPushButton *btnH = new QPushButton("瀏覽目標...");
    gl->addWidget(new QLabel("目標 Hash:"), 0, 0); gl->addWidget(hashFileEdit, 0, 1); gl->addWidget(btnH, 0, 2);
    gl->addWidget(new QLabel("類型 -m:"), 1, 0); gl->addWidget(modeSpin, 1, 1);
    gl->addWidget(new QLabel("輸出 -o:"), 1, 2); gl->addWidget(outPathEdit, 1, 3);

    // --- 區塊 2: 攻擊模式 ---
    QGroupBox *modeGroup = new QGroupBox("⚔️ 攻擊模式設定");
    QGridLayout *al = new QGridLayout(modeGroup);
    attackModeCombo = new QComboBox();
    attackModeCombo->addItem("字典模式 (-a 0)", 0);
    attackModeCombo->addItem("組合模式 (-a 1)", 1);
    attackModeCombo->addItem("掩碼模式 (-a 3)", 3);
    dict1Edit = new QLineEdit(); dict2Edit = new QLineEdit(); maskEdit = new QLineEdit();
    QPushButton *btnD1 = new QPushButton("字典 1"); QPushButton *btnD2 = new QPushButton("字典 2");
    al->addWidget(new QLabel("攻擊模式:"), 0, 0); al->addWidget(attackModeCombo, 0, 1);
    al->addWidget(new QLabel("字典 1:"), 1, 0); al->addWidget(dict1Edit, 1, 1); al->addWidget(btnD1, 1, 2);
    al->addWidget(new QLabel("字典 2:"), 2, 0); al->addWidget(dict2Edit, 2, 1); al->addWidget(btnD2, 2, 2);
    al->addWidget(new QLabel("掩碼 [Mask]:"), 3, 0); al->addWidget(maskEdit, 3, 1, 1, 2);

    // --- 區塊 3: 終端日誌 (14px 粗體黑底) ---
    logView = new QTextEdit();
    logView->setReadOnly(true);
    logView->setStyleSheet(
        "background-color: black; color: white; "
        "font-family: 'Microsoft JhengHei'; font-size: 14px; "
        "font-weight: bold; border: 2px solid #333; padding: 5px;"
    );

    // --- 區塊 4: 控制列 ---
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    auto addCtrl = [&](QString n, QString c, QString clr) {
        QPushButton *b = new QPushButton(n);
        b->setStyleSheet(QString("background:%1; color:white; font-weight:bold; height:35px;").arg(clr));
        connect(b, &QPushButton::clicked, [=](){ sendCommand(c, n); });
        ctrlLayout->addWidget(b);
    };
    addCtrl("📊 Status", "s", "#34495e");
    addCtrl("⏸️ Pause", "p", "#d35400");
    addCtrl("▶️ Resume", "r", "#27ae60");
    addCtrl("🛑 Quit", "q", "#c0392b");

    // 日誌管理與結果清理按鈕群
    QPushButton *btnExportLog = new QPushButton("💾 匯出日誌");
    btnExportLog->setStyleSheet("background:#2980b9; color:white; font-weight:bold; height:35px;");
    connect(btnExportLog, &QPushButton::clicked, this, &MainWindow::exportLog);
    ctrlLayout->addWidget(btnExportLog);

    QPushButton *btnClearLog = new QPushButton("🗑️ 清空日誌");
    btnClearLog->setStyleSheet("background:#7f8c8d; color:white; font-weight:bold; height:35px;");
    connect(btnClearLog, &QPushButton::clicked, this, &MainWindow::clearLog);
    ctrlLayout->addWidget(btnClearLog);

    QPushButton *btnResetRes = new QPushButton("🗑️ 清空結果檔案");
    btnResetRes->setStyleSheet("background:#e67e22; color:white; font-weight:bold; height:35px;");
    connect(btnResetRes, &QPushButton::clicked, this, &MainWindow::clearResultFile);
    ctrlLayout->addWidget(btnResetRes);

    // --- 區塊 5: 啟動按鈕 ---
    QHBoxLayout *runLayout = new QHBoxLayout();
    QPushButton *btnRun = new QPushButton("🚀 啟動任務 (自動下載核心 + RTX 加速)");
    QPushButton *btnShow = new QPushButton("📜 查看 Crack 結果");
    btnRun->setStyleSheet("height:60px; background:#2ecc71; color:white; font-weight:bold; font-size:18px;");
    btnShow->setStyleSheet("height:60px; background:#3498db; color:white; font-weight:bold; font-size:18px;");
    runLayout->addWidget(btnRun); runLayout->addWidget(btnShow);

    mainLayout->addWidget(cfgGroup);
    mainLayout->addWidget(modeGroup);
    mainLayout->addWidget(logView);
    mainLayout->addLayout(ctrlLayout);
    mainLayout->addLayout(runLayout);
    setCentralWidget(c);

    // 信號與槽 (UI 控制綁定)
    connect(btnH, &QPushButton::clicked, [=](){ hashFileEdit->setText(QFileDialog::getOpenFileName(this, "選擇 Hash 檔案")); });
    connect(btnD1, &QPushButton::clicked, [=](){ dict1Edit->setText(QFileDialog::getOpenFileName(this, "選擇字典 1")); });
    connect(btnD2, &QPushButton::clicked, [=](){ dict2Edit->setText(QFileDialog::getOpenFileName(this, "選擇字典 2")); });
    connect(btnRun, &QPushButton::clicked, this, &MainWindow::startHashcat);
    connect(btnShow, &QPushButton::clicked, this, &MainWindow::showResult);

    // 綁定 IDM 高速下載器的訊號回報
    connect(idmDownloader, &MultiThreadDownloader::progressUpdated, this, [=](double pct, double speed){
        appendLog(QString("📥 [IDM 32線程] 下載進度: %1% | 總速度: %2 MB/s").arg(pct, 0, 'f', 1).arg(speed, 0, 'f', 2), "#f1c40f");
    });

    connect(idmDownloader, &MultiThreadDownloader::downloadFinished, this, [=](bool success, QString msg){
        if(success) {
            appendLog("📦 下載成功，正在使用 PowerShell 自動部署...", "#2ecc71");
            QProcess::execute("powershell", QStringList() << "-Command" << 
                "tar -xf hc.7z; Get-ChildItem -Path 'hashcat-*' | %{ Copy-Item -Path \"$($_.FullName)/*\" -Destination './' -Recurse -Force }; Remove-Item -Path 'hc.7z','hashcat-*' -Recurse -Force;");
            appendLog("✅ 部署完成，請再次點擊啟動任務！", "#2ecc71");
        } else {
            appendLog("❌ 下載失敗: " + msg, "red");
        }
    });
}

void MainWindow::startHashcat() {
    // 檢查核心是否已就緒，若無，則啟動 32 線程異步並行分段下載
    if (!QFile::exists("hashcat.exe")) {
        appendLog("🌐 核心不存在，正在啟動 32 執行緒高速並行下載 Hashcat 6.2.6...", "#3498db");
        idmDownloader->startDownload("https://hashcat.net/files/hashcat-6.2.6.7z", "hc.7z", 32);
        return;
    }

    if (process && process->state() == QProcess::Running) return;
    
    process = new QProcess(this);
    QStringList args;
    int mode = attackModeCombo->currentData().toInt();
    
    // RTX 3050 算力深度調校參數 (-w 3: High Workload Profile 模式)
    args << "-a" << QString::number(mode) << "-m" << QString::number(modeSpin->value())
         << "-o" << outPathEdit->text() << "--force" << "-w" << "3" << hashFileEdit->text();
    
    if (mode == 0) args << dict1Edit->text();
    else if (mode == 1) args << dict1Edit->text() << dict2Edit->text();
    else if (mode == 3) args << (maskEdit->text().isEmpty() ? "?a?a?a?a?a?a?a?a" : maskEdit->text());

    connect(process, &QProcess::readyReadStandardOutput, this, [=](){ appendLog(process->readAllStandardOutput()); });
    connect(process, &QProcess::readyReadStandardError, this, [=](){ appendLog(process->readAllStandardError(), "red"); });
    
    process->start("hashcat.exe", args);
    appendLog("🚀 指令已發送，RTX 加速中...", "#2ecc71");
}

void MainWindow::sendCommand(const QString &cmd, const QString &btnName) {
    if (process && process->state() == QProcess::Running) {
        // 修正 Windows 換行符並強制同步排空緩衝區
        process->write(cmd.toUtf8() + "\r\n");
        process->waitForBytesWritten();
        appendLog("➤ 手動指令: " + btnName, "#3498db");
    }
}

void MainWindow::appendLog(const QString &msg, const QString &color) {
    if (msg.isEmpty()) return;
    logView->setTextColor(QColor(color));
    logView->append(msg.trimmed());
    logView->moveCursor(QTextCursor::End);
}

void MainWindow::clearLog() { 
    logView->clear(); 
    appendLog("🗑️ 日誌快取已清空", "#7f8c8d"); 
}

void MainWindow::exportLog() {
    QString fileName = QFileDialog::getSaveFileName(this, "匯出終端日誌", "hashcat_log.txt", "文字檔案 (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << logView->toPlainText();
            file.close();
            QMessageBox::information(this, "成功", "日誌已成功匯出至：" + fileName);
        } else {
            QMessageBox::warning(this, "錯誤", "無法寫入檔案，請檢查權限設定");
        }
    }
}

void MainWindow::clearResultFile() { 
    if(QFile::remove(outPathEdit->text())) appendLog("✅ 結果檔已清除", "#e67e22"); 
}

void MainWindow::showResult() {
    QFile f(outPathEdit->text());
    if (f.open(QIODevice::ReadOnly)) { 
        QMessageBox::information(this, "破解成功", f.readAll()); 
        f.close();
    } else {
        QMessageBox::warning(this, "提示", "尚未發現破解結果或檔案無法開啟");
    }
}