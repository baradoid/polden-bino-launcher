#pragma once

#include <QObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
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
    QString dlProjTaskId, dlProgTaskId;
    QMap<QNetworkReply*, QString> replyMap;

    bool webUploadTodayLogAsMultiPart(QString todayLogPath);
    void webUploadTodayLogAsRequest(QString todayLogPath);

    void processTasks(QString);
    void processTask(QString);
    void installProgram(QString);
    void uninstallProgram(QString);
    void restart(QString taskId);
    void uploadPath();


    bool isHttpRedirect(QNetworkReply *reply);

    QString saveFileName(const QUrl &url);
    bool saveToDisk(const QString &filename, QIODevice *data);

    void processLoginReply(QNetworkReply*);
    void processAliveReply(QNetworkReply*);
    void processTasksReply(QNetworkReply*);
    void processUploadLogsReply(QNetworkReply*);
    void processProgressReply(QNetworkReply*);
    bool processDownloaded(QNetworkReply*);
    void postProgress(QString taskId);

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
