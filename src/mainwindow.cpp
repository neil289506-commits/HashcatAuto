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
#include <QFont>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    navManager = new QNetworkAccessManager(this);
    idmDownloader = new MultiThreadDownloader(this); // 初始化 32 線程下載器
    cmdTerminal = new CmdTerminal(this); // 美化終端機(破解開始時彈出)
    connect(cmdTerminal, &CmdTerminal::commandRequested, this, &MainWindow::sendCommand);
    logWindow = new LogWindow(this); // 獨立日誌視窗(破解開始時彈出)

    setWindowIcon(QIcon("icon.ico"));
    setWindowTitle("Hashcat Ultra Professional v2026  |  Neil Edition");
    resize(1040, 880);

    applyStyleSheet();

    QWidget *c = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(c);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // --- 標題列 ---
    QLabel *headerLabel = new QLabel("HASHCAT ULTRA PROFESSIONAL");
    headerLabel->setObjectName("headerLabel");
    QLabel *subLabel = new QLabel("MSVC 2022 x64 · Qt 6.11.0 · RTX 加速核心");
    subLabel->setObjectName("subLabel");
    QVBoxLayout *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(2);
    headerLayout->addWidget(headerLabel);
    headerLayout->addWidget(subLabel);
    mainLayout->addLayout(headerLayout);

    // --- 區塊 1: 參數設定 ---
    QGroupBox *cfgGroup = new QGroupBox("指令參數設定");
    QGridLayout *gl = new QGridLayout(cfgGroup);
    gl->setSpacing(10);
    hashFileEdit = new QLineEdit();
    hashFileEdit->setPlaceholderText("選擇要破解的 Hash 檔案路徑");
    modeSpin = new QSpinBox(); modeSpin->setMaximum(99999);
    outPathEdit = new QLineEdit("result.txt");
    QPushButton *btnH = makeActionButton("瀏覽...", "neutral");
    gl->addWidget(new QLabel("目標 Hash"), 0, 0); gl->addWidget(hashFileEdit, 0, 1); gl->addWidget(btnH, 0, 2);
    gl->addWidget(new QLabel("類型 -m"), 1, 0); gl->addWidget(modeSpin, 1, 1);
    gl->addWidget(new QLabel("輸出 -o"), 1, 2); gl->addWidget(outPathEdit, 1, 3);

    // --- 區塊 2: 攻擊模式 ---
    QGroupBox *modeGroup = new QGroupBox("攻擊模式設定");
    QGridLayout *al = new QGridLayout(modeGroup);
    al->setSpacing(10);
    attackModeCombo = new QComboBox();
    attackModeCombo->addItem("字典模式 (-a 0)", 0);
    attackModeCombo->addItem("組合模式 (-a 1)", 1);
    attackModeCombo->addItem("掩碼模式 (-a 3)", 3);
    dict1Edit = new QLineEdit(); dict2Edit = new QLineEdit(); maskEdit = new QLineEdit();
    QPushButton *btnD1 = makeActionButton("字典 1", "neutral");
    QPushButton *btnD2 = makeActionButton("字典 2", "neutral");
    al->addWidget(new QLabel("攻擊模式"), 0, 0); al->addWidget(attackModeCombo, 0, 1);
    al->addWidget(new QLabel("字典 1"), 1, 0); al->addWidget(dict1Edit, 1, 1); al->addWidget(btnD1, 1, 2);
    al->addWidget(new QLabel("字典 2"), 2, 0); al->addWidget(dict2Edit, 2, 1); al->addWidget(btnD2, 2, 2);
    al->addWidget(new QLabel("掩碼 (Mask)"), 3, 0); al->addWidget(maskEdit, 3, 1, 1, 2);

    // --- 區塊 3: 終端日誌 ---
    QGroupBox *logGroup = new QGroupBox("終端輸出");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logView = new QTextEdit();
    logView->setObjectName("logView");
    logView->setReadOnly(true);
    QFont logFont("Consolas");
    logFont.setStyleHint(QFont::Monospace);
    logFont.setPointSize(10);
    logView->setFont(logFont);
    logLayout->addWidget(logView);

    // --- 區塊 4: hashcat 執行期控制指令 (單鍵指令) ---
    QGroupBox *ctrlGroup = new QGroupBox("執行期控制指令 (Runtime Keys)");
    QHBoxLayout *ctrlLayout = new QHBoxLayout(ctrlGroup);
    ctrlLayout->setSpacing(8);

    auto addCtrl = [&](const QString &label, const QString &cmd, const QString &cssClass) {
        QPushButton *b = makeActionButton(label, cssClass);
        connect(b, &QPushButton::clicked, [=](){ sendCommand(cmd, label); });
        ctrlLayout->addWidget(b);
        return b;
    };

    addCtrl("[S] Status",     "s", "info");
    addCtrl("[P] Pause",      "p", "warning");
    addCtrl("[R] Resume",     "r", "success");
    addCtrl("[B] Bypass",     "b", "neutral");
    addCtrl("[C] Checkpoint", "c", "neutral");
    addCtrl("[F] Finish",     "f", "warning");
    addCtrl("[Q] Quit",       "q", "danger");

    // --- 區塊 5: 日誌管理 ---
    QGroupBox *toolsGroup = new QGroupBox("日誌與檔案管理");
    QHBoxLayout *toolsLayout = new QHBoxLayout(toolsGroup);
    toolsLayout->setSpacing(8);

    QPushButton *btnExportLog = makeActionButton("匯出日誌", "info");
    connect(btnExportLog, &QPushButton::clicked, this, &MainWindow::exportLog);
    toolsLayout->addWidget(btnExportLog);

    QPushButton *btnClearLog = makeActionButton("清空日誌", "neutral");
    connect(btnClearLog, &QPushButton::clicked, this, &MainWindow::clearLog);
    toolsLayout->addWidget(btnClearLog);

    QPushButton *btnResetRes = makeActionButton("清空結果檔案", "warning");
    connect(btnResetRes, &QPushButton::clicked, this, &MainWindow::clearResultFile);
    toolsLayout->addWidget(btnResetRes);

    // --- 區塊 6: 啟動按鈕 ---
    QHBoxLayout *runLayout = new QHBoxLayout();
    runLayout->setSpacing(10);
    QPushButton *btnRun = new QPushButton("啟動任務  (自動下載核心 + RTX 加速)");
    QPushButton *btnShow = new QPushButton("查看 Crack 結果");
    btnRun->setObjectName("runButton");
    btnShow->setObjectName("showButton");
    btnRun->setFixedHeight(56);
    btnShow->setFixedHeight(56);
    runLayout->addWidget(btnRun, 2);
    runLayout->addWidget(btnShow, 1);

    mainLayout->addWidget(cfgGroup);
    mainLayout->addWidget(modeGroup);
    mainLayout->addWidget(logGroup, 1);
    mainLayout->addWidget(ctrlGroup);
    mainLayout->addWidget(toolsGroup);
    mainLayout->addLayout(runLayout);
    setCentralWidget(c);

    // 信號與槽 (UI 控制綁定)
    connect(btnH, &QPushButton::clicked, [=](){ hashFileEdit->setText(QFileDialog::getOpenFileName(this, "選擇 Hash 檔案")); });
    connect(btnD1, &QPushButton::clicked, [=](){ dict1Edit->setText(QFileDialog::getOpenFileName(this, "選擇字典 1")); });
    connect(btnD2, &QPushButton::clicked, [=](){ dict2Edit->setText(QFileDialog::getOpenFileName(this, "選擇字典 2")); });
    connect(btnRun, &QPushButton::clicked, this, &MainWindow::startHashcat);
    connect(btnShow, &QPushButton::clicked, this, &MainWindow::showResult);

    // 綁定下載器的訊號回報
    connect(idmDownloader, &MultiThreadDownloader::progressUpdated, this, [=](double pct, double speed){
        appendLog(QString("[下載] 進度: %1%%  總速度: %2 MB/s").arg(pct, 0, 'f', 1).arg(speed, 0, 'f', 2), "#f5c518");
    });

    connect(idmDownloader, &MultiThreadDownloader::downloadFinished, this, [=](bool success, QString msg){
        if(success) {
            appendLog("[下載] 完成,正在使用 PowerShell 自動部署...", "#2ecc71");
            QProcess::execute("powershell", QStringList() << "-Command" <<
                "tar -xf hc.7z; Get-ChildItem -Path 'hashcat-*' | %{ Copy-Item -Path \"$($_.FullName)/*\" -Destination './' -Recurse -Force }; Remove-Item -Path 'hc.7z','hashcat-*' -Recurse -Force;");
            appendLog("[部署] 完成,請再次點擊啟動任務", "#2ecc71");
        } else {
            appendLog("[錯誤] 下載失敗: " + msg, "#e74c3c");
        }
    });
}

