#include "com.h"
#include <QThread>
#include <QDebug>

Com::Com(QObject *parent) : QSerialPort(parent),
    enc1Val(-1), enc2Val(-1), distVal(-1),
    enc1Offset(0), enc2Offset(0),
    comPacketsRcvd(0), comErrorPacketsRcvd(0),
    demoModePeriod(this), demoModeState(idle),
    checkIspTimer(this),ispState(idleIspState),
    demoCycleCount(0)
{
    setBaudRate(115200);
    connect(this, SIGNAL(readyRead()),
            this, SLOT(handleSerialReadyRead()));


//    connect(&demoModeTimeOut, &QTimer::timeout, [=](){
//        emit msg("human interface timeout. Start demo mode.");
//        demoModeState = idle;
//        demoModePeriod.start();

//    });
//    demoModeTimeOut.setSingleShot(true);
//    demoModeTimeOut.setInterval(/*120*/ 15*1000);
//    demoModeTimeOut.start();

    connect(&checkIspTimer, SIGNAL(timeout()),
                    this, SLOT(handleCheckIspRunning()));


    connect(&demoModePeriod, SIGNAL(timeout()),
            this, SLOT(handleDemoModePeriod()));

    resetDemoModeTimer();
}

bool Com::open()
{
    bool ret = QSerialPort::open(QIODevice::ReadWrite);
    if(ret == true){
        sendCmd("ver\r\n");
        //checkIspTimer.setSingleShot(true);
        //checkIspTimer.setInterval(500);
        //checkIspTimer.start();
    }
    return ret;
}

void Com::processStr(QString str)
{
    recvdComPacks++;

    if(ispState == idleIspState){
        if(str.startsWith("compile time:") == true){
            str.remove("compile time:");
            str.remove("\r\n");
            firmwareVer = str;
        }
        else if(str.startsWith("sysclk") == true){

        }
        else if(str.startsWith("ADC") == true){

        }
        else{
            if(str.length() != 17){
                comErrorPacketsRcvd++;
            }

            QStringList strList = str.split(" ");
            if(strList.size() >= 3){
                int xPos1 = strList[0].toInt(Q_NULLPTR, 16);
                int xPos2 = strList[1].toInt(Q_NULLPTR, 16);
                int dist = strList[2].toInt(Q_NULLPTR, 10);
                //float temp = strList[2].toInt(Q_NULLPTR, 10)/10.;
                //ui->lineEditEnc1->setText(QString::number(xPos1));
                //ui->lineEditEnc2->setText(QString::number(xPos2));
                //ui->lineEditTerm1->setText(QString::number(temp));
                //qDebug() << xPos1 << xPos2;

                //rangeThresh = ui->lineEditRangeThresh->text().toInt();

                //qInfo("%d %d %d", distVal, rangeThresh, dist<rangeThresh);

//                if(dist < 30)
//                    resetDemoModeTimer();

                if(demoModeState == idle){
                    enc1Val = xPos1;
                    enc2Val = xPos2;
                    distVal = dist;
                }

                //enc1Val = xPos1;
                //enc2Val = xPos2;
                //distVal = dist;

                //sendPosData();
                emit newPosData(xPos1, xPos2, dist);
            }
        }
    }
    else{
        //qDebug()<< ispState << ": " << qPrintable(str);
        switch (ispState) {
        case waitSynchronizedIspState:
            if(str == "enter ISP OK\n"){
                QThread::msleep(100);
                sendCmd("?");
            }
            else if(str == "Synchronized\r\n"){
                //qDebug("\"Synchronized\" recvd");
                emit msg("ISP running");
                checkIspTimer.stop();
                sendCmd("Synchronized\r\n");
                //sendIspGoCmd();
                ispState = waitSynchronizedOkIspState;
            }
            break;
        case waitSynchronizedOkIspState:
            if(str == "OK\r\n"){
                //qDebug("\"Synchronized OK\" recvd");
                sendCmd("4000\r\n");
                ispState = waitSynchronizedClkOKIspState;
            }
            break;
        case waitSynchronizedClkOKIspState:
            if(str == "OK\r\n"){
                sendCmd("U 23130\r\n");
                ispState = waitUnlockOkIspState;
            }
            break;
        case waitUnlockOkIspState:
            if(str == "0\r\n"){
                sendCmd("G 0 T\r\n");
                ispState = waitGoOkIspState;
            }
            break;
        case waitGoOkIspState:
            if(str == "0\r\n"){
                ispState = idleIspState;
                emit msg(QString("user fwm running"));
            }
            break;


        default:
            break;
        }
    }
}


