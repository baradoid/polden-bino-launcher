#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QtCore/QTimer>
#include <QSerialPort>
#include <QTime>
#include <QSettings>
#include <QButtonGroup>
#include <QTableWidgetItem>
#include <QCheckBox>

namespace Ui {
class Dialog;
}

class QWidgetCust : public QWidget
{
    Q_OBJECT
public:
    explicit QWidgetCust(QWidget *parent = 0){

    }
    ~QWidgetCust(){

    }

protected:
    void mousePressEvent(QMouseEvent * event){
        //qInfo("mousePressEvent");
        emit mousePressedSignal();
    };

signals:
    void mousePressedSignal();
};

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

    uint16_t enc1Val, enc2Val;
    uint16_t enc1Offset, enc2Offset;
    int distVal;
    bool lastDistTreshState;

    int distanceOverThreshCnt;

    QButtonGroup *horEncSelectBG;
    QTableWidgetItem *leEnc1, *leEnc2;
    QTableWidgetItem *leEnc1Off, *leEnc2Off;    
    QCheckBox *checkEnc1Inv, *checkEnc2Inv;

    void initEncTableWidget();
    void initAppAutoStartCheckBox();

    void processStr(QString str);

    QTimer wdTimer, logUpdateTimer;
    int wdNoClientsTimeSecs;
    bool bUnityStarted;

    void appendLogString(QString str);
    void appendLogFileString(QString str);

    void restartUnityBuild();
    QString formatSize(qint64 size);
    void setAudioEnable(bool);
    void sendCmd(const char* s);
    QTime startTime;
    void updateUptime();

    void sendPosData();

    uint32_t comPacketsRcvd, comErrorPacketsRcvd;

    QTimer *writeCbParamsTimer;
    uint8_t cbWriteParamsCount;


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
    void on_audioOn_clicked();
    void on_audioOff_clicked();
    void on_checkBoxAudioEnableOnStartup_clicked();
    void on_pushButton_clicked();

    void handleLogUpdateTimeout();
    void on_pushButtonEncSetZero_clicked();
    void on_checkBoxRangeAlwaysOn_clicked(bool checked);    
    void handleUiUpdate();
    void handleWriteCBParamsTimeout();    
    void on_pushButtonFindWnd_clicked();
};

#endif // DIALOG_H