QPushButton* MainWindow::makeActionButton(const QString &text, const QString &cssClass) {
    QPushButton *b = new QPushButton(text);
    b->setProperty("class", cssClass);
    b->setMinimumHeight(34);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

void MainWindow::applyStyleSheet() {
    setStyleSheet(R"(
        QWidget {
            background-color: #1b1d23;
            color: #e6e6e6;
            font-family: "Microsoft JhengHei", "Segoe UI";
            font-size: 13px;
        }
        QLabel#headerLabel {
            font-size: 20px;
            font-weight: 700;
            color: #ffffff;
            letter-spacing: 1px;
        }
        QLabel#subLabel {
            font-size: 11px;
            color: #8a8f98;
        }
        QGroupBox {
            border: 1px solid #2e313a;
            border-radius: 8px;
            margin-top: 14px;
            padding: 12px 10px 10px 10px;
            font-weight: 600;
            color: #c7cbd1;
            background-color: #20222a;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #5aa9ff;
        }
        QLineEdit, QSpinBox, QComboBox {
            background-color: #14151a;
            border: 1px solid #33363f;
            border-radius: 6px;
            padding: 6px 8px;
            color: #e6e6e6;
            selection-background-color: #3b82f6;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1px solid #5aa9ff;
        }
        QTextEdit#logView {
            background-color: #0d0e12;
            border: 1px solid #2e313a;
            border-radius: 6px;
            padding: 8px;
            color: #d9d9d9;
        }
        QPushButton {
            border: none;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 600;
            color: #ffffff;
            background-color: #3a3f4b;
        }
        QPushButton:hover { background-color: #4a505e; }
        QPushButton:pressed { background-color: #2e323c; }

        QPushButton[class="neutral"]  { background-color: #4b5563; }
        QPushButton[class="neutral"]:hover { background-color: #5b6472; }

        QPushButton[class="info"]     { background-color: #2563eb; }
        QPushButton[class="info"]:hover { background-color: #3b82f6; }

        QPushButton[class="success"]  { background-color: #16a34a; }
        QPushButton[class="success"]:hover { background-color: #22c55e; }

        QPushButton[class="warning"]  { background-color: #d97706; }
        QPushButton[class="warning"]:hover { background-color: #f59e0b; }

        QPushButton[class="danger"]   { background-color: #dc2626; }
        QPushButton[class="danger"]:hover { background-color: #ef4444; }

        QPushButton#runButton {
            background-color: #16a34a;
            font-size: 15px;
        }
        QPushButton#runButton:hover { background-color: #22c55e; }

        QPushButton#showButton {
            background-color: #2563eb;
            font-size: 15px;
        }
        QPushButton#showButton:hover { background-color: #3b82f6; }

        QScrollBar:vertical {
            background: #14151a;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #3a3f4b;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover { background: #4a505e; }
    )");
}

void MainWindow::startHashcat() {
    // 檢查核心是否已就緒,若無,則啟動 32 線程異步並行分段下載
    if (!QFile::exists("hashcat.exe")) {
        appendLog("[核心] 尚未就緒,正在啟動 32 執行緒高速並行下載 Hashcat 6.2.6...", "#3498db");
        idmDownloader->startDownload("https://hashcat.net/files/hashcat-6.2.6.7z", "hc.7z", 32);
        return;
    }

    if (hashcatProcess && hashcatProcess->isRunning()) return;

    QStringList args;
    int mode = attackModeCombo->currentData().toInt();

    // RTX 3050 算力深度調校參數 (-w 3: High Workload Profile 模式)
    args << "-a" << QString::number(mode) << "-m" << QString::number(modeSpin->value())
         << "-o" << outPathEdit->text() << "--force" << "-w" << "3" << hashFileEdit->text();

    if (mode == 0) args << dict1Edit->text();
    else if (mode == 1) args << dict1Edit->text() << dict2Edit->text();
    else if (mode == 3) args << (maskEdit->text().isEmpty() ? "?a?a?a?a?a?a?a?a" : maskEdit->text());

    // 改用 ConPTY 啟動 hashcat,而非 QProcess 的匿名管線。
    // hashcat 的 s/p/r/b/c/f/q 單鍵指令是靠主控台 API 讀鍵盤緩衝區,
    // 沒有真正的主控台連接,那些按鍵永遠送不到 hashcat 手上。
    hashcatProcess = new PseudoConsoleProcess(this);

    connect(hashcatProcess, &PseudoConsoleProcess::outputReceived, this, [=](const QString &text){
        relayToTerminal(text, QString());
    });
    connect(hashcatProcess, &PseudoConsoleProcess::errorOccurred, this, [=](const QString &msg){
        appendLog("[錯誤] " + msg, "#e74c3c");
        cmdTerminal->stopSession(false, msg);
        logWindow->appendLine("[錯誤] " + msg, "#ef4444");
    });
    connect(hashcatProcess, &PseudoConsoleProcess::finished, this, [=](int exitCode){
        bool ok = (exitCode == 0);
        QString reason = ok ? QString() : QString("結束碼: %1").arg(exitCode);
        cmdTerminal->stopSession(ok, reason);
        logWindow->appendLine(ok ? "[任務結束] 正常完成" : "[任務結束] " + reason, ok ? "#22c55e" : "#ef4444");
        appendLog(ok ? "[任務] hashcat 已正常結束" : "[任務] hashcat 結束,代碼 " + QString::number(exitCode), ok ? "#2ecc71" : "#e74c3c");
    });

    // 破解開始時彈出美化終端機 + 獨立日誌視窗,並即時同步資訊
    QString sessionLabel = QString("Mode -a %1  |  -m %2").arg(mode).arg(modeSpin->value());
    cmdTerminal->startSession(sessionLabel);
    logWindow->startSession(sessionLabel);

    if (!hashcatProcess->start("hashcat.exe", args)) {
        appendLog("[錯誤] 無法透過 ConPTY 啟動 hashcat.exe", "#e74c3c");
        return;
    }
    appendLog("[任務] 已透過虛擬終端(ConPTY)啟動 hashcat,RTX 加速中...", "#2ecc71");
}

void MainWindow::sendCommand(const QString &cmd, const QString &btnName) {
    if (hashcatProcess && hashcatProcess->isRunning()) {
        // ConPTY:直接送出單一按鍵字元,完全比照真人在終端機上按鍵,
        // 不需要換行、不需要 Enter,符合 hashcat 讀取單鍵指令的規定。
        hashcatProcess->writeKey(cmd);
        appendLog("[指令] " + btnName, "#5aa9ff");
    } else {
        appendLog("[提示] 尚未有任務執行中,無法送出指令: " + btnName, "#e74c3c");
    }
}

void MainWindow::relayToTerminal(const QString &text, const QString &color) {
    if (text.trimmed().isEmpty()) return;
    appendLog(text, color.isEmpty() ? "#e6e6e6" : color); // 主視窗日誌
    cmdTerminal->appendOutput(text, color);                 // 同步彈出終端機
    logWindow->appendLine(text, color);                     // 同步獨立日誌視窗
    parseAndSyncStatus(text);                                // 解析速度/進度並更新狀態列
}

void MainWindow::parseAndSyncStatus(const QString &text) {
    // hashcat --status 輸出範例:
    // Speed.#1.........:  1234.5 MH/s
    // Progress.........: 123456/999999 (12.34%)
    QString speed, progress, state;

    QRegularExpression speedRe(R"(Speed\.#\d+\.*:\s*([\d.,]+\s*[kKmMgG]?H/s))");
    QRegularExpressionMatch sm = speedRe.match(text);
    if (sm.hasMatch()) speed = sm.captured(1).trimmed();

    QRegularExpression progRe(R"(Progress\.*:\s*([\d/]+\s*\([\d.]+%\)))");
    QRegularExpressionMatch pm = progRe.match(text);
    if (pm.hasMatch()) progress = pm.captured(1).trimmed();

    QRegularExpression statusRe(R"(Status\.*:\s*(\S+))");
    QRegularExpressionMatch stm = statusRe.match(text);
    if (stm.hasMatch()) state = stm.captured(1).trimmed();

    if (!speed.isEmpty() || !progress.isEmpty() || !state.isEmpty()) {
        cmdTerminal->setStatusInfo(speed, progress, state.isEmpty() ? QString() : ("執行中 (" + state + ")"));
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
    appendLog("[日誌] 快取已清空", "#8a8f98");
}

void MainWindow::exportLog() {
    QString fileName = QFileDialog::getSaveFileName(this, "匯出終端日誌", "hashcat_log.txt", "文字檔案 (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << logView->toPlainText();
            file.close();
            QMessageBox::information(this, "成功", "日誌已成功匯出至:" + fileName);
        } else {
            QMessageBox::warning(this, "錯誤", "無法寫入檔案,請檢查權限設定");
        }
    }
}

void MainWindow::clearResultFile() {
    if(QFile::remove(outPathEdit->text())) appendLog("[結果] 結果檔已清除", "#f59e0b");
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
