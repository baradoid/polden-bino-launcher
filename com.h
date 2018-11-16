#pragma once

#include <QObject>
#include <QSerialPort>
#include <QTimer>

typedef enum {
    idle, idleTimeout, walkIn, movingLeft, movingRight, walkOut
} TDemoModeState;

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

    void setZero();

private:            
    int recvdComPacks, bytesRecvd;    
    QString recvStr;
    QTimer demoModePeriod;

    TDemoModeState demoModeState;
    int demoModeSteps;

    void processStr(QString str);
    void sendCmd(const char* s);

    void resetDemoModeTimer();

signals:
    void newPosData(uint16_t enc1Val, uint16_t enc2Val, int dist);
    void msg(QString);

public slots:
private slots:
    void handleSerialReadyRead();    
    void handleDemoModePeriod();
};
