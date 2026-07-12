#include "cmd.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QTextCursor>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>

CmdTerminal::CmdTerminal(QWidget *parent)
    : QDialog(parent), dragging(false) {

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false); // 只隱藏,重複使用同一個實例
    setModal(false);
    resize(880, 560);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QWidget *frame = new QWidget(this);
    frame->setObjectName("terminalFrame");
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);
    root->addWidget(frame);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 180));
    frame->setGraphicsEffect(shadow);

    // ---- 標題列(可拖曳、含紅黃綠仿系統按鈕)----
    titleBar = new QWidget(frame);
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(kTitleBarHeight);
    QHBoxLayout *tbLayout = new QHBoxLayout(titleBar);
    tbLayout->setContentsMargins(14, 0, 10, 0);
    tbLayout->setSpacing(8);

    QLabel *dotRed = new QLabel(); dotRed->setObjectName("dotRed"); dotRed->setFixedSize(12, 12);
    QLabel *dotYellow = new QLabel(); dotYellow->setObjectName("dotYellow"); dotYellow->setFixedSize(12, 12);
    QLabel *dotGreen = new QLabel(); dotGreen->setObjectName("dotGreen"); dotGreen->setFixedSize(12, 12);

    titleLabel = new QLabel("hashcat@rtx-accel  ~  執行期終端機");
    titleLabel->setObjectName("titleLabel");

    btnMin = new QPushButton("—");
    btnMin->setObjectName("winBtn");
    btnClose = new QPushButton("✕");
    btnClose->setObjectName("winBtn");
    btnMin->setFixedSize(28, 24);
    btnClose->setFixedSize(28, 24);
    btnMin->setCursor(Qt::PointingHandCursor);
    btnClose->setCursor(Qt::PointingHandCursor);

    tbLayout->addWidget(dotRed);
    tbLayout->addWidget(dotYellow);
    tbLayout->addWidget(dotGreen);
    tbLayout->addSpacing(10);
    tbLayout->addWidget(titleLabel);
    tbLayout->addStretch();
    tbLayout->addWidget(btnMin);
    tbLayout->addWidget(btnClose);
    frameLayout->addWidget(titleBar);

    connect(btnClose, &QPushButton::clicked, this, &QWidget::hide);
    connect(btnMin, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(dotRed, &QLabel::linkActivated, this, &QWidget::hide); // 保留擴充彈性(未使用連結)

    // ---- 狀態列(即時同步資訊)----
    QWidget *statusBar = new QWidget(frame);
    statusBar->setObjectName("statusBar");
    QHBoxLayout *sbLayout = new QHBoxLayout(statusBar);
    sbLayout->setContentsMargins(14, 6, 14, 6);
    sbLayout->setSpacing(18);

    auto makeChip = [&](const QString &label) {
        QLabel *l = new QLabel(label);
        l->setObjectName("statusChip");
        return l;
    };
    stateChip = makeChip("狀態: 待命");
    speedChip = makeChip("速度: --");
    progressChip = makeChip("進度: --");
    elapsedChip = makeChip("耗時: 00:00:00");

    sbLayout->addWidget(stateChip);
    sbLayout->addWidget(speedChip);
    sbLayout->addWidget(progressChip);
    sbLayout->addStretch();
    sbLayout->addWidget(elapsedChip);
    frameLayout->addWidget(statusBar);

    // ---- 輸出區 ----
    output = new QTextEdit(frame);
    output->setObjectName("termOutput");
    output->setReadOnly(true);
    QFont mono("Cascadia Code");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    output->setFont(mono);
    frameLayout->addWidget(output, 1);

    // ---- 快捷單鍵指令列 ----
    QWidget *quickBar = new QWidget(frame);
    quickBar->setObjectName("quickBar");
    QHBoxLayout *qbLayout = new QHBoxLayout(quickBar);
    qbLayout->setContentsMargins(14, 8, 14, 8);
    qbLayout->setSpacing(6);
    qbLayout->addWidget(quickKeyButton("[S] Status", "s", "info"));
    qbLayout->addWidget(quickKeyButton("[P] Pause", "p", "warning"));
    qbLayout->addWidget(quickKeyButton("[R] Resume", "r", "success"));
    qbLayout->addWidget(quickKeyButton("[B] Bypass", "b", "neutral"));
    qbLayout->addWidget(quickKeyButton("[C] Checkpoint", "c", "neutral"));
    qbLayout->addWidget(quickKeyButton("[F] Finish", "f", "warning"));
    qbLayout->addWidget(quickKeyButton("[Q] Quit", "q", "danger"));
    qbLayout->addStretch();
    frameLayout->addWidget(quickBar);

    // ---- 輸入列 ----
    QWidget *inputBar = new QWidget(frame);
    inputBar->setObjectName("inputBar");
    QHBoxLayout *ibLayout = new QHBoxLayout(inputBar);
    ibLayout->setContentsMargins(14, 8, 14, 14);
    ibLayout->setSpacing(8);
    QLabel *prompt = new QLabel("$");
    prompt->setObjectName("promptLabel");
    inputLine = new QLineEdit();
    inputLine->setObjectName("termInput");
    inputLine->setPlaceholderText("輸入自訂指令後按 Enter 直接送往 hashcat...");
    ibLayout->addWidget(prompt);
    ibLayout->addWidget(inputLine, 1);
    frameLayout->addWidget(inputBar);

    connect(inputLine, &QLineEdit::returnPressed, this, &CmdTerminal::onInputReturnPressed);

    uiTimer = new QTimer(this);
    uiTimer->setInterval(1000);
    connect(uiTimer, &QTimer::timeout, this, &CmdTerminal::tickElapsed);

    applyStyle();
}

