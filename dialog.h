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

    void processStr(QString str);

private slots:
    void handleReadPendingDatagrams();
    void hbTimerOut();

    void debugTimerOut();

    void on_pushButtonComOpen_clicked();
    void on_pushButton_refreshCom_clicked();


    void handleSerialReadyRead();
};

#endif // DIALOG_H
