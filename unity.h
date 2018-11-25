#pragma once

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QTime>
#include <QMap>
#include <QTimer>
#include <QAbstractTableModel>
#include <QProcess>


typedef struct{
    QHostAddress addr;
    int port;
    QTime hbSendTime;
    int lastHbaRecvdTime;
    bool bHbaRecvd;
    QString resolution;
    float fps;
} TSenderInfo;

//class UnityClientsTableModel :  public QAbstractTableModel
//{
//    Q_OBJECT
//public:
//    explicit UnityClientsTableModel(QObject *parent, QMap<QString, TSenderInfo> &cm);
//    int rowCount(const QModelIndex& parent = QModelIndex()) const;
//    int columnCount(const QModelIndex& parent = QModelIndex()) const;
//    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
//    //inline void setMap(QMap<QString, TSenderInfo> *m) { clientsMap = m; }
//    void dataUpdated();
//    void rowCountUpdated() { emit layoutChanged(); }

//private:
//    QMap<QString, TSenderInfo> &clientsMap;
//};


class Unity : public QObject
{
    Q_OBJECT
public:    
    explicit Unity(QObject *parent = nullptr);


    //UnityClientsTableModel clientsTableModel;

    QMap<QString, TSenderInfo> clientsMap;

    void start();
    void sendPosData(uint16_t val1, uint16_t val2, int16_t distVal);


#pragma pack(push,1)
typedef struct{
    uint16_t pos1;
    uint16_t pos2;
    uint8_t rangeThresh;
} CbDataUdp;
#pragma pack(pop)

    void sendCbData(CbDataUdp &cbData);
    void restartUnityBuild();
    void setUnityPath(QString p){unityPath = p;}

    void setWdTimeOutSecs(int to){wdTimeOutSecs = to;}
    void setFpsLimit(int fpsLim){fpsLimit = fpsLim;}
    void setWdEnable(bool bEna){ wdEnable = bEna;}
    QTime wdNoClientsTime;

private:
    QProcess p;
    QString unityPath;
    QUdpSocket *udpSocket;

    QTimer wdTimer, hbTimer;

    int wdTimeOutSecs;
    bool wdEnable;
    int fpsLimit;
    QTime lastOkFpsTime;

    //int wdNoClientsTimeSecs;
    //bool bUnityStarted;

signals:
    void msg(QString);

private slots:
    void handleReadPendingDatagrams();
    void handleHbTimerOut();
    void handleWdTimeout();
};

