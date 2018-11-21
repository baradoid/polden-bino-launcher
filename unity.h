#ifndef UNITY_H
#define UNITY_H

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QTime>
#include <QMap>

class Unity : public QObject
{
    Q_OBJECT
public:
    explicit Unity(QObject *parent = nullptr);

    void sendPosData(uint16_t val1, uint16_t val2, int16_t distVal);

private:
    QUdpSocket *udpSocket;

    typedef struct{
        QHostAddress addr;
        int port;

        QTime lastHbaRecvd;
        bool bHbaRecvd;
    } TSenderInfo;

    QMap<QString, TSenderInfo> clientsMap;

#pragma pack(push,1)
typedef struct{
    uint16_t pos1;
    uint16_t pos2;
    uint8_t rangeThresh;
} CbDataUdp;
#pragma pack(pop)

signals:
    void msg(QString);

private slots:
    void handleReadPendingDatagrams();
    void hbTimerOut();
};

#endif // UNITY_H
