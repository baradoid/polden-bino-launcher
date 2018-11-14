#include "com.h"

Com::Com(QObject *parent) : QSerialPort(parent),
    enc1Val(-1), enc2Val(-1), distVal(-1),
    enc1Offset(0), enc2Offset(0)
{
    setBaudRate(115200);
    connect(this, SIGNAL(readyRead()),
            this, SLOT(handleSerialReadyRead()));
}

bool Com::open()
{
    return QSerialPort::open(QIODevice::ReadWrite);
}

void Com::processStr(QString str)
{
    recvdComPacks++;
    //qDebug() << str.length() <<":" <<str;
//    if(str.length() != 41){
//        str.remove("\r\n");
//        qDebug() << "string length " << str.length() << "not equal 41" << qPrintable(str);
//    }
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

        enc1Val = xPos1;
        enc2Val = xPos2;
        distVal = dist;

        //enc1Val = xPos1;
        //enc2Val = xPos2;
        //distVal = dist;

        //sendPosData();
        emit newPosData(xPos1, xPos2, dist);

    }

    //appendPosToGraph(xPos);

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
            if(recvStr.length() != 17){
                comErrorPacketsRcvd++;
            }
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
        debStr.remove('\n');
        qInfo("send: %s, sended %d", qPrintable(debStr), write(s));
    }
}

void Com::setZero()
{
    enc1Offset = enc1Val;
    enc2Offset = enc2Val;

}
