#pragma once

#include <QObject>
#include <QSerialPort>
#include <QTimer>

class Com : public QSerialPort
{
    Q_OBJECT
public:
    explicit Com(QObject *parent = nullptr);    
    bool open();        

    void setAudioEnable(bool);

    int16_t enc1Val, enc2Val, distVal;
    uint16_t enc1Offset, enc2Offset;
    //int distVal;
    uint16_t rangeThresh;
    uint32_t comPacketsRcvd, comErrorPacketsRcvd;
    uint32_t demoCycleCount;

    QString firmwareVer;

    enum TDemoModeState {
        idle, idleTimeout, walkIn, movingLeft, movingRight, walkOut
    };

    Q_ENUM(TDemoModeState)

    int demoModeSteps;
    QTimer demoModePeriod;
    TDemoModeState demoModeState;

    void setZero();
    void startDemo();
    void stopDemo();
    void enableDemo(bool);

    enum TEnterIspState{
        idleIspState,
        waitSynchronizedIspState,
        waitSynchronizedOkIspState,
        waitSynchronizedClkOKIspState,
        waitUnlockOkIspState,
        waitGoOkIspState
    };
    Q_ENUM(TEnterIspState)

    TEnterIspState ispState;
    void startIsp();

private:            
    int recvdComPacks, bytesRecvd;    
    QString recvStr;    
    QTimer checkIspTimer;

    void processStr(QString str);
    void sendCmd(const char* s);

    void sendIspGoCmd();
    void resetDemoModeTimer();

signals:
    void newPosData(uint16_t enc1Val, uint16_t enc2Val, int dist);
    void msg(QString);

private slots:
    void handleSerialReadyRead();    
    void handleDemoModePeriod();
    void handleCheckIspRunning();

};
