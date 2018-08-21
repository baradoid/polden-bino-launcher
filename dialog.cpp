#include "dialog.h"
#include "ui_dialog.h"
#include <QTime>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QProcess>
#include <QFileDialog>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    enc1(0),enc2(0),r(0),
    settings("bino-settings.ini", QSettings::IniFormat),
    wdNoClientsTimeSecs(0),
    bUnityStarted(false)
    //settings("murinets", "binoc-launcher")
{
    ui->setupUi(this);

    QString compileDateTime = QString(__DATE__) + " " + QString(__TIME__);
    ui->label_buildTime->setText(compileDateTime);

    QString logDirPath = settings.value("log/logPath").toString();
    if(logDirPath.isEmpty()){
        logDirPath = QDir::currentPath() + "/logs";
    }

    ui->checkBoxLogClearIfSizeExceed->setChecked(settings.value("log/logCLearIfSizeExceed", true).toBool());

    ui->lineEditLogPath->setText(logDirPath);
    appendLogFileString("\r\n");
    appendLogFileString("--- start");
    appendLogString(QString("restore log dir:\"")+(logDirPath.isEmpty()? "n/a":logDirPath)+ "\"");

    udpSocket = new QUdpSocket(this);
    if(udpSocket->bind(8050)){
        appendLogString("udp socket on port 8050 bind OK");
    }
    else{
        appendLogString("udp socket on port 8050 bind FAIL");
    }

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

    appendLogString(QString("restore com port num:\"")+(mainCom.isEmpty()? "n/a":mainCom)+ "\"");

    if(ui->comComboBox->count() == 0){
        appendLogString(QString("no com ports in system"));
    }
    else{
        for(int i=0; i<ui->comComboBox->count(); i++){
           // ui->comComboBoxUsbMain->itemData()
            if(ui->comComboBox->itemData(i).toString() == mainCom){
                appendLogString(QString("com port %1 present. Try open.").arg(mainCom));
                ui->comComboBox->setCurrentIndex(i);
                on_pushButtonComOpen_clicked();
                //if(ui->checkBoxInitOnStart->isChecked()){
                //    on_pushButtonInitiate_clicked();
                //}
                break;
            }
        }
    }

    rangeThresh = settings.value("rangeThresh", 20).toInt();

    ui->lineEditRangeThresh->setValidator( new QIntValidator(0, 100, this) );
    ui->lineEditRangeThresh->setText(QString::number(rangeThresh));

    ui->lineEditEnc1->setText("n/a");
    ui->lineEditEnc2->setText("n/a");
    ui->lineEditRange->setText("n/a");
    ui->lineEditRange_2->setText("n/a");

    ui->lineEditDebugR->setValidator(new QIntValidator(0,50, this));
    ui->lineEditDebugX->setValidator(new QIntValidator(0,8191, this));
    ui->lineEditDebugY->setValidator(new QIntValidator(0,8191, this));

    QString unityBuildPath = settings.value("watchdog/unityBuildExePath").toString();
    appendLogString(QString("restore unity path:\"")+(unityBuildPath.isEmpty()? "n/a":unityBuildPath)+ "\"");
    ui->lineEditBuildPath->setText(unityBuildPath);
    ui->lineEditBuildPath->setToolTip(unityBuildPath);

    ui->lineEditWdTimeOutSecs->setValidator(new QIntValidator(10,999, this));
    QString wdTo = settings.value("watchdog/timeout", 10).toString();
    appendLogString(QString("restore watchdog time out:\"")+(wdTo.isEmpty()? "n/a":wdTo)+ "\"");
    ui->lineEditWdTimeOutSecs->setText(wdTo);

    connect(&wdTimer, SIGNAL(timeout()), this, SLOT(handleWdTimeout()));
    wdTimer.setInterval(1000);
    wdTimer.start();

    ui->checkBoxWdEnable->setChecked(settings.value("watchdog/ena", false).toBool());

    ui->label_buildTime->setText(QString(__DATE__) + " " + QString(__TIME__));

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
    //qDebug("handleReadPendingDatagrams");
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        if(datagram.isValid() == false)
            continue;

        QString sAddrStr = datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort());

        if(datagram.data() == "reg"){            
            bool bExist = false;

            for(int r=0; r<ui->listWidgetClients->count(); r++){
                QString itText = ui->listWidgetClients->item(r)->text();
                if(itText == sAddrStr)
                    bExist = true;
            }
            appendLogString("UDP client reg: "+datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort()));
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
                    //qDebug() << pSI->lastHbaRecvd.elapsed();
                    break;
                }
            }


        }
        else{
            qDebug()<< datagram.senderAddress() << datagram.senderPort() << ":" << datagram.data();
        }



    }
    //qDebug("handleReadPendingDatagrams end");

}

