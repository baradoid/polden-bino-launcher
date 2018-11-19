#pragma once

#include <QObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "utils.h"

class Web : public QObject
{
    Q_OBJECT
public:
    explicit Web(QObject *parent = nullptr);

    QString wbUser, wbPass, wbPath;

    TDirStruct *dirStruct;

    void start();

private:
    QNetworkAccessManager nam;
    QString guid;

    bool webUploadTodayLogAsMultiPart(QString todayLogPath);
    void webUploadTodayLogAsRequest(QString todayLogPath);

    void processTasks(QString);
    void processTask(QString);
    void installProgram(QString);
    void uninstallProgram(QString);
    void restart();


    bool isHttpRedirect(QNetworkReply *reply);

    QString saveFileName(const QUrl &url);
    bool saveToDisk(const QString &filename, QIODevice *data);

    void processLoginReply(QNetworkReply*);
    void processAliveReply(QNetworkReply*);
    void processTasksReply(QNetworkReply*);
    void processUploadLogsReply(QNetworkReply*);
    void processDownloaded(QNetworkReply*);

    void cleanZipTemporaryFiles();

signals:
    void msg(QString);
    void connError(QString msg);
    void connSuccess();

public slots:       
    void handleNamReplyFinished(QNetworkReply*);

    void handleWbLoginTimeout();
    void handlePostAliveTimeout();
    void handlePostTasksTimeout();

};