QPushButton* CmdTerminal::quickKeyButton(const QString &text, const QString &cmd, const QString &cssClass) {
    QPushButton *b = new QPushButton(text);
    b->setProperty("class", cssClass);
    b->setCursor(Qt::PointingHandCursor);
    b->setMinimumHeight(30);
    connect(b, &QPushButton::clicked, [=](){ emit commandRequested(cmd, text); });
    return b;
}

void CmdTerminal::onInputReturnPressed() {
    QString text = inputLine->text().trimmed();
    if (text.isEmpty()) return;
    emit commandRequested(text, "手動指令: " + text);
    inputLine->clear();
}

void CmdTerminal::startSession(const QString &taskName) {
    clearTerminal();
    stateChip->setText("狀態: 執行中");
    speedChip->setText("速度: --");
    progressChip->setText("進度: --");
    elapsedChip->setText("耗時: 00:00:00");
    titleLabel->setText("hashcat@rtx-accel  ~  " + taskName);
    writeLine(QString("[%1] 任務啟動: %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), taskName), "#5aa9ff");
    sessionTimer.start();
    uiTimer->start();
    show();
    raise();
    activateWindow();
}

void CmdTerminal::stopSession(bool success, const QString &reason) {
    uiTimer->stop();
    stateChip->setText(success ? "狀態: 已完成" : "狀態: 已結束");
    QString msg = success ? "[任務結束] 正常完成" : "[任務結束] " + (reason.isEmpty() ? "已終止" : reason);
    writeLine(msg, success ? "#22c55e" : "#ef4444");
}

void CmdTerminal::appendOutput(const QString &text, const QString &color) {
    if (text.isEmpty()) return;
    QString c = color;
    if (c.isEmpty()) {
        if (text.contains("Exception", Qt::CaseInsensitive) || text.contains("Error", Qt::CaseInsensitive))
            c = "#ef4444";
        else if (text.contains("Cracked", Qt::CaseInsensitive) || text.contains("Status...........: Cracked"))
            c = "#22c55e";
        else
            c = "#d9d9d9";
    }
    writeLine(text, c);
}

void CmdTerminal::setStatusInfo(const QString &speed, const QString &progress, const QString &etaText) {
    if (!speed.isEmpty())    speedChip->setText("速度: " + speed);
    if (!progress.isEmpty()) progressChip->setText("進度: " + progress);
    if (!etaText.isEmpty())  stateChip->setText("狀態: " + etaText);
}

void CmdTerminal::clearTerminal() {
    output->clear();
}

void CmdTerminal::writeLine(const QString &text, const QString &color) {
    output->setTextColor(QColor(color));
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
        output->append(line.trimmed());
    }
    output->moveCursor(QTextCursor::End);
}

void CmdTerminal::tickElapsed() {
    qint64 secs = sessionTimer.elapsed() / 1000;
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    elapsedChip->setText(QString("耗時: %1:%2:%3")
        .arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));
}

void CmdTerminal::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() <= kTitleBarHeight) {
        dragging = true;
        dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void CmdTerminal::mouseMoveEvent(QMouseEvent *event) {
    if (dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragOffset);
        event->accept();
    }
}

void CmdTerminal::mouseReleaseEvent(QMouseEvent *event) {
    dragging = false;
    event->accept();
}

void CmdTerminal::applyStyle() {
    setStyleSheet(R"(
        #terminalFrame {
            background-color: #101116;
            border: 1px solid #2e313a;
            border-radius: 12px;
        }
        #titleBar {
            background-color: #16171d;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
            border-bottom: 1px solid #24262e;
        }
        #titleLabel {
            color: #9aa0aa;
            font-size: 12px;
            font-weight: 600;
        }
        #dotRed    { background-color: #ff5f57; border-radius: 6px; }
        #dotYellow { background-color: #febc2e; border-radius: 6px; }
        #dotGreen  { background-color: #28c840; border-radius: 6px; }
        QPushButton#winBtn {
            background: transparent;
            color: #8a8f98;
            font-weight: 700;
            border-radius: 4px;
        }
        QPushButton#winBtn:hover { background-color: #2a2d36; color: #ffffff; }

        #statusBar {
            background-color: #14151a;
            border-bottom: 1px solid #24262e;
        }
        QLabel#statusChip {
            color: #b7bcc4;
            font-size: 11px;
            font-weight: 600;
            background-color: #1f212a;
            border: 1px solid #2e313a;
            border-radius: 5px;
            padding: 3px 10px;
        }

        QTextEdit#termOutput {
            background-color: #0a0b0e;
            color: #d9d9d9;
            border: none;
            padding: 10px 14px;
        }

        #quickBar {
            background-color: #14151a;
            border-top: 1px solid #24262e;
        }
        QPushButton {
            border: none;
            border-radius: 6px;
            padding: 5px 10px;
            font-weight: 600;
            color: #ffffff;
            background-color: #3a3f4b;
            font-size: 11px;
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

        #inputBar {
            background-color: #101116;
            border-bottom-left-radius: 12px;
            border-bottom-right-radius: 12px;
        }
        QLabel#promptLabel {
            color: #22c55e;
            font-weight: 700;
            font-size: 14px;
        }
        QLineEdit#termInput {
            background-color: #0a0b0e;
            border: 1px solid #2e313a;
            border-radius: 6px;
            padding: 6px 10px;
            color: #e6e6e6;
            font-family: "Cascadia Code", "Consolas";
        }
        QLineEdit#termInput:focus { border: 1px solid #5aa9ff; }
    )");
}