void Dialog::hbTimerOut()
{
    //qDebug("check1");
    for(int r=0; r<ui->listWidgetClients->count(); r++){
        QListWidgetItem *lwi = ui->listWidgetClients->item(r);
        QString itText = lwi->text();
        TSenderInfo *pSI = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
        //qDebug() << pSI->lastHbaRecvd.elapsed();
        //qDebug("check1 %d", pSI->lastHbaRecvd.elapsed() );
        if(pSI->lastHbaRecvd.elapsed() > 500){
            if(pSI->bHbaRecvd == false){
                appendLogString(QString("UDP client %1:%2 no answer in timeout 500 ms. delete. ").arg(pSI->addr.toString()).arg(QString::number(pSI->port)));
                //qDebug() << "delete client";
                ui->listWidgetClients->takeItem(r);
                delete pSI;
                break;
                //ui->listWidgetClients->removeItemWidget(lwi);

            }
        }

    }

    //qDebug("check2");
    for(int r=0; r<ui->listWidgetClients->count(); r++){
        TSenderInfo *sndInfo = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
        //if(itText == sAddr)
        //    bExist = true;
        //if(udpSocket->isWritable()){
        //}
        if(sndInfo->bHbaRecvd == true){
            sndInfo->bHbaRecvd = false;
            udpSocket->writeDatagram("hb", sndInfo->addr, sndInfo->port);
            sndInfo->lastHbaRecvd.restart();
        }

    }
    //qDebug("check2 end");

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
                     appendLogString(QString("%1 port open FAIL").arg(comName));
                     return;
                 }                 
                 qDebug("%s port opened", qUtf8Printable(comName));
                 appendLogString(QString("%1 port opened").arg(comName));
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

    QString logStr("com ports present: ");
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
           logStr += (serialPortInfo.portName() + " " + serialPortInfo.systemLocation() + " " + description);
    }

    if(ui->comComboBox->count() == 0 ){
        logStr += "n/a";
    }
    appendLogString(logStr);

    //appendLogString("----");

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
        cbdata.rangeThresh = (int)(dist<rangeThresh);
        ui->checkBoxThreshExcess->setChecked(dist<rangeThresh);

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

void Dialog::on_pushButtonDebugSend_clicked()
{
    int x,y,r;
    x = ui->lineEditDebugX->text().toInt();
    y = ui->lineEditDebugY->text().toInt();
    r = ui->lineEditDebugR->text().toInt();
    CbDataUdp cbdata;
    cbdata.pos1 = (int16_t)x;
    cbdata.pos2 = (int16_t)y;
    cbdata.rangeThresh = (int16_t)r;
    for(int r=0; r<ui->listWidgetClients->count(); r++){
        TSenderInfo *sndInfo = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
        udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), sndInfo->addr, sndInfo->port);
    }

}

void Dialog::on_pushButtonWDTest_clicked()
{
    appendLogString("watchdog: test button");
    restartUnityBuild();
}

void Dialog::on_pushButtonWdSelectPath_clicked()
{
    QString str = QFileDialog::getOpenFileName(this,
       tr("Select unity build exe file"), "/home/jana", tr("Unity Build exe file (*.exe)"));
    if(str.isEmpty() == false){
        settings.setValue("watchdog/unityBuildExePath", str);
        ui->lineEditBuildPath->setText(str);
        ui->lineEditBuildPath->setToolTip(str);
    }
}

void Dialog::on_lineEditWdTimeOutSecs_editingFinished()
{
    int wto = ui->lineEditWdTimeOutSecs->text().toInt();
    settings.setValue("watchdog/timeout", wto);
}


