#include "dialog.h"
#include "ui_dialog.h"
#include  <QTime>


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(8050);

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(handleReadPendingDatagrams()));

    hbTimer.setInterval(250);
    QObject::connect(&hbTimer, SIGNAL(timeout()), this, SLOT(hbTimerOut()));
    hbTimer.start();


}

Dialog::~Dialog()
{
    delete ui;
}

typedef struct{
    QHostAddress addr;
    int port;

    QTime lastHbaRecvd;
    bool bHbaRecvd;
} TSenderInfo;

void Dialog::handleReadPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        QString sAddrStr = datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort());

        if(datagram.data() == "reg"){
            bool bExist = false;

            for(int r=0; r<ui->listWidgetClients->count(); r++){
                QString itText = ui->listWidgetClients->item(r)->text();
                if(itText == sAddrStr)
                    bExist = true;
            }
            if(bExist == false){
                TSenderInfo *pSI = new TSenderInfo;
                pSI->addr = datagram.senderAddress();
                pSI->port = datagram.senderPort();
                pSI->lastHbaRecvd.start();
                pSI->bHbaRecvd = true;

                ui->listWidgetClients->addItem(sAddrStr);
                ui->listWidgetClients->item(ui->listWidgetClients->count()-1)->setData(Qt::UserRole, (int)pSI);
            }
        }
        else if(datagram.data() == "hba"){
            for(int r=0; r<ui->listWidgetClients->count(); r++){
                QString itText = ui->listWidgetClients->item(r)->text();
                TSenderInfo *pSI = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
                if(itText == sAddrStr){
                    //pSI->lastHbaRecvd.restart();
                    pSI->bHbaRecvd = true;
                    qDebug() << pSI->lastHbaRecvd.elapsed();
                    break;
                }
            }


        }
        else{
            qDebug()<< datagram.senderAddress() << datagram.senderPort() << ":" << datagram.data();
        }



    }

}

void Dialog::hbTimerOut()
{
    for(int r=0; r<ui->listWidgetClients->count(); r++){
        QListWidgetItem *lwi = ui->listWidgetClients->item(r);
        QString itText = lwi->text();
        TSenderInfo *pSI = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
        //qDebug() << pSI->lastHbaRecvd.elapsed();
        if(pSI->bHbaRecvd == false){
            if(pSI->lastHbaRecvd.elapsed() > 1000){
                qDebug() << "delete client";
                ui->listWidgetClients->takeItem(r);
                delete pSI;
                break;
                //ui->listWidgetClients->removeItemWidget(lwi);

            }
        }

    }

    for(int r=0; r<ui->listWidgetClients->count(); r++){
        TSenderInfo *sndInfo = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
        //if(itText == sAddr)
        //    bExist = true;
        //if(udpSocket->isWritable()){
        //}
        if(sndInfo->bHbaRecvd = true){
            sndInfo->bHbaRecvd = false;
            udpSocket->writeDatagram("hb", sndInfo->addr, sndInfo->port);
            sndInfo->lastHbaRecvd.restart();
        }

    }


}
