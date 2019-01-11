#include "com.h"
#include <QThread>
#include <QDebug>
//#include <QtMath>


Com::Com(QObject *parent) : QSerialPort(parent),
    enc1Val(-1), enc2Val(-1), distVal(-1),
    enc1Offset(0), enc2Offset(0),
    comPacketsRcvd(0), comErrorPacketsRcvd(0),
    checkIspTimer(this),ispState(idleIspState),
    pingTimer(this)
{
    setBaudRate(115200);
    connect(this, SIGNAL(readyRead()),
            this, SLOT(handleSerialReadyRead()));

    connect(&checkIspTimer, SIGNAL(timeout()),
                    this, SLOT(handleCheckIspRunning()));


    //readyReadStandardError()
    //void	readyReadStandardOutput()

    pingTimer.setInterval(2*1000);
    pingTimer.setSingleShot(false);
    connect(&pingTimer, &QTimer::timeout, [=](){
        sendCmd("up?\r\n");
    });



    connect(&fwProcess, &QProcess::readyReadStandardOutput, [=](){
        emit msg(QString(fwProcess.readAllStandardOutput()));
    });

    connect(&fwProcess, &QProcess::readyReadStandardError, [=](){
        emit msg(QString(fwProcess.readAllStandardError()));
    });
//    connect(&fwProcess,  static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
//            [=](int exitCode, QProcess::ExitStatus exitStatus){
//        emit msg("finished");
//    });

    connect(&fwProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(handleProcessFinished(int,QProcess::ExitStatus)));
}

bool Com::open()
{
    bool ret = QSerialPort::open(QIODevice::ReadWrite);
    if(ret == true){
        //sendCmd("?");
        sendCmd("ver\r\n");
        //checkIspTimer.setSingleShot(true);
        //checkIspTimer.setInterval(500);
        //checkIspTimer.start();
        pingTimer.start();
    }
    return ret;
}

void Com::processStr(QString str)
{
    recvdComPacks++;

    qDebug()<<"processStr: " << qPrintable(str);
    if(ispState == idleIspState){
        if(str.startsWith("compile time:") == true){
            str.remove("compile time:");
            str.remove("\r\n");
            firmwareVer = str;
        }
        else if(str.endsWith("Synchronized\r\n")){
            //qDebug("\"Synchronized\" recvd");
            emit msg("ISP running");
            //checkIspTimer.stop();
            sendCmd("Synchronized\r\n");
            //sendIspGoCmd();
            ispState = waitSynchronizedOkIspState;
        }
        else if(str.startsWith("sysclk") == true){

        }

        else if(str.startsWith("ADC") == true){

        }
        else if(str.startsWith("up:") == true){
            str.remove("up:");
            str.remove("\r\n");
            bool bOk = false;
            int upTime = str.toInt(&bOk);
            if(bOk == true){
                uint64_t s = upTime;
                int ss = s%60;
                int m = (s/60)%60;
                int h = (s/3600)%24;
                int d = (s/(3600*24));

                QString tStr;
                tStr.sprintf("%02d:%02d:%02d:%02d", d,h,m,ss);
                //ui->lineEditUptime->setText(tStr);
                emit updateUptime(tStr);
            }
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

                //if(demoModeState == idle){
                    enc1Val = xPos1;
                    enc2Val = xPos2;
                    distVal = dist;
                //}

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
        qDebug()<<"ispState state:" << ispState;
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
            if(str.endsWith("OK\r\n")){
                //qDebug("\"Synchronized OK\" recvd");
                sendCmd("4000\r\n");
                ispState = eraseMemState;
            }
            break;
        case eraseMemState:
            if(str.endsWith("OK\r\n")){
                QStringList args;
                args << "com(31, 115200)";
                args << "device(lpc1758,0,0)";
                args << "erase(0, protectisp)";
                close();
                fwProcess.start("fm", args);
                ispState = flashMemState;
            }
            break;
        case waitFmProcessEndState:
            qDebug() << "waitFmProcessEndState " << qPrintable(str);
            sendCmd("U 23130\r\n");
            ispState = waitUnlockOkIspState;
            break;

        case waitUnlockOkIspState:
            qDebug() << "waitUnlockOkIspState " << qPrintable(str);
            if(str == "0\r\n"){                
                sendCmd("G 0 T\r\n");
                ispState = waitGoOkIspState;
            }
            break;
        case waitGoOkIspState:
            qDebug() << "waitGoOkIspState " << qPrintable(str);
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

void Com::handleProcessFinished(int code, QProcess::ExitStatus stat)
{
    qDebug()<<"handleProcessFinished state:" << ispState <<" code:"<< code <<" stat:"<<stat;

    QStringList args;
    QString fwP = fwPath.replace('/', '\\');
    switch(ispState){
    case flashMemState:
        args << "com(31, 115200)";
        args << "device(lpc1758,0,0)";
        args << QString("hexfile(%1, nochecksums, nofill, protectisp)").arg(fwPath);
        fwProcess.start("fm", args);
        ispState = verifyMemState;
        break;

    case verifyMemState:
        open();
        sendCmd("U 23130\r\n");
        ispState = waitUnlockOkIspState;

//        QStringList args;
//        args << "com(31, 115200)";
//        args << "device(lpc1758,0,0)";
//        QString fwP = fwPath.replace('/', '\\');
//        args << QString("hexfile(%1, nochecksums, nofill, protectisp)").arg(fwPath);
//        close();
//        fwProcess.start("fm", args);
        break;

    default:
        break;
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

void Com::hwRestart()
{
    sendCmd("reset\n");
    sendCmd("restart\n");
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

void Com::startIsp(QString fwp)
{
    fwPath = fwp;
    sendCmd("isp\r\n");
    ispState = waitSynchronizedIspState;
}