void Dialog::handleWdTimeout()
{
    if(ui->listWidgetClients->count() == 0){
        ui->label_wd_noclients->show();
        ui->label_wd_noclients_2->show();
        ui->lineEdit_wdNoClientsTimer->show();
        wdNoClientsTimeSecs++;
        ui->lineEdit_wdNoClientsTimer->setText(QString::number(wdNoClientsTimeSecs));
        int wto = ui->lineEditWdTimeOutSecs->text().toInt();
        if(ui->checkBoxWdEnable->isChecked() && (bUnityStarted==false) && (wdNoClientsTimeSecs > wto)){
          on_pushButtonWDTest_clicked();
          appendLogString("watchdog: no clients in timeout. Try to restart unity build");
          restartUnityBuild();
          bUnityStarted = true;
        }

    }
    else{
        bUnityStarted = false;
        ui->label_wd_noclients->hide();
        ui->label_wd_noclients_2->hide();
        ui->lineEdit_wdNoClientsTimer->hide();
        wdNoClientsTimeSecs=0;
    }
    //qDebug("wd");

}

void Dialog::on_checkBoxWdEnable_clicked(bool checked)
{
    settings.setValue("watchdog/ena", checked);
}

void Dialog::on_lineEditLogPath_editingFinished()
{
    QString pathStr = ui->lineEditLogPath->text();
    ui->lineEditLogPath->setToolTip(pathStr);
    settings.setValue("log/logPath", pathStr);
}

void Dialog::on_pushButtonLogSelectPath_clicked()
{
    QString str = QFileDialog::getExistingDirectory(this, tr("Select dir for logs"));
    if(str.isEmpty() == false){
        //settings.setValue("watchdog/unityBuildExePath", str);
        ui->lineEditLogPath->setText(str);
        //ui->lineEditLogPath->setToolTip(str);
        on_lineEditLogPath_editingFinished();
    }
}

void Dialog::appendLogString(QString str)
{
    QString curTime = QTime::currentTime().toString();
    QString logString = curTime + ">" + str;

    ui->textEditLog->append(logString);
    appendLogFileString(logString);
}

void Dialog::appendLogFileString(QString logString)
{
    QString pathStr = ui->lineEditLogPath->text();
    if(QDir().mkpath(pathStr) == true){

        qint64 size = 0;
        QDir dir(pathStr);
        //calculate total size of current directories' files
        for(QString filePath : dir.entryList(QStringList() << "*.txt", QDir::Files)) {
            QFileInfo fi(dir, filePath);
            size+= fi.size();

        }
        //qDebug() << "dir path" << size ;
        //qDebug() << logFiles;
        ui->lineEditLogSize->setText(QString::number((float)size/1024., 'f', 2));

        if((ui->checkBoxLogClearIfSizeExceed->isChecked()) &&(size > (1024*1024))){
            QStringList logFiles = dir.entryList(QStringList() << "*.txt" << "*.JPG",QDir::Files);
            if((logFiles.count()>0))
                qDebug() << "remove file" << dir.absoluteFilePath(logFiles[0]) << QFile(dir.absoluteFilePath(logFiles[0])).remove();
        }



        QString logFileName = QDate::currentDate().toString("yyyy-MM-dd")+".txt";
        pathStr + logFileName;
        //qDebug() << "creates path ok " << qPrintable(pathStr + "/" + logFileName);
        QFile f(pathStr + "/" + logFileName);
        if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
            f.write(qPrintable(logString + "\r\n"));
            f.close();
        }
    }
}

void Dialog::restartUnityBuild()
{
    QProcess p;

    p.start("taskkill /IM VR.exe");
    if(p.waitForFinished()){
        appendLogString("kill \"VR.exe\" ... OK");
    }
    else{
        appendLogString("kill \"VR.exe\" ... FAIL");
    }
    QString str = ui->lineEditBuildPath->text();
    if(str.isEmpty() == true){
        appendLogString("Path empty. Nothing to start.");
        return;
    }
    if(p.startDetached(str)){
        appendLogString(QString("start \"") + str + "\" ... OK");
    }
    else{
        appendLogString(QString("start \"") + str + "\" ... FAIL");
    }
}


void Dialog::on_checkBoxLogClearIfSizeExceed_clicked()
{
    settings.setValue("log/logCLearIfSizeExceed", ui->checkBoxLogClearIfSizeExceed->isChecked());
}
