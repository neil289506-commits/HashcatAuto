Hashcat Ultra Professional v2026 

這是一個由 Neil 開發，基於 C++ 與 Qt 6 框架的高級 Hashcat 管理工具。專為 Windows 系統優化，旨在提供一個高效、專業且自動化的密碼恢復介面。
🚀 核心特色

    GPU 效能調優：預設針對 NVIDIA RTX 3050 等系列顯卡進行優化，強制啟用 -w 3 (High Workload Profile) 以發揮最大算力。

    極簡自動化部署：若執行目錄缺失核心組件，系統將自動從官網下載 Hashcat 6.2.6 並透過 PowerShell 無損解壓部署。

    專業日誌系統：專為資安人員設計的 14px 粗體 黑底 UI 介面，確保在大量數據滾動時仍具備極高可讀性。

    嚴格資源封裝：專案設定了「強制圖標編譯」邏輯。編譯時若缺失 app.ico，MSVC 將報出 RC2135 錯誤並停止構建，確保發布版本的一致性。

    多模式支援：整合字典攻擊 (-a 0)、組合攻擊 (-a 1) 及自定義掩碼攻擊 (-a 3)。

⚖️ 免責聲明 (Disclaimer)

    [!CAUTION]
    法律警告：本工具僅供合法的資安教育、授權滲透測試及個人密碼恢復使用。嚴禁將本軟體用於任何非法目的。使用者須自行承擔因不當使用所產生的法律責任，開發者對任何形式的損害概不負責。啟動程式即代表同意此條款。

🛠️ 開發環境與構建

    OS: Windows 10/11

    Compiler: MSVC 2022 (v143)

    Framework: Qt 6.11.0

    Standard: C++17

編譯步驟

    確保專案根目錄擁有 app.ico 檔案。

    開啟 Developer PowerShell for VS 2022 並執行：
    PowerShell

    mkdir build
    cd build
    qmake ..\Qt.pro
    nmake

    使用 windeployqt 部署依賴函式庫。


這是一個由 Neil 開發，基於 C++ 與 Qt 6 框架的高級 Hashcat 管理工具。專為 Windows 系統優化，旨在提供一個高效、專業且自動化的密碼恢復介面。
🚀 核心特色

    GPU 效能調優：預設針對 NVIDIA RTX 3050 等系列顯卡進行優化，強制啟用 -w 3 (High Workload Profile) 以發揮最大算力。

    極簡自動化部署：若執行目錄缺失核心組件，系統將自動從官網下載 Hashcat 6.2.6 並透過 PowerShell 無損解壓部署。

    專業日誌系統：專為資安人員設計的 14px 粗體 黑底 UI 介面，確保在大量數據滾動時仍具備極高可讀性。

    嚴格資源封裝：專案設定了「強制圖標編譯」邏輯。編譯時若缺失 app.ico，MSVC 將報出 RC2135 錯誤並停止構建，確保發布版本的一致性。

    多模式支援：整合字典攻擊 (-a 0)、組合攻擊 (-a 1) 及自定義掩碼攻擊 (-a 3)。

⚖️ 免責聲明 (Disclaimer)

    [!CAUTION]
    法律警告：本工具僅供合法的資安教育、授權滲透測試及個人密碼恢復使用。嚴禁將本軟體用於任何非法目的。使用者須自行承擔因不當使用所產生的法律責任，開發者對任何形式的損害概不負責。啟動程式即代表同意此條款。

🛠️ 開發環境與構建

    OS: Windows 10/11

    Compiler: MSVC 2022 (v143)

    Framework: Qt 6.11.0

    Standard: C++17
