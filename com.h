#pragma once

#include <QObject>
#include <QSerialPort>

class Com : public QObject
{
    Q_OBJECT
public:
    explicit Com(QObject *parent = nullptr);
    void setPort(QString);
    bool open();
    void close();

private:
    QSerialPort serial;
    int recvdComPacks, bytesRecvd;
    uint32_t comPacketsRcvd, comErrorPacketsRcvd;
    QString recvStr;

    void processStr(QString str);

    void sendCmd(const char* s);

    void setAudioEnable(bool);
    void on_audioOn_clicked();
    void on_audioOff_clicked();

signals:
    void newPosData(uint16_t enc1Val, uint16_t enc2Val, int dist);
    void msg(QString);

public slots:
private slots:
    void handleSerialReadyRead();
};
