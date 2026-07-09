#ifndef MULTITHREADDOWNLOADER_H
#define MULTITHREADDOWNLOADER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTime>

struct DownloadPart {
    int id;
    qint64 startByte;
    qint64 endByte;
    qint64 bytesReceived;
    bool isFinished;
    QString tmpFilename;
};

class MultiThreadDownloader : public QObject {
    Q_OBJECT
public:
    explicit MultiThreadDownloader(QObject *parent = nullptr);
    void startDownload(const QString &urlStr, const QString &savePath, int threadCount = 32);

signals:
    void progressUpdated(double percentage, double speedMbps);
    void downloadFinished(bool success, QString msg);

private slots:
    void onSizeCheckFinished();
    void onPartFinished(int partId);
    void onPartProgress(qint64 bytesRead, qint64 totalBytes);

private:
    void downloadNextParts();
    void mergeFiles();

    QUrl url;
    QString finalSavePath;
    int maxThreads;
    int activeConnections;
    qint64 totalFileSize;
    
    QList<DownloadPart> parts;
    QNetworkAccessManager sizeManager;
    QList<QNetworkAccessManager*> workerManagers;
    
    QTime speedTimer;
    qint64 lastReceivedBytes;
    qint64 totalBytesDownloaded;
};

#endif // MULTITHREADDOWNLOADER_H