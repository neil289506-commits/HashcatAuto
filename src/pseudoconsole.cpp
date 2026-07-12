#include "pseudoconsole.h"
#include <QCoreApplication>

PseudoConsoleProcess::PseudoConsoleProcess(QObject *parent) : QObject(parent) {}

PseudoConsoleProcess::~PseudoConsoleProcess() {
    terminate();
    cleanup();
}

bool PseudoConsoleProcess::start(const QString &program, const QStringList &arguments, const QString &workingDir) {
    if (running) return false;

    HANDLE ptyIn  = INVALID_HANDLE_VALUE; // ConPTY 讀取端(輸入)
    HANDLE ptyOut = INVALID_HANDLE_VALUE; // ConPTY 寫入端(輸出)

    // 兩條管線:
    // 1) inputWrite(我們) -> ptyIn(ConPTY)  : 我們寫入按鍵,ConPTY 讀走轉成主控台輸入事件
    // 2) ptyOut(ConPTY) -> outputRead(我們) : ConPTY 把子行程畫面輸出寫進來,我們讀走
    if (!CreatePipe(&ptyIn, &inputWrite, NULL, 0) ||
        !CreatePipe(&outputRead, &ptyOut, NULL, 0)) {
        emit errorOccurred("無法建立 ConPTY 管線");
        return false;
    }

    COORD size{ 120, 32 };
    HRESULT hr = CreatePseudoConsole(size, ptyIn, ptyOut, 0, &hPC);

    // ConPTY 內部已複製一份控制代碼,這兩個可以立即關閉
    CloseHandle(ptyIn);
    CloseHandle(ptyOut);

    if (FAILED(hr)) {
        emit errorOccurred("CreatePseudoConsole 失敗,需要 Windows 10 1809(組建 17763)以上版本");
        cleanup();
        return false;
    }

    STARTUPINFOEXW si{};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);

    SIZE_T attrListSize = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);
    attrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(malloc(attrListSize));
    if (!attrList || !InitializeProcThreadAttributeList(attrList, 1, 0, &attrListSize)) {
        emit errorOccurred("初始化 ProcThreadAttributeList 失敗");
        cleanup();
        return false;
    }
    si.lpAttributeList = attrList;

    if (!UpdateProcThreadAttribute(attrList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                    hPC, sizeof(HPCON), NULL, NULL)) {
        emit errorOccurred("UpdateProcThreadAttribute 失敗");
        cleanup();
        return false;
    }

    QString cmdLine = "\"" + program + "\"";
    for (const QString &a : arguments) cmdLine += " \"" + a + "\"";
    std::wstring wCmd = cmdLine.toStdWString();

    std::wstring wDir = workingDir.isEmpty()
        ? QCoreApplication::applicationDirPath().toStdWString()
        : workingDir.toStdWString();

    BOOL ok = CreateProcessW(
        NULL,
        wCmd.data(),
        NULL, NULL,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL,
        wDir.c_str(),
        &si.StartupInfo,
        &pi
    );

    if (!ok) {
        emit errorOccurred(QString("啟動失敗,錯誤碼: %1").arg(GetLastError()));
        cleanup();
        return false;
    }

    running = true;
    readerThread = std::thread(&PseudoConsoleProcess::readLoop, this);
    waiterThread = std::thread(&PseudoConsoleProcess::waitLoop, this);
    return true;
}

void PseudoConsoleProcess::readLoop() {
    char buf[4096];
    DWORD readBytes = 0;
    while (true) {
        BOOL ok = ReadFile(outputRead, buf, sizeof(buf) - 1, &readBytes, NULL);
        if (!ok || readBytes == 0) break;
        buf[readBytes] = '\0';
        QString text = QString::fromLocal8Bit(buf, static_cast<int>(readBytes));
        emit outputReceived(text);
    }
}

void PseudoConsoleProcess::waitLoop() {
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    running = false;

    // 關閉 ConPTY 讓 readLoop 的 ReadFile 自然解除阻塞、結束執行緒
    if (inputWrite != INVALID_HANDLE_VALUE) { CloseHandle(inputWrite); inputWrite = INVALID_HANDLE_VALUE; }
    if (hPC) { ClosePseudoConsole(hPC); hPC = nullptr; }

    emit finished(static_cast<int>(code));
}

void PseudoConsoleProcess::writeKey(const QString &key) {
    if (!running || inputWrite == INVALID_HANDLE_VALUE) return;
    QByteArray bytes = key.toLocal8Bit();
    DWORD written = 0;
    WriteFile(inputWrite, bytes.constData(), static_cast<DWORD>(bytes.size()), &written, NULL);
}

void PseudoConsoleProcess::terminate() {
    if (running && pi.hProcess) {
        TerminateProcess(pi.hProcess, 1);
    }
}

void PseudoConsoleProcess::cleanup() {
    if (readerThread.joinable()) readerThread.join();
    if (waiterThread.joinable()) waiterThread.join();

    if (hPC) { ClosePseudoConsole(hPC); hPC = nullptr; }
    if (inputWrite != INVALID_HANDLE_VALUE) { CloseHandle(inputWrite); inputWrite = INVALID_HANDLE_VALUE; }
    if (outputRead != INVALID_HANDLE_VALUE) { CloseHandle(outputRead); outputRead = INVALID_HANDLE_VALUE; }
    if (pi.hProcess) { CloseHandle(pi.hProcess); pi.hProcess = nullptr; }
    if (pi.hThread)  { CloseHandle(pi.hThread);  pi.hThread  = nullptr; }
    if (attrList) {
        DeleteProcThreadAttributeList(attrList);
        free(attrList);
        attrList = nullptr;
    }
}
