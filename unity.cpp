#include <QProcess>
#include <QFileInfo>
#include "unity.h"

Unity::Unity(QObject *parent) : QObject(parent)
{
    udpSocket = new QUdpSocket(this);

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(handleReadPendingDatagrams()));

    hbTimer.setInterval(250);
    QObject::connect(&hbTimer, SIGNAL(timeout()), this, SLOT(handleHbTimerOut()));

}

void Unity::start()
{
    if(udpSocket->bind(8050)){
        emit msg("udp socket on port 8050 bind OK");
    }
    else{
        emit msg("udp socket on port 8050 bind FAIL");
    }

        hbTimer.start();

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
            bExist = clientsMap.contains(sAddrStr);
            emit msg("UDP client reg: "+datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort()));
            if(bExist == false){
                clientsMap[sAddrStr].addr = datagram.senderAddress();
                clientsMap[sAddrStr].port = datagram.senderPort();
                clientsMap[sAddrStr].lastHbaRecvd.start();
                clientsMap[sAddrStr].bHbaRecvd = true;
            }
        }
        else if(dData.startsWith("hba") == true){
            clientsMap[sAddrStr].bHbaRecvd = true;
            if(dData.length() > 3){
                //qDebug() << qPrintable(dData);
                QStringList strL =  dData.split(" ");
                strL[1].replace(',','.');
                float fps = strL[1].toFloat();
                //ui->tableWidgetClients->item(r, 2)->setText(QString::number(fps,'f', 1));
                //ui->tableWidgetClients->item(r, 3)->setText(strL[2]);
            }
        }
        else{
            emit msg(QString("unknown msg from \"%1:%2\": \"")
                     .arg(datagram.senderAddress().toString())
                     .arg(datagram.senderPort()) + datagram.data()+  "\"");
            //qDebug()<< datagram.senderAddress() << datagram.senderPort() << ":" << datagram.data();
        }

    }
    //qDebug("handleReadPendingDatagrams end");

}

void Unity::handleHbTimerOut()
{
    //qDebug("check1");
    foreach (QString k, clientsMap.keys()) {
        if(clientsMap[k].lastHbaRecvd.elapsed() > 500){
            if(clientsMap[k].bHbaRecvd == false){
                emit msg(QString("UDP client %1:%2 no answer in timeout 500 ms. delete. ")
                         .arg(k).arg(QString::number(clientsMap[k].port)));
                //qDebug() << "delete client";
                clientsMap.remove(k);
                break;
                //ui->listWidgetClients->removeItemWidget(lwi);
            }
        }
    }

    //qDebug("check2");
    foreach (TSenderInfo si, clientsMap.values()) {
        if(si.bHbaRecvd == true){
            si.bHbaRecvd = false;
            udpSocket->writeDatagram("hb", si.addr, si.port);
            si.lastHbaRecvd.restart();
        }

    }
    //qDebug("check2 end");
}

void Unity::sendPosData(uint16_t val1, uint16_t val2, int16_t distVal)
{
    CbDataUdp cbdata;
//    if(horEncSelectBG->checkedId() == 0){
        cbdata.pos1 = (int16_t)val1&0x1fff;
        cbdata.pos2 = (int16_t)val2&0x1fff;
//    }
//    else if(horEncSelectBG->checkedId() == 1){
//        cbdata.pos1 = (int16_t)((com->enc2Val-com->enc2Offset)&0x1fff);
//        cbdata.pos2 = (int16_t)((com->enc1Val-com->enc1Offset)&0x1fff);
//    }
//    if(checkEnc1Inv->isChecked())
//        cbdata.pos1 = 0x1fff - cbdata.pos1;
//    if(checkEnc2Inv->isChecked())
//        cbdata.pos2 = 0x1fff - cbdata.pos2;
//    //cbdata.distance = (int16_t)dist;
    bool distThreshExceed = false;//ui->checkBoxRangeAlwaysOn->isChecked() ||(com->distVal<com->rangeThresh);
    cbdata.rangeThresh = distThreshExceed? 1:0;

//    ui->checkBoxThreshExcess->setChecked(distThreshExceed);

//    if((distThreshExceed == true) && (lastDistTreshState == false)){
//        distanceOverThreshCnt ++ ;
//        ui->lineEditRangeTreshExceedCount->setText(QString::number(distanceOverThreshCnt));
//    }
//    lastDistTreshState = distThreshExceed;


    foreach (TSenderInfo si, clientsMap.values()) {
        //qInfo() << sizeof(CbDataUdp);
        udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), si.addr, si.port);
    }

}

void Unity::sendCbData(CbDataUdp &cbData)
{
    foreach (TSenderInfo si, clientsMap.values()) {
        //qInfo() << sizeof(CbDataUdp);
        udpSocket->writeDatagram((const char*)&cbData, sizeof(CbDataUdp), si.addr, si.port);
    }
}

void Unity::restartUnityBuild()
{
    QProcess p;

    QFileInfo unityFileInfo(unityPath);

    qDebug() << unityFileInfo.fileName().toLatin1();
    p.start("taskkill /IM " + unityFileInfo.fileName());
    if(p.waitForFinished()){
        emit msg("kill \"" + unityFileInfo.fileName() + "\" ... OK");
    }
    else{
        emit msg("kill \"" + unityFileInfo.fileName() + "\" ... FAIL");
    }
    //QString str = ui->lineEditBuildPath->text();
    if(unityPath.isEmpty() == true){
        emit msg("Path empty. Nothing to start.");
        return;
    }
    QString unityPathQuoted = "\"" + unityPath + "\"";
    if(p.startDetached(unityPathQuoted)){
        emit msg(QString("start \"") + unityPath + "\" ... OK");
    }
    else{
        emit msg(QString("start \"") + unityPath + "\" ... FAIL");
    }
}
