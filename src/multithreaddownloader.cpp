#include "multithreaddownloader.h"
#include <QNetworkRequest>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QElapsedTimer> // 1. 直接在 CPP 內部引入高精度計時器

// 2. 建立一個專屬於此 CPP 的局部計時器指標，徹底繞過 .h 的變數類型爭議
static QElapsedTimer* msvcTimer = nullptr;

MultiThreadDownloader::MultiThreadDownloader(QObject *parent) 
    : QObject(parent), activeConnections(0), totalFileSize(0), totalBytesDownloaded(0), lastReceivedBytes(0) {
    if (!msvcTimer) {
        msvcTimer = new QElapsedTimer();
    }
}

void MultiThreadDownloader::startDownload(const QString &urlStr, const QString &savePath, int threadCount) {
    url = QUrl(urlStr);
    finalSavePath = savePath;
    maxThreads = threadCount;
    parts.clear();
    totalBytesDownloaded = 0;
    activeConnections = 0;

    QNetworkRequest request(url);
    QNetworkReply *reply = sizeManager.head(request);
    connect(reply, &QNetworkReply::finished, this, &MultiThreadDownloader::onSizeCheckFinished);
}

void MultiThreadDownloader::onSizeCheckFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        emit downloadFinished(false, "無法獲取伺服器檔案大小，請檢查網路。");
        reply->deleteLater();
        return;
    }

    totalFileSize = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    reply->deleteLater();

    if (totalFileSize <= 0) {
        emit downloadFinished(false, "伺服器不支援分段下載或檔案為空。");
        return;
    }

    qint64 partSize = totalFileSize / maxThreads;
    QString tmpDir = QCoreApplication::applicationDirPath() + "/tmp_dl/";
    QDir().mkpath(tmpDir);

    for (int i = 0; i < maxThreads; ++i) {
        DownloadPart part;
        part.id = i;
        part.startByte = i * partSize;
        part.endByte = (i == maxThreads - 1) ? (totalFileSize - 1) : ((i + 1) * partSize - 1);
        part.bytesReceived = 0;
        part.isFinished = false;
        part.tmpFilename = QString("%1part_%2.tmp").arg(tmpDir).arg(i);
        parts.append(part);
    }

    msvcTimer->start(); // 使用局部安全計時器
    lastReceivedBytes = 0;

    downloadNextParts();
}

void MultiThreadDownloader::downloadNextParts() {
    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].isFinished || parts[i].bytesReceived > 0) continue;

        activeConnections++;
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        workerManagers.append(manager);

        QNetworkRequest request(url);
        QString rangeHeader = QString("bytes=%1-%2").arg(parts[i].startByte).arg(parts[i].endByte);
        request.setRawHeader("Range", rangeHeader.toUtf8());

        QNetworkReply *reply = manager->get(request);
        reply->setProperty("partId", i);

        connect(reply, &QNetworkReply::readyRead, [=]() {
            int pId = reply->property("partId").toInt();
            QFile file(parts[pId].tmpFilename);
            if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                qint64 written = file.write(reply->readAll());
                parts[pId].bytesReceived += written;
                totalBytesDownloaded += written;
                file.close();
                onPartProgress(0, 0);
            }
        });

        connect(reply, &QNetworkReply::finished, [=]() {
            int pId = reply->property("partId").toInt();
            onPartFinished(pId);
            reply->deleteLater();
            manager->deleteLater();
        });
    }
}

void MultiThreadDownloader::onPartProgress(qint64, qint64) {
    double elapsed = msvcTimer->elapsed() / 1000.0; // 使用局部安全計時器
    if (elapsed >= 0.5) {
        double speed = ((totalBytesDownloaded - lastReceivedBytes) / 1024.0 / 1024.0) / elapsed;
        double pct = ((double)totalBytesDownloaded / totalFileSize) * 100.0;
        emit progressUpdated(pct, speed);

        msvcTimer->restart(); // 使用局部安全計時器
        lastReceivedBytes = totalBytesDownloaded;
    }
}

void MultiThreadDownloader::onPartFinished(int partId) {
    parts[partId].isFinished = true;
    activeConnections--;

    bool allDone = true;
    for (const auto &part : parts) {
        if (!part.isFinished) { allDone = false; break; }
    }

    if (allDone) {
        mergeFiles();
    }
}

void MultiThreadDownloader::mergeFiles() {
    QFile finalFile(finalSavePath);
    if (!finalFile.open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, "無法建立最終合併檔案。");
        return;
    }

    for (int i = 0; i < parts.size(); ++i) {
        QFile partFile(parts[i].tmpFilename);
        if (partFile.open(QIODevice::ReadOnly)) {
            finalFile.write(partFile.readAll());
            partFile.close();
            partFile.remove();
        }
    }
    finalFile.close();

    QDir().rmdir(QCoreApplication::applicationDirPath() + "/tmp_dl/");
    emit downloadFinished(true, "32 執行緒高速下載並部署成功！");
}