#ifndef CMD_H
#define CMD_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QElapsedTimer>
#include <QTimer>
#include <QMouseEvent>

// 自製美化終端機視窗
// 破解任務開始時彈出,即時同步 hashcat 的 stdout/stderr、
// 狀態列(速度/進度/耗時),並提供快捷單鍵指令輸入。
class CmdTerminal : public QDialog {
    Q_OBJECT

public:
    explicit CmdTerminal(QWidget *parent = nullptr);

    void startSession(const QString &taskName);   // 任務開始:清空畫面、啟動計時
    void stopSession(bool success, const QString &reason = QString()); // 任務結束
    void appendOutput(const QString &text, const QString &color = QString());
    void setStatusInfo(const QString &speed, const QString &progress, const QString &etaText);
    void clearTerminal();

signals:
    void commandRequested(const QString &cmd, const QString &label); // 使用者於終端機下達指令

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onInputReturnPressed();
    void tickElapsed();

private:
    void applyStyle();
    QPushButton* quickKeyButton(const QString &text, const QString &cmd, const QString &cssClass);
    void writeLine(const QString &text, const QString &color);

    // 標題列
    QWidget *titleBar;
    QLabel  *titleLabel;
    QPushButton *btnMin;
    QPushButton *btnClose;

    // 狀態列
    QLabel *stateChip;
    QLabel *speedChip;
    QLabel *progressChip;
    QLabel *elapsedChip;

    // 輸出區與輸入列
    QTextEdit *output;
    QLineEdit *inputLine;

    QElapsedTimer sessionTimer;
    QTimer *uiTimer;

    bool dragging;
    QPoint dragOffset;
    static const int kTitleBarHeight = 42;
};

#endif // CMD_H
