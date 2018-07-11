#include "dialog.h"
#include "ui_dialog.h"


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(8050);

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(handleReadPendingDatagrams()));
}

Dialog::~Dialog()
{
    delete ui;
}


void Dialog::handleReadPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        qDebug() << datagram.data();
        if(QString(datagram.data()) == "reg"){
            bool bExist = false;
            QString sAddr = datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort());
            for(int r=0; r<ui->listWidgetClients->count(); r++){
                QString itText = ui->listWidgetClients->item(r)->text();
                if(itText == sAddr)
                    bExist = true;
            }
            if(bExist == false){
                ui->listWidgetClients->addItem(sAddr);
            }
        }


    }

}
