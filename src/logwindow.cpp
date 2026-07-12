#include "logwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QTextCursor>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QFile>
#include <QTextStream>

LogWindow::LogWindow(QWidget *parent)
    : QDialog(parent), autoScroll(true) {

    setWindowTitle("獨立日誌視窗");
    setModal(false);
    resize(760, 520);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    // ---- 標題與統計 ----
    QHBoxLayout *headerLayout = new QHBoxLayout();
    titleLabel = new QLabel("日誌視窗");
    titleLabel->setObjectName("logTitle");
    countLabel = new QLabel("共 0 筆");
    countLabel->setObjectName("logCount");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(countLabel);
    root->addLayout(headerLayout);

    // ---- 工具列:篩選 / 自動捲動 / 匯出 / 清空 ----
    QHBoxLayout *toolLayout = new QHBoxLayout();
    toolLayout->setSpacing(8);
    filterEdit = new QLineEdit();
    filterEdit->setObjectName("filterEdit");
    filterEdit->setPlaceholderText("輸入關鍵字即時篩選日誌 (例如 Status / Error / Cracked)...");
    connect(filterEdit, &QLineEdit::textChanged, this, &LogWindow::onFilterChanged);

    btnAutoScroll = new QPushButton("自動捲動: 開");
    btnAutoScroll->setObjectName("toggleBtn");
    btnAutoScroll->setCheckable(true);
    btnAutoScroll->setChecked(true);
    btnAutoScroll->setCursor(Qt::PointingHandCursor);
    connect(btnAutoScroll, &QPushButton::toggled, this, &LogWindow::onAutoScrollToggled);

    btnExport = new QPushButton("匯出");
    btnExport->setProperty("class", "info");
    btnExport->setCursor(Qt::PointingHandCursor);
    connect(btnExport, &QPushButton::clicked, this, &LogWindow::onExportClicked);

    btnClear = new QPushButton("清空");
    btnClear->setProperty("class", "neutral");
    btnClear->setCursor(Qt::PointingHandCursor);
    connect(btnClear, &QPushButton::clicked, this, &LogWindow::onClearClicked);

    toolLayout->addWidget(filterEdit, 1);
    toolLayout->addWidget(btnAutoScroll);
    toolLayout->addWidget(btnExport);
    toolLayout->addWidget(btnClear);
    root->addLayout(toolLayout);

    // ---- 輸出區 ----
    view = new QTextEdit();
    view->setObjectName("logView");
    view->setReadOnly(true);
    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    view->setFont(mono);
    root->addWidget(view, 1);

    applyStyle();
}

void LogWindow::startSession(const QString &taskName) {
    clearLog();
    titleLabel->setText("日誌視窗  -  " + taskName);
    appendLine(QString("[%1] 任務開始,獨立日誌視窗已同步").arg(QDateTime::currentDateTime().toString("HH:mm:ss")), "#5aa9ff");
    show();
    raise();
    activateWindow();
}

void LogWindow::appendLine(const QString &text, const QString &color) {
    if (text.trimmed().isEmpty()) return;

    QString c = color.isEmpty() ? "#d9d9d9" : color;
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        entries.append({ trimmed, c });
    }
    countLabel->setText(QString("共 %1 筆").arg(entries.size()));

    // 若目前有篩選字串,且新行不符合篩選,則不即時畫出(但仍保留在歷史紀錄中)
    QString filter = filterEdit->text().trimmed();
    if (filter.isEmpty()) {
        for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;
            view->setTextColor(QColor(c));
            view->append(trimmed);
        }
        if (autoScroll) view->moveCursor(QTextCursor::End);
    } else {
        rebuildView();
    }
}

void LogWindow::clearLog() {
    entries.clear();
    view->clear();
    countLabel->setText("共 0 筆");
}

void LogWindow::onFilterChanged(const QString &) {
    rebuildView();
}

void LogWindow::rebuildView() {
    QString filter = filterEdit->text().trimmed();
    view->clear();
    int shown = 0;
    for (const LogEntry &e : entries) {
        if (!filter.isEmpty() && !e.text.contains(filter, Qt::CaseInsensitive)) continue;
        view->setTextColor(QColor(e.color));
        view->append(e.text);
        shown++;
    }
    countLabel->setText(filter.isEmpty()
        ? QString("共 %1 筆").arg(entries.size())
        : QString("符合 %1 / 共 %2 筆").arg(shown).arg(entries.size()));
    if (autoScroll) view->moveCursor(QTextCursor::End);
}

void LogWindow::onAutoScrollToggled(bool checked) {
    autoScroll = checked;
    btnAutoScroll->setText(checked ? "自動捲動: 開" : "自動捲動: 關");
    if (checked) view->moveCursor(QTextCursor::End);
}

void LogWindow::onExportClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "匯出獨立日誌", "hashcat_full_log.txt", "文字檔案 (*.txt)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const LogEntry &e : entries) out << e.text << "\n";
        file.close();
        QMessageBox::information(this, "成功", "日誌已匯出至:" + fileName);
    } else {
        QMessageBox::warning(this, "錯誤", "無法寫入檔案,請檢查權限設定");
    }
}

void LogWindow::onClearClicked() {
    clearLog();
}

void LogWindow::applyStyle() {
    setStyleSheet(R"(
        QDialog {
            background-color: #1b1d23;
            color: #e6e6e6;
            font-family: "Microsoft JhengHei", "Segoe UI";
            font-size: 13px;
        }
        QLabel#logTitle {
            font-size: 15px;
            font-weight: 700;
            color: #ffffff;
        }
        QLabel#logCount {
            font-size: 11px;
            color: #8a8f98;
        }
        QLineEdit#filterEdit {
            background-color: #14151a;
            border: 1px solid #33363f;
            border-radius: 6px;
            padding: 6px 10px;
            color: #e6e6e6;
        }
        QLineEdit#filterEdit:focus { border: 1px solid #5aa9ff; }

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

        QPushButton#toggleBtn {
            background-color: #2e313a;
        }
        QPushButton#toggleBtn:checked {
            background-color: #16a34a;
        }

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
