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
private:
    QUdpSocket *udpSocket;

    typedef struct{
        QHostAddress addr;
        int port;

        QTime lastHbaRecvd;
        bool bHbaRecvd;
    } TSenderInfo;

    QMap<QString, TSenderInfo> clientsMap;

signals:
    void msg(QString);

private slots:
    void handleReadPendingDatagrams();
    void hbTimerOut();
};

#endif // UNITY_H