void Com::handleSerialReadyRead()
{
    QByteArray ba = readAll();

    bytesRecvd += ba.length();

    for(int i=0; i<ba.length(); i++){
        recvStr.append((char)ba[i]);
        if(ba[i]=='\n'){
            //qInfo("strLen %d", recvStr.length());
            comPacketsRcvd++;
            //appendLogString(QString("strLen:") + QString::number(recvStr.length()));

            processStr(recvStr);
            recvStr.clear();
        }
    }
}

void Com::setAudioEnable(bool bEna)
{
    emit msg(QString("set ") + QString(bEna?"audioOn":"audioOff"));
    if(bEna){
        sendCmd("audioOn\n");
    }
    else{
        sendCmd("audioOff\n");
    }
}

void Com::sendCmd(const char* s)
{
    if(isOpen()){
        QString debStr(s);
        debStr.remove("\r\n");
        int iSnd = write(s);
        //qInfo("send: %s, sended %d", qPrintable(debStr), iSnd));
    }
}

void Com::sendIspGoCmd()
{
    sendCmd("G 0 T\r\n");
}

void Com::setZero()
{
    enc1Offset = enc1Val;
    enc2Offset = enc2Val;
}

void Com::handleCheckIspRunning()
{
    qDebug("handleCheckIspRunning");
    emit msg(QString("ISP timeout. Start ISP"));
    sendCmd("isp\r\n");

    //QTimer::singleShot(100, [=](){
        //sendCmd("?");
    //});


}

void Com::resetDemoModeTimer()
{
    demoCycleCount = 0;
    demoModeSteps = 1;
    demoModeState = idle;
    demoModePeriod.setSingleShot(false);
    demoModePeriod.setInterval(60*1000);
    demoModePeriod.start();
}

void Com::startDemo()
{
    demoModePeriod.setInterval(50);
    demoModeSteps = 1;
    demoModeState = idle;
}

void Com::stopDemo()
{
    resetDemoModeTimer();

}

void Com::enableDemo(bool bEna)
{
    resetDemoModeTimer();
    if(bEna ==  true){
        startDemo();
    }
    else{
        demoModePeriod.stop();
    }
}

void Com::handleDemoModePeriod()
{
    switch(demoModeState){
    case idle:
        if(distVal == -1)
            distVal = 50;
        if(enc1Val == -1)
            enc1Val = 0;
        if(enc2Val == -1)
            enc2Val = 0;
        emit msg("human interface timeout. Start demo mode");
        demoModePeriod.setInterval(50);        
        demoModeState = idleTimeout;
        demoCycleCount++;
        break;
    case idleTimeout:
        demoModeSteps--;
        if(demoModeSteps <= 0){
            demoModeSteps = 125;
            demoModeState = walkIn;
        }
        break;
    case walkIn:
        //emit msg("human interface timeout. Start demo mode. walkIn");
        distVal --;
        if(distVal <= 5){
            demoModeState = movingLeft;
        }
        break;
    case movingLeft:
        //emit msg("human interface timeout. Start demo mode. moveLeft");
        enc1Val = (enc1Val+enc1Offset-25)&0x1fff;
        enc2Val = enc2Offset; //(enc2Val-1)&0x1fff;

        demoModeSteps--;
        if(demoModeSteps <= 0){
            demoModeSteps = 125;
            demoModeState = movingRight;
        }
        break;
    case movingRight:
        //emit msg("human interface timeout. Start demo mode. moveRight");

        enc1Val = (enc1Val+enc1Offset+25)&0x1fff;
        enc2Val = enc2Offset; //(enc2Val-1)&0x1fff;

        demoModeSteps--;
        if(demoModeSteps <= 0){
            demoModeSteps = 125;
            demoModeState = walkOut;
        }
        break;
    case walkOut:
        //emit msg("human interface timeout. Start demo mode. walkOut");
        distVal ++;
        if(distVal >= 50){
            demoModeSteps = 400;
            demoModeState = idle;
        }
        break;

    }
    emit newPosData(0, 0, 0);
}

void Com::startIsp()
{
    sendCmd("isp\r\n");
    ispState = waitSynchronizedIspState;
}
