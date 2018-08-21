#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QtCore/QTimer>
#include <QSerialPort>
#include <QTime>
#include <QSettings>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;
    QUdpSocket *udpSocket;
    QTimer hbTimer;
    uint16_t enc1,enc2,r;

    QTimer debugPosTimer;

    QSerialPort serial;
    int recvdComPacks;
    int bytesRecvd;
    QTime startRecvTime;

    QString recvStr;

    QSettings settings;

    uint16_t rangeThresh;

    void processStr(QString str);

    QTimer wdTimer;
    int wdNoClientsTimeSecs;
    bool bUnityStarted;

    void appendLogString(QString str);
    void appendLogFileString(QString str);

    void restartUnityBuild();
    QString formatSize(qint64 size);

private slots:
    void handleReadPendingDatagrams();
    void hbTimerOut();

    void debugTimerOut();

    void on_pushButtonComOpen_clicked();
    void on_pushButton_refreshCom_clicked();


    void handleSerialReadyRead();
    void on_pushButtonSet_clicked();
    void on_pushButtonDebugSend_clicked();
    void on_pushButtonWDTest_clicked();
    void on_pushButtonWdSelectPath_clicked();
    void on_lineEditWdTimeOutSecs_editingFinished();
    void handleWdTimeout();
    void on_checkBoxWdEnable_clicked(bool checked);
    void on_lineEditLogPath_editingFinished();
    void on_pushButtonLogSelectPath_clicked();
    void on_checkBoxLogClearIfSizeExceed_clicked();
};

#endif // DIALOG_H
