#ifndef PSEUDOCONSOLE_H
#define PSEUDOCONSOLE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <windows.h>
#include <thread>

// 使用 Windows ConPTY(虛擬終端)啟動子行程。
//
// hashcat 在 Windows 上是透過主控台 API(_kbhit / _getch / ReadConsoleInput)
// 直接讀取「自己所連接的主控台」的鍵盤緩衝區,而不是讀取 stdin 管線。
// QProcess 預設是用匿名管線(anonymous pipe)接 stdin/stdout,
// 這種管線並不是主控台,hashcat 完全偵測不到,
// 這就是為什麼先前用 process->write() 送 "s\r\n" 完全沒有反應。
//
// ConPTY 會建立一個「看起來像真正主控台」的偽終端,子行程(hashcat)
// 誤以為自己連接到互動式終端機,因此其單鍵執行期指令
// (s/p/r/b/c/f/q) 才能正常被觸發;我們則透過管線把畫面輸出讀回來、
// 把按鍵寫進去,兩邊都可控,且行為完全比照真人操作終端機。
class PseudoConsoleProcess : public QObject {
    Q_OBJECT
public:
    explicit PseudoConsoleProcess(QObject *parent = nullptr);
    ~PseudoConsoleProcess() override;

    // program: 可執行檔路徑(例如 "hashcat.exe")
    // arguments: 命令列參數
    // workingDir: 工作目錄,留空則使用目前應用程式目錄
    bool start(const QString &program, const QStringList &arguments, const QString &workingDir = QString());

    // 送出單鍵指令,例如 "s"、"p"、"q"...
    // 完全比照真人在終端機上按下該按鍵,不需要加換行、不需要 Enter。
    void writeKey(const QString &key);

    void terminate();  // 強制結束子行程
    bool isRunning() const { return running; }

signals:
    void outputReceived(const QString &text); // 子行程畫面輸出(即時同步)
    void finished(int exitCode);              // 子行程結束
    void errorOccurred(const QString &message);

private:
    void cleanup();
    void readLoop();   // 背景執行緒:持續讀取子行程畫面輸出
    void waitLoop();   // 背景執行緒:等待子行程結束

    HPCON hPC = nullptr;
    PROCESS_INFORMATION pi{};
    LPPROC_THREAD_ATTRIBUTE_LIST attrList = nullptr;

    HANDLE inputWrite = INVALID_HANDLE_VALUE;  // 我們寫入端:寫入 = 送出按鍵
    HANDLE outputRead = INVALID_HANDLE_VALUE;  // 我們讀取端:讀取 = 畫面輸出

    std::thread readerThread;
    std::thread waiterThread;

    volatile bool running = false;
};

#endif // PSEUDOCONSOLE_H
