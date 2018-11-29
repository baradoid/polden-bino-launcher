#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QTimer>
#include <QTime>
#include <QSettings>
#include <QButtonGroup>
#include <QTableWidgetItem>
#include <QCheckBox>

#include "web.h"
#include "com.h"
#include "utils.h"
#include "unity.h"

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

//typedef enum { idle, connected } TWebState;


class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;
    QUdpSocket *udpSocket;
    //QTimer hbTimer;
    //uint16_t enc1,enc2,r;

    bool lastDistTreshState;

    QTimer debugPosTimer;

    int recvdComPacks;
    int bytesRecvd;
    QTime startRecvTime;

    QString recvStr;
    QSettings settings;

    TDirStruct dirStruct;

    int distanceOverThreshCnt;

    QButtonGroup *horEncSelectBG;
    QTableWidgetItem *leEnc1, *leEnc2;
    QTableWidgetItem *leEnc1Off, *leEnc2Off;    
    QCheckBox *checkEnc1Inv, *checkEnc2Inv;

    void initEncTableWidget();
    void initAppAutoStartCheckBox();
    void initComPort();
    void initRootPathControl();

    void initPathStruct(QString rootDir);

    void initUnity();

    void processStr(QString str);

    QTimer logUpdateTimer;

    QString todayLogName();
    QString todayLogPath();
    void appendLogString(QString str);
    void appendLogFileString(QString str);

    void restartUnityBuild();
    QString formatSize(qint64 size);
    void setAudioEnable(bool);
    void sendCmd(const char* s);
    QTime startTime;
    void updateUptime();

    void sendPosData();

    void updateWindowState(uint32_t);


    QTimer *writeCbParamsTimer;
    uint8_t cbWriteParamsCount;
    //TWebState webState;
//    QString guid;
    Web *web;
    Com *com;    

    void setConnectionError(QString errStr);
    void setConnectionSuccess();

    Unity *unity;

private slots:
//    void handleReadPendingDatagrams();
//    void hbTimerOut();

    //void debugTimerOut();

    void on_pushButtonComOpen_clicked();
    void on_pushButton_refreshCom_clicked();

    void on_pushButtonSet_clicked();
    void on_pushButtonDebugSend_clicked();
    //void on_lineEditLogPath_editingFinished();
    //void on_pushButtonLogSelectPath_clicked();
    void on_checkBoxLogClearIfSizeExceed_clicked();
    void on_audioOn_clicked();
    void on_audioOff_clicked();
    void on_checkBoxAudioEnableOnStartup_clicked();
    void on_pushButton_clicked();

    void handleLogUpdateTimeout();
    void on_pushButtonEncSetZero_clicked();
    void on_checkBoxRangeAlwaysOn_clicked(bool checked);    
    void handleUpdateUi();
    void handleWriteCBParamsTimeout();    
    void on_pushButtonFindWnd_clicked();

    void handleComNewPosData(uint16_t enc1Val, uint16_t enc2Val, int dist);

    //void on_pushButton_ComDemoStart_clicked();
    void on_checkBox_demoEna_clicked(bool checked);
    void on_pushButton_ComStartIsp_clicked();
};

#endif // DIALOG_H
