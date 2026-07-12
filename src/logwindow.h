#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QList>

// 獨立的日誌視窗
// 破解任務開始時彈出,與主視窗、CmdTerminal 各自獨立、
// 即時同步全部輸出,並提供關鍵字篩選、自動捲動、匯出功能。
class LogWindow : public QDialog {
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);

    void startSession(const QString &taskName); // 任務開始:清空、更新標題、顯示視窗
    void appendLine(const QString &text, const QString &color = QString()); // 即時同步一行(或多行)輸出
    void clearLog();

private slots:
    void onFilterChanged(const QString &text);
    void onExportClicked();
    void onClearClicked();
    void onAutoScrollToggled(bool checked);

private:
    void applyStyle();
    void rebuildView(); // 依目前篩選字串重新繪製輸出區

    struct LogEntry {
        QString text;
        QString color;
    };

    QLabel *titleLabel;
    QLabel *countLabel;
    QLineEdit *filterEdit;
    QTextEdit *view;
    QPushButton *btnExport;
    QPushButton *btnClear;
    QPushButton *btnAutoScroll;

    bool autoScroll;
    QList<LogEntry> entries; // 保留完整歷史紀錄,供篩選時重新繪製
};

#endif // LOGWINDOW_H
