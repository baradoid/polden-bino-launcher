#include "dialog.h"
#include "ui_dialog.h"
#include <QTime>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    enc1(0),enc2(0),r(0),
    settings("bino-settings.ini", QSettings::IniFormat)
    //settings("murinets", "binoc-launcher")
{
    ui->setupUi(this);

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(8050);

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(handleReadPendingDatagrams()));

    hbTimer.setInterval(250);
    QObject::connect(&hbTimer, SIGNAL(timeout()), this, SLOT(hbTimerOut()));
    hbTimer.start();    

    debugPosTimer.setInterval(20);
    QObject::connect(&debugPosTimer, SIGNAL(timeout()), this, SLOT(debugTimerOut()));
    //debugPosTimer.start();


    on_pushButton_refreshCom_clicked();

    QString mainCom = settings.value("usbMain", "").toString();

    for(int i=0; i<ui->comComboBox->count(); i++){
       // ui->comComboBoxUsbMain->itemData()
        if(ui->comComboBox->itemData(i).toString() == mainCom){
            ui->comComboBox->setCurrentIndex(i);
            on_pushButtonComOpen_clicked();
            //if(ui->checkBoxInitOnStart->isChecked()){
            //    on_pushButtonInitiate_clicked();
            //}
            break;
        }
    }

    rangeThresh = settings.value("rangeThresh", 20).toInt();

    ui->lineEditRangeThresh->setValidator( new QIntValidator(0, 100, this) );
    ui->lineEditRangeThresh->setText(QString::number(rangeThresh));

    ui->lineEditEnc1->setText("n/a");
    ui->lineEditEnc2->setText("n/a");
    ui->lineEditRange->setText("n/a");
    ui->lineEditRange_2->setText("n/a");
}

Dialog::~Dialog()
{
    QString comName = (ui->comComboBox->currentData().toString());
    settings.setValue("usbMain", comName);
    settings.setValue("rangeThresh", rangeThresh);

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
        if(pSI->lastHbaRecvd.elapsed() > 1000){
            if(pSI->bHbaRecvd == false){
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

typedef struct{
    int16_t pos1;
    int16_t pos2;
    uint16_t rangeThresh:1;
    //int16_t distance;
//    int8_t headTemp;
//    int8_t batteryTemp;
//    int32_t cashCount;
} CbDataUdp;


void Dialog::debugTimerOut()
{
    enc1++;
    enc2 = 1278;
    if(enc1 >=8192)
        enc1 = 0;
    CbDataUdp cbdata;
    cbdata.pos1 = enc1;
    cbdata.pos2 = enc2;
    //cbdata.distance = 12;

    ui->lineEditEnc1->setText(QString::number(enc1));
    ui->lineEditEnc2->setText(QString::number(enc2));
    //ui->lineEditRange->setText(QString::number(cbdata.distance));

    for(int r=0; r<ui->listWidgetClients->count(); r++){
        TSenderInfo *sndInfo = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
        udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), sndInfo->addr, sndInfo->port);
    }


}

void Dialog::on_pushButtonComOpen_clicked()
{
    serial.setBaudRate(115200);
     if(ui->pushButtonComOpen->text() == "open"){
         if(serial.isOpen() == false){
             QString comName = ui->comComboBox->currentText();
             if(comName.length() > 0){
                 //UartThread.requestToStart(comName);
                 serial.setPortName(comName);
                 if (!serial.open(QIODevice::ReadWrite)) {
                     qDebug("%s port open FAIL", qUtf8Printable(comName));
                     return;
                 }
                 qDebug("%s port opened", qUtf8Printable(comName));
                 connect(&serial, SIGNAL(readyRead()),
                         this, SLOT(handleSerialReadyRead()));
//                 connect(&serial, SIGNAL(bytesWritten(qint64)),
//                         this, SLOT(handleSerialDataWritten(qint64)));
                 ui->pushButtonComOpen->setText("close");
                 //emit showStatusBarMessage("connected", 3000);
                 //ui->statusBar->showMessage("connected", 2000);
                 recvdComPacks = 0;
                 startRecvTime = QTime::currentTime();
             }
         }
     }
     else{
         serial.close();
         //udpSocket->close();
         qDebug("port closed");
         ui->pushButtonComOpen->setText("open");
         //contrStringQueue.clear();
         //ui->statusBar->showMessage("disconnected", 2000);
     }
}

void Dialog::on_pushButton_refreshCom_clicked()
{
    ui->comComboBox->clear();
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    const QString blankString = QObject::tr("N/A");
      QString description;
      QString manufacturer;
      QString serialNumber;
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
           description = serialPortInfo.description();
           manufacturer = serialPortInfo.manufacturer();
           serialNumber = serialPortInfo.serialNumber();
           qDebug() << endl
               << QObject::tr("Port: ") << serialPortInfo.portName() << endl
               << QObject::tr("Location: ") << serialPortInfo.systemLocation() << endl
               << QObject::tr("Description: ") << (!description.isEmpty() ? description : blankString) << endl
               << QObject::tr("Manufacturer: ") << (!manufacturer.isEmpty() ? manufacturer : blankString) << endl
               << QObject::tr("Serial number: ") << (!serialNumber.isEmpty() ? serialNumber : blankString) << endl
               << QObject::tr("Vendor Identifier: ") << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : blankString) << endl
               << QObject::tr("Product Identifier: ") << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : blankString) << endl
               << QObject::tr("Busy: ") << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) << endl;
           ui->comComboBox->addItem(serialPortInfo.portName(), serialPortInfo.portName());
    }
}


void Dialog::processStr(QString str)
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
        ui->lineEditEnc1->setText(QString::number(xPos1));
        ui->lineEditEnc2->setText(QString::number(xPos2));
        ui->lineEditRange->setText(QString::number(dist));
        //ui->lineEditTerm1->setText(QString::number(temp));
        //qDebug() << xPos1 << xPos2;

        CbDataUdp cbdata;
        cbdata.pos1 = (int16_t)xPos1;
        cbdata.pos2 = (int16_t)xPos2;
        //cbdata.distance = (int16_t)dist;
        cbdata.rangeThresh = (int)(dist>rangeThresh);

        for(int r=0; r<ui->listWidgetClients->count(); r++){
            TSenderInfo *sndInfo = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
            udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), sndInfo->addr, sndInfo->port);
        }
    }

    //appendPosToGraph(xPos);

}

void Dialog::handleSerialReadyRead()
{
    QByteArray ba = serial.readAll();

    bytesRecvd += ba.length();

    for(int i=0; i<ba.length(); i++){
        recvStr.append((char)ba[i]);
        if(ba[i]=='\n'){
            processStr(recvStr);
            recvStr.clear();
        }
    }

}

void Dialog::on_pushButtonSet_clicked()
{
    rangeThresh = ui->lineEditRangeThresh->text().toInt();
    qDebug("thresh: %d", rangeThresh);
}
