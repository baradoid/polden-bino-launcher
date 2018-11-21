#include "unity.h"

Unity::Unity(QObject *parent) : QObject(parent)
{
    udpSocket = new QUdpSocket(this);
    if(udpSocket->bind(8050)){
        emit msg("udp socket on port 8050 bind OK");
    }
    else{
        emit msg("udp socket on port 8050 bind FAIL");
    }

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(handleReadPendingDatagrams()));


}

void Unity::handleReadPendingDatagrams()
{
    //qDebug("handleReadPendingDatagrams");
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        if(datagram.isValid() == false)
            continue;

        QString sAddrStr = datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort());

        QString dData(datagram.data());
        if(dData == "reg"){
            bool bExist = false;

            bExist = clientsMap.keys().contains(sAddrStr);

            emit msg("UDP client reg: "+datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort()));
            if(bExist == false){
//                TSenderInfo *pSI = new TSenderInfo;
//                pSI->addr = datagram.senderAddress();
//                pSI->port = datagram.senderPort();
//                pSI->lastHbaRecvd.start();
//                pSI->bHbaRecvd = true;

////                ui->listWidgetClients->addItem(sAddrStr);
////                ui->listWidgetClients->item(ui->listWidgetClients->count()-1)->setData(Qt::UserRole, (int)pSI);

//                int curRowInd = ui->tableWidgetClients->rowCount();
//                ui->tableWidgetClients->setRowCount(curRowInd+1);
//                QTableWidgetItem *twi = new QTableWidgetItem(sAddrStr);
//                twi->setData(Qt::UserRole, (int)pSI);
//                twi->setTextAlignment(Qt::AlignCenter);
//                ui->tableWidgetClients->setItem(curRowInd, 0, twi);
//                twi = new QTableWidgetItem("n/a");
//                twi->setTextAlignment(Qt::AlignCenter);
//                ui->tableWidgetClients->setItem(curRowInd, 1, twi);
//                twi = new QTableWidgetItem("n/a");
//                twi->setTextAlignment(Qt::AlignCenter);
//                ui->tableWidgetClients->setItem(curRowInd, 2, twi);
//                twi = new QTableWidgetItem("n/a");
//                twi->setTextAlignment(Qt::AlignCenter);
//                ui->tableWidgetClients->setItem(curRowInd, 3, twi);
//                ui->tableWidgetClients->resizeColumnsToContents();
            }
        }
        else if(dData.startsWith("hba") == true){
//            for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
//                QString itText = ui->tableWidgetClients->item(r, 0)->text();
//                TSenderInfo *pSI = (TSenderInfo*)ui->tableWidgetClients->item(r, 0)->data(Qt::UserRole).toInt();
//                if(itText == sAddrStr){
//                    //pSI->lastHbaRecvd.restart();
//                    pSI->bHbaRecvd = true;
//                    //qDebug() << pSI->lastHbaRecvd.elapsed();
//                    ui->tableWidgetClients->item(r, 1)->setText(QString::number(pSI->lastHbaRecvd.elapsed()));
//                    if(dData.length() > 3){
//                        //qDebug() << qPrintable(dData);
//                        QStringList strL =  dData.split(" ");
//                        strL[1].replace(',','.');
//                        float fps = strL[1].toFloat();
//                        ui->tableWidgetClients->item(r, 2)->setText(QString::number(fps,'f', 1));
//                        ui->tableWidgetClients->item(r, 3)->setText(strL[2]);
//                    }
//                    ui->tableWidgetClients->resizeColumnsToContents();
//                    break;
//                }
//            }

        }
        else{
            qDebug()<< datagram.senderAddress() << datagram.senderPort() << ":" << datagram.data();
        }



    }
    //qDebug("handleReadPendingDatagrams end");

}

void Unity::hbTimerOut()
{
//    //qDebug("check1");
//    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
//        //QListWidgetItem *lwi = ui->listWidgetClients->item(r);
//        //QString itText = lwi->text();
//        TSenderInfo *pSI = (TSenderInfo*)ui->tableWidgetClients->item(r, 0)->data(Qt::UserRole).toInt();
//        //qDebug() << pSI->lastHbaRecvd.elapsed();
//        //qDebug("check1 %d", pSI->lastHbaRecvd.elapsed() );
//        if(pSI->lastHbaRecvd.elapsed() > 500){
//            if(pSI->bHbaRecvd == false){
//                emit msg(QString("UDP client %1:%2 no answer in timeout 500 ms. delete. ").arg(pSI->addr.toString()).arg(QString::number(pSI->port)));
//                //qDebug() << "delete client";
//                //ui->listWidgetClients->takeItem(r);

//                ui->tableWidgetClients->removeRow(r);
//                delete pSI;
//                break;
//                //ui->listWidgetClients->removeItemWidget(lwi);

//            }
//        }

//    }

//    //qDebug("check2");
//    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
//        TSenderInfo *sndInfo = (TSenderInfo*)ui->tableWidgetClients->item(r,0)->data(Qt::UserRole).toInt();
//        //if(itText == sAddr)
//        //    bExist = true;
//        //if(udpSocket->isWritable()){
//        //}
//        if(sndInfo->bHbaRecvd == true){
//            sndInfo->bHbaRecvd = false;
//            udpSocket->writeDatagram("hb", sndInfo->addr, sndInfo->port);
//            sndInfo->lastHbaRecvd.restart();
//        }

//    }
//    //qDebug("check2 end");

}
