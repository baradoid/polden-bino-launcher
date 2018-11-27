#include <QProcess>
#include <QFileInfo>
#include "unity.h"

Unity::Unity(QObject *parent) : QObject(parent),
    wdTimeOutSecs(0), wdEnable(false),
    p(this), fpsLimit(0), lowFpsSoftRestart(0)/*, bUnityStarted(false)*/
{    
    udpSocket = new QUdpSocket(this);

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(handleReadPendingDatagrams()));

    hbTimer.setInterval(250);
    QObject::connect(&hbTimer, SIGNAL(timeout()), this, SLOT(handleHbTimerOut()));

    connect(&wdTimer, SIGNAL(timeout()), this, SLOT(handleWdTimeout()));
    wdTimer.setInterval(1000);
    wdTimer.start();

    wdNoClientsTime.start();
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
                clientsMap[sAddrStr].hbSendTime.start();
                clientsMap[sAddrStr].bHbaRecvd = true;
                clientsMap[sAddrStr].resolution = "n/a";
                clientsMap[sAddrStr].fps = -1;
                //clientsTableModel.rowCountUpdated();
            }
        }
        else if(dData.startsWith("hba") == true){
            clientsMap[sAddrStr].bHbaRecvd = true;
            if(dData.length() > 3){
                //qDebug() << clientsMap[sAddrStr].hbSendTime.elapsed();
                clientsMap[sAddrStr].lastHbaRecvdTime = clientsMap[sAddrStr].hbSendTime.elapsed();
                //qDebug() << qPrintable(dData);
                QStringList strL =  dData.split(" ");
                strL[1].replace(',','.');
                float fps = strL[1].toFloat();

                clientsMap[sAddrStr].fps = fps;
                clientsMap[sAddrStr].resolution = strL[2];

                if(fps > fpsLimit)
                    lastOkFpsTime.restart();
                else{
                }




                //ui->tableWidgetClients->item(r, 2)->setText(QString::number(fps,'f', 1));                
                //ui->tableWidgetClients->item(r, 3)->setText(strL[2]);


                //clientsTableModel.dataUpdated();
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
        if(clientsMap[k].hbSendTime.elapsed() > 500){
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
    foreach (QString n, clientsMap.keys()) {
        TSenderInfo &si = clientsMap[n];
        if(si.bHbaRecvd == true){
            si.bHbaRecvd = false;
            udpSocket->writeDatagram("hb", si.addr, si.port);
            si.hbSendTime.restart();
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
    //QProcess p;

    QFileInfo unityFileInfo(unityPath);

    p.kill();
    //qDebug() << unityFileInfo.fileName().toLatin1();
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
    p.start(unityPathQuoted);
    //if()){
        emit msg(QString("start \"") + unityPath + "\" ... OK");
    //}
    //else{
    //    emit msg(QString("start \"") + unityPath + "\" ... FAIL");
    //}
}


void Unity::handleWdTimeout()
{    
    //qDebug() << p.state();
    if(clientsMap.isEmpty() == true){
//        ui->label_wd_noclients->show();
//        ui->label_wd_noclients_2->show();
//        ui->lineEdit_wdNoClientsTimer->show();        
//        ui->lineEdit_wdNoClientsTimer->setText(QString::number(wdNoClientsTimeSecs));
//        int wto = ui->lineEditWdTimeOutSecs->text().toInt();


        if( wdEnable &&
           (p.state() == QProcess::NotRunning) &&
           ((wdNoClientsTime.elapsed()/1000) > wdTimeOutSecs)){
          emit msg("WD: no clients in timeout. Try to restart unity build");
          restartUnityBuild();
          //bUnityStarted = true;
        }

    }
    else{
        wdNoClientsTime.start();
        int lastOkFpsSecs = lastOkFpsTime.elapsed()/1000;
        //qDebug() << "lastOkFpsTime " << lastOkFpsSecs ;

        if((lastOkFpsSecs >= 2) && ((lastOkFpsSecs %2) == 0) )
            emit msg(QString("WD: low fps detected %1 secs").arg(lastOkFpsSecs));

        if(lastOkFpsSecs  > 15/*wdTimeOutSecs*/){
            lowFpsSoftRestart++;
            if(lowFpsSoftRestart > 2){
                emit msg(QString("WD: low FPS hw restart"));
                //emit needHwRestart();
                QProcess::startDetached("shutdown /r /t 0");
            }
            else{
                emit msg(QString("WD: low FPS restart unity: count %1").arg(lowFpsSoftRestart));
                restartUnityBuild();
            }

        }
    }
    //qDebug("wd");

    //foreach (TSenderInfo si, clientsMap.values()) {
    //qInfo() << sizeof(CbDataUdp);
        //wdNoClientsTimeSecs++;
        //emit msg("watchdog: no clients in timeout. Try to restart unity build");
        //restartUnityBuild();
        //bUnityStarted = true;
    //}


//    updateUptime();
}

//////////////

//UnityClientsTableModel::UnityClientsTableModel(QObject *parent, QMap<QString, TSenderInfo> &cm) :
//    QAbstractTableModel(parent), clientsMap(cm)
//{
//}

//int UnityClientsTableModel::rowCount(const QModelIndex& parent) const
//{
//    return clientsMap.count();
//}

//int UnityClientsTableModel::columnCount(const QModelIndex & parent) const
//{
//    return 4;
//}

//QVariant UnityClientsTableModel::data(const QModelIndex& index, int role) const
//{
//    if(clientsMap.count() == 0)
//        return QVariant();
//    QVariant ret;
//    if(role == Qt::DisplayRole) {

//        QString n = clientsMap.keys()[index.row()];
//        TSenderInfo &si = clientsMap[n];

//        switch(index.column()){
//        case 0:
//            //emit QAbstractTableModel::layoutChanged;

//            return n;
//            break;
//        case 1:
//            return QString::number(si.lastHbaRecvd.elapsed());
//        case 2:
//            return "fps";
//            break;
//        case 3:
//            return "res";
//        }


//        QString str = QString("%1:%2").arg(index.row()).arg(index.column());
//        return str;
//    }
//    else if(role == Qt::TextAlignmentRole){
//        return Qt::AlignHCenter;
//    }
//    return ret;
//}

//void UnityClientsTableModel::dataUpdated()
//{
//    int rc = clientsMap.count();
//    int cc = 4;
//    emit dataChanged(index(0, 0), index(rc-1, cc-1));
//    //QModelIndex topLeft = createIndex(0,0);
//    //emit a signal to make the view reread identified data
//    //emit dataChanged(topLeft, topLeft);

//}


