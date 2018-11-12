#include "com.h"

Com::Com(QObject *parent) : QObject(parent)
{    
    serial.setBaudRate(115200);
    connect(&serial, SIGNAL(readyRead()),
            this, SLOT(handleSerialReadyRead()));
}
void Com::setPort(QString name)
{
    serial.setPortName(name);
}

bool Com::open()
{
    return serial.open(QIODevice::ReadWrite);
}

void Com::close()
{
    serial.close();
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
    QByteArray ba = serial.readAll();

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
    emit msg(QString("COM: set ") + QString(bEna?"audioOn":"audioOff"));
    if(bEna){
        sendCmd("audioOn\n");
    }
    else{
        sendCmd("audioOff\n");
    }
}

void Com::on_audioOn_clicked()
{
    setAudioEnable(true);
}

void Com::on_audioOff_clicked()
{
    setAudioEnable(false);
}

void Com::sendCmd(const char* s)
{
    if(serial.isOpen()){
        QString debStr(s);
        debStr.remove('\n');
        qInfo("send: %s, sended %d", qPrintable(debStr), serial.write(s));
    }
}
