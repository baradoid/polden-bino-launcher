#include "dialog.h"
#include "ui_dialog.h"
#include <QTime>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QProcess>
#include <QFileDialog>
#include <QHostInfo>
#include <QDesktopWidget>
#include <QScreen>
#include <QDesktopServices>
#include <windows.h>
#include <QRadioButton>

#include <QUrlQuery>
#include <QPair>

#include <QHttpMultiPart>
#include <QHttpPart>
//#include <foldercompressor.h>
//#define QUAZIP_STATIC

//#include "quazip/quazip.h"
//#include "quazip/quazipfile.h"


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    enc1(0),enc2(0),r(0),
    settings("Polden Ltd.", "bino-launcher"),
    wdNoClientsTimeSecs(0),
    bUnityStarted(false),
    enc1Val(0), enc2Val(0),
    enc1Offset(0), enc2Offset(0),
    distanceOverThreshCnt(0),
    lastDistTreshState(false),
    comPacketsRcvd(0), comErrorPacketsRcvd(0),
    webState(idle)
    //settings("murinets", "binoc-launcher")
{
    ui->setupUi(this);    

    startTime.start();
    updateUptime();
    QString compileDateTime = QString(__DATE__) + " " + QString(__TIME__);
    ui->label_buildTime->setText(compileDateTime);

    QString hostName = QHostInfo::localHostName();
    QString logDirPath = settings.value("log/logPath").toString();
    if(logDirPath.isEmpty()){
        logDirPath = QDir::currentPath() + "/logs";
    }

    if(logDirPath.endsWith(hostName) == false){
        logDirPath+= "_" + hostName;
    }

    ui->checkBoxLogClearIfSizeExceed->setChecked(settings.value("log/logCLearIfSizeExceed", true).toBool());

    ui->lineEditLogPath->setText(logDirPath);
    appendLogFileString("\r\n");
    appendLogFileString("--- start");
    appendLogString(QString("restore log dir:\"")+(logDirPath.isEmpty()? "n/a":logDirPath)+ "\"");

    ui->lineEditHostName->setText(hostName);
    appendLogString(QString("hostname: \"") + hostName + "\"");

    QList<QScreen*> screens = QGuiApplication::screens();
    QSize scrSize = QSize(0,0);
    if(screens.isEmpty() == false){
        scrSize = screens[0]->size();
        screens[0]->orientation();
        ui->lineEditScreenResolutionW->setText(QString::number(scrSize.width()));
        ui->lineEditScreenResolutionH->setText(QString::number(scrSize.height()));
        Qt::ScreenOrientation scrOr = screens[0]->orientation();
        QString orStr;
        switch(scrOr){
            case Qt::PrimaryOrientation:
                orStr = "PrimaryOrientation";
                break;
            case Qt::PortraitOrientation:
                orStr = "PortraitOrientation";
                break;
            case Qt::LandscapeOrientation:
                orStr = "LandscapeOrientation";
                break;
            case Qt::InvertedPortraitOrientation:
                orStr = "InvertedPortraitOrientation";
                break;
            case Qt::InvertedLandscapeOrientation:
                orStr = "InvertedLandscapeOrientation";
                break;
        }
        ui->lineEditOrientation->setText(orStr);
        appendLogString(QString("resolution:%1x%2 ").arg(scrSize.width()).arg(scrSize.height()) + orStr);
    }
    else{
        appendLogString("No available screens");
    }


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

    bool bAudioEnableOnStartup = settings.value("auidoEnableOnStartup", true).toBool();
    appendLogString(QString("restore audio enable on startUp: ")+(bAudioEnableOnStartup? "true":"false"));
    ui->checkBoxAudioEnableOnStartup->setChecked(bAudioEnableOnStartup);

    on_pushButton_refreshCom_clicked();

    QString mainCom = settings.value("usbMain", "").toString();

    appendLogString(QString("COM: restore com port num:\"")+(mainCom.isEmpty()? "n/a":mainCom)+ "\"");

    if(ui->comComboBox->count() == 0){
        appendLogString(QString("COM: no com ports in system"));
    }
    else{
        for(int i=0; i<ui->comComboBox->count(); i++){
           // ui->comComboBoxUsbMain->itemData()
            if(ui->comComboBox->itemData(i).toString() == mainCom){
                appendLogString(QString("COM: port %1 present. Try open.").arg(mainCom));
                ui->comComboBox->setCurrentIndex(i);
                on_pushButtonComOpen_clicked();
                if(ui->checkBoxAudioEnableOnStartup->isChecked() == true)
                    setAudioEnable(true);
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
    appendLogString(QString("WD:restore watchdog time out:\"")+(wdTo.isEmpty()? "n/a":wdTo)+ "\"");
    ui->lineEditWdTimeOutSecs->setText(wdTo);

    ui->lineEditFpsLimit->setValidator(new QIntValidator(10,999, this));
    QString fpsLimit = settings.value("watchdog/timeout", 30).toString();
    appendLogString(QString("WD:restore fps limit:\"")+(fpsLimit.isEmpty()? "n/a":fpsLimit)+ "\"");
    ui->lineEditFpsLimit->setText(fpsLimit);

    connect(&wdTimer, SIGNAL(timeout()), this, SLOT(handleWdTimeout()));
    wdTimer.setInterval(1000);
    wdTimer.start();

    ui->checkBoxWdEnable->setChecked(settings.value("watchdog/ena", false).toBool());

    ui->label_buildTime->setText(QString(__DATE__) + " " + QString(__TIME__));

    QStringList columnNames;
    columnNames<< "1" << "2"<< "3";

    //ui->tableWidgetClients->setColumnCount(3);
    //ui->tableWidgetClients->setRowCount(2);
    //ui->tableWidgetClients->setVerticalHeaderLabels(columnNames);
    //ui->tableWidgetClients->setItem(0, 0, new QTableWidgetItem("0"));
    //ui->tableWidgetClients->setItem(0, 1, new QTableWidgetItem("1"));
    //ui->tableWidgetClients->setItem(0, 2, new QTableWidgetItem("2"));
    //ui->tableWidgetClients->resizeColumnsToContents();
    ui->tableWidgetClients->setColumnCount(4);
    ui->tableWidgetClients->resizeColumnsToContents();

    connect(&logUpdateTimer, SIGNAL(timeout()), this, SLOT(handleLogUpdateTimeout()));
    logUpdateTimer.setInterval(30*1000);
    logUpdateTimer.start();


    //ui->checkBoxRangeAlwaysOn->setChecked(settings.value("rangeAlwaysOn", false).toBool());
    ui->checkBoxRangeAlwaysOn->setChecked(false);

    ui->lineEditRangeTreshExceedCount->setText("0");

    ui->checkBoxThreshExcess->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->checkBoxThreshExcess->setFocusPolicy(Qt::NoFocus);

    initEncTableWidget();

    QTimer *uiUpdateTimer = new QTimer(this);
    connect(uiUpdateTimer, SIGNAL(timeout()), this, SLOT(handleUiUpdate()));
    uiUpdateTimer->setInterval(50);
    uiUpdateTimer->start();


    initAppAutoStartCheckBox();

    writeCbParamsTimer = new QTimer(this);
    connect(writeCbParamsTimer, SIGNAL(timeout()), this, SLOT(handleWriteCBParamsTimeout()));
    writeCbParamsTimer->setInterval(100);
    writeCbParamsTimer->start();
    cbWriteParamsCount = 3;

    QString wbPath = settings.value("webManger/path", "http://localhost").toString();
    QString wbUser = settings.value("webManger/user", "bino").toString();
    QString wbPass = settings.value("webManger/pass", "bino12345").toString();

    ui->lineEdit_wbPath->setText(wbPath);
    ui->lineEdit_wbUser->setText(wbUser);
    ui->lineEdit_wbPass->setText(wbPass);

    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(handleNamReplyFinished(QNetworkReply*)));

    QPalette palette;
    palette.setColor(QPalette::Base,Qt::gray);
    ui->lineEdit_manserver_stat->setPalette(palette);
    ui->lineEdit_manserver_stat->setText("n/a");

    QTimer::singleShot(100, this, SLOT(handleWbLoginTimeout()));

    //root dir
    QString rootPath = settings.value("rootPath").toString();
    if(rootPath.isEmpty()){
        rootPath = QDir::currentPath();
    }
    if(QDir(rootPath).exists() == false){
        QDir().mkdir(rootPath);
    }

    ui->lineEditRootPath->setText(rootPath);
    connect(ui->pushButtonSelectRootPath, &QPushButton::clicked,
        [=](){
        QString str = QFileDialog::getExistingDirectory(this, tr("Select root dir"),
                                                        rootPath,
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
        if(str.isEmpty() == false){
            settings.setValue("rootPath", str);
            ui->lineEditRootPath->setText(str);
            ui->lineEditRootPath->setToolTip(str);
        }
    });

}



Dialog::~Dialog()
{
    QString comName = (ui->comComboBox->currentData().toString());
    settings.setValue("usbMain", comName);
    settings.setValue("rangeThresh", rangeThresh);

    appendLogString("--- quit");
    delete ui;
}

typedef struct{
    QHostAddress addr;
    int port;

    QTime lastHbaRecvd;
    bool bHbaRecvd;
} TSenderInfo;

void Dialog::initEncTableWidget()
{
    ui->tableWidgetEncoders->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidgetEncoders->setHorizontalHeaderLabels(QStringList() << "val" << "offset" << "horiz" << "inv");
    ui->tableWidgetEncoders->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    //
    //ui->tableWidgetEncoders->setCellWidget(0,3, new QRadioButton());
    //ui->tableWidgetEncoders->item(0, 3)->setTextAlignment(Qt::AlignHCenter);

    horEncSelectBG = new QButtonGroup();
    horEncSelectBG->addButton(new QRadioButton(), 0);
    horEncSelectBG->addButton(new QRadioButton(), 1);

    connect(horEncSelectBG, QOverload<int>::of(&QButtonGroup::buttonClicked),
        [=](int id){
        settings.setValue("encoders/hozEncId", id);
    });


    QWidget * w = new QWidgetCust();
    QHBoxLayout *l = new QHBoxLayout();
    l->setAlignment( Qt::AlignCenter );
    l->addWidget( horEncSelectBG->button(0) );
    l->setContentsMargins(0,0,0,0);
    w->setLayout( l );
    ui->tableWidgetEncoders->setCellWidget(0,2, w);

    connect(((QWidgetCust*)w), &QWidgetCust::mousePressedSignal, [=](){
        ((QRadioButton*)horEncSelectBG->button(0))->setChecked(true);
        settings.setValue("encoders/hozEncId", 0);
    });


    w = new QWidgetCust();
    l = new QHBoxLayout();
    l->setAlignment( Qt::AlignCenter );
    l->addWidget( horEncSelectBG->button(1) );
    l->setContentsMargins(0,0,0,0);
    w->setLayout( l );
    //connect(w, SIGNAL())
    ui->tableWidgetEncoders->setCellWidget(1, 2, w);

    connect(((QWidgetCust*)w), &QWidgetCust::mousePressedSignal, [=](){
        ((QRadioButton*)horEncSelectBG->button(1))->setChecked(true);
        settings.setValue("encoders/hozEncId", 1);
    });

    int defEncId = settings.value("encoders/hozEncId", 0).toInt();
    ((QRadioButton*)horEncSelectBG->button(defEncId))->setChecked(true);


    leEnc1 = new QTableWidgetItem;
    leEnc2 = new QTableWidgetItem;
    leEnc1Off = new QTableWidgetItem;
    leEnc2Off = new QTableWidgetItem;

    QList<QTableWidgetItem*> wList;
    wList.append(leEnc1);
    wList.append(leEnc2);
    wList.append(leEnc1Off);
    wList.append(leEnc2Off);

    foreach (QTableWidgetItem* twl, wList) {
        twl->setTextAlignment(Qt::AlignCenter);
        twl->setText("n/a");
    }


    //ui->lineEditEnc1->setText("n/a");
    //ui->lineEditEnc2->setText("n/a");

    enc1Offset = settings.value("encoders/enc1Offset", 0).toInt();
    //ui->lineEditEnc1Offset->setText(QString::number(enc1Offset));
    leEnc1Off->setText(QString::number(enc1Offset));

    enc2Offset = settings.value("encoders/enc2Offset", 0).toInt();
    //ui->lineEditEnc2Offset->setText(QString::number(enc2Offset));
    leEnc2Off->setText(QString::number(enc2Offset));

    ui->tableWidgetEncoders->setItem(0, 0, leEnc1);
    ui->tableWidgetEncoders->setItem(1, 0, leEnc2);
    ui->tableWidgetEncoders->setItem(0, 1, leEnc1Off);
    ui->tableWidgetEncoders->setItem(1, 1, leEnc2Off);


    QWidget *checkBoxWidget = new QWidget();
    checkEnc1Inv = new QCheckBox();      // We declare and initialize the checkbox
    QHBoxLayout *layoutCheckBox = new QHBoxLayout(checkBoxWidget); // create a layer with reference to the widget
    layoutCheckBox->addWidget(checkEnc1Inv);            // Set the checkbox in the layer
    layoutCheckBox->setAlignment(Qt::AlignCenter);  // Center the checkbox
    layoutCheckBox->setContentsMargins(0,0,0,0);    // Set the zero padding

    checkEnc1Inv->setChecked(settings.value("encoders/enc1Inverse", false).toBool());

    connect(checkEnc1Inv, &QCheckBox::clicked, [=](bool bChecked){ settings.setValue("encoders/enc1Inverse", bChecked); });
    ui->tableWidgetEncoders->setCellWidget(0,3, checkBoxWidget);


    checkBoxWidget = new QWidget();
    checkEnc2Inv = new QCheckBox();      // We declare and initialize the checkbox
    layoutCheckBox = new QHBoxLayout(checkBoxWidget); // create a layer with reference to the widget
    layoutCheckBox->addWidget(checkEnc2Inv);            // Set the checkbox in the layer
    layoutCheckBox->setAlignment(Qt::AlignCenter);  // Center the checkbox
    layoutCheckBox->setContentsMargins(0,0,0,0);    // Set the zero padding

    checkEnc2Inv->setChecked(settings.value("encoders/enc2Inverse", false).toBool());

    connect(checkEnc2Inv, &QCheckBox::clicked, [=](bool bChecked){ settings.setValue("encoders/enc2Inverse", bChecked); });
    ui->tableWidgetEncoders->setCellWidget(1,3, checkBoxWidget);



//    QTableWidgetItem *checkBoxItem = new QTableWidgetItem();
//    checkBoxItem->setTextAlignment(Qt::AlignHCenter);
//    checkBoxItem->setCheckState(Qt::Unchecked);

    //ui->tableWidgetEncoders->item(0,3)->setCheckState(Qt::Unchecked);
    //ui->tableWidgetEncoders->setItem(0,3, NULL);
    //ui->tableWidgetEncoders->setItem(0,3, checkBoxItem);
    //ui->tableWidgetEncoders->setItem(1,3, checkBoxItem);

}

void Dialog::handleReadPendingDatagrams()
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

            for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
                QString itText = ui->tableWidgetClients->item(r, 0)->text();
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

//                ui->listWidgetClients->addItem(sAddrStr);
//                ui->listWidgetClients->item(ui->listWidgetClients->count()-1)->setData(Qt::UserRole, (int)pSI);

                int curRowInd = ui->tableWidgetClients->rowCount();
                ui->tableWidgetClients->setRowCount(curRowInd+1);
                QTableWidgetItem *twi = new QTableWidgetItem(sAddrStr);
                twi->setData(Qt::UserRole, (int)pSI);
                twi->setTextAlignment(Qt::AlignCenter);
                ui->tableWidgetClients->setItem(curRowInd, 0, twi);
                twi = new QTableWidgetItem("n/a");
                twi->setTextAlignment(Qt::AlignCenter);
                ui->tableWidgetClients->setItem(curRowInd, 1, twi);
                twi = new QTableWidgetItem("n/a");
                twi->setTextAlignment(Qt::AlignCenter);
                ui->tableWidgetClients->setItem(curRowInd, 2, twi);
                twi = new QTableWidgetItem("n/a");
                twi->setTextAlignment(Qt::AlignCenter);
                ui->tableWidgetClients->setItem(curRowInd, 3, twi);
                ui->tableWidgetClients->resizeColumnsToContents();
            }
        }
        else if(dData.startsWith("hba") == true){
            for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
                QString itText = ui->tableWidgetClients->item(r, 0)->text();
                TSenderInfo *pSI = (TSenderInfo*)ui->tableWidgetClients->item(r, 0)->data(Qt::UserRole).toInt();
                if(itText == sAddrStr){
                    //pSI->lastHbaRecvd.restart();
                    pSI->bHbaRecvd = true;
                    //qDebug() << pSI->lastHbaRecvd.elapsed();
                    ui->tableWidgetClients->item(r, 1)->setText(QString::number(pSI->lastHbaRecvd.elapsed()));
                    if(dData.length() > 3){
                        //qDebug() << qPrintable(dData);
                        QStringList strL =  dData.split(" ");
                        strL[1].replace(',','.');
                        float fps = strL[1].toFloat();
                        ui->tableWidgetClients->item(r, 2)->setText(QString::number(fps,'f', 1));
                        ui->tableWidgetClients->item(r, 3)->setText(strL[2]);
                    }
                    ui->tableWidgetClients->resizeColumnsToContents();
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
    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
        //QListWidgetItem *lwi = ui->listWidgetClients->item(r);
        //QString itText = lwi->text();
        TSenderInfo *pSI = (TSenderInfo*)ui->tableWidgetClients->item(r, 0)->data(Qt::UserRole).toInt();
        //qDebug() << pSI->lastHbaRecvd.elapsed();
        //qDebug("check1 %d", pSI->lastHbaRecvd.elapsed() );
        if(pSI->lastHbaRecvd.elapsed() > 500){
            if(pSI->bHbaRecvd == false){
                appendLogString(QString("UDP client %1:%2 no answer in timeout 500 ms. delete. ").arg(pSI->addr.toString()).arg(QString::number(pSI->port)));
                //qDebug() << "delete client";
                //ui->listWidgetClients->takeItem(r);

                ui->tableWidgetClients->removeRow(r);
                delete pSI;
                break;
                //ui->listWidgetClients->removeItemWidget(lwi);

            }
        }

    }

    //qDebug("check2");
    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
        TSenderInfo *sndInfo = (TSenderInfo*)ui->tableWidgetClients->item(r,0)->data(Qt::UserRole).toInt();
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

#pragma pack(push,1)
typedef struct{
    uint16_t pos1;
    uint16_t pos2;
    uint8_t rangeThresh;
} CbDataUdp;
#pragma pack(pop)

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

    //ui->lineEditEnc1->setText(QString::number(enc1));
    //ui->lineEditEnc2->setText(QString::number(enc2));
    //ui->lineEditRange->setText(QString::number(cbdata.distance));

    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
        TSenderInfo *sndInfo = (TSenderInfo*)ui->tableWidgetClients->item(r,0)->data(Qt::UserRole).toInt();
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
                     appendLogString(QString("COM: %1 port open FAIL").arg(comName));
                     return;
                 }                 
//                 qDebug("%s port opened", qUtf8Printable(comName));
                 appendLogString(QString("COM: port %1 opened").arg(serial.portName()));
                 connect(&serial, SIGNAL(readyRead()),
                         this, SLOT(handleSerialReadyRead()));
//                 connect(&serial, SIGNAL(bytesWritten(qint64)),
//                         this, SLOT(handleSerialDataWritten(qint64)));
                 ui->pushButtonComOpen->setText("close");
                 //emit showStatusBarMessage("connected", 3000);
                 //ui->statusBar->showMessage("connected", 2000);
                 recvdComPacks = 0;
                 startRecvTime = QTime::currentTime();
                 cbWriteParamsCount = 3;
             }
         }
     }
     else{
         serial.close();
         //udpSocket->close();
//         qDebug("port closed");
         ui->pushButtonComOpen->setText("open");
         //contrStringQueue.clear();
         //ui->statusBar->showMessage("disconnected", 2000);
         appendLogString(QString("COM: port %1 closed").arg(serial.portName()));
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

    QString logStr("COM: com ports present: ");
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
           description = serialPortInfo.description();
           manufacturer = serialPortInfo.manufacturer();
           serialNumber = serialPortInfo.serialNumber();
//           qDebug() << endl
//               << QObject::tr("Port: ") << serialPortInfo.portName() << endl
//               << QObject::tr("Location: ") << serialPortInfo.systemLocation() << endl
//               << QObject::tr("Description: ") << (!description.isEmpty() ? description : blankString) << endl
//               << QObject::tr("Manufacturer: ") << (!manufacturer.isEmpty() ? manufacturer : blankString) << endl
//               << QObject::tr("Serial number: ") << (!serialNumber.isEmpty() ? serialNumber : blankString) << endl
//               << QObject::tr("Vendor Identifier: ") << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : blankString) << endl
//               << QObject::tr("Product Identifier: ") << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : blankString) << endl
//               << QObject::tr("Busy: ") << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) << endl;
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
        //ui->lineEditEnc1->setText(QString::number(xPos1));
        //ui->lineEditEnc2->setText(QString::number(xPos2));
        //ui->lineEditTerm1->setText(QString::number(temp));
        //qDebug() << xPos1 << xPos2;

        rangeThresh = ui->lineEditRangeThresh->text().toInt();

        //qInfo("%d %d %d", distVal, rangeThresh, dist<rangeThresh);
        enc1Val = xPos1;
        enc2Val = xPos2;
        distVal = dist;

        sendPosData();

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
//    for(int r=0; r<ui->listWidgetClients->count(); r++){
//        TSenderInfo *sndInfo = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
//        udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), sndInfo->addr, sndInfo->port);
//    }

}

void Dialog::on_pushButtonWDTest_clicked()
{
    appendLogString("watchdog: test button");
    restartUnityBuild();
}

void Dialog::on_pushButtonWdSelectPath_clicked()
{
    QString str = QFileDialog::getOpenFileName(this,
       tr("Select unity build exe file"), "/home/jana", tr("Unity Build exe file (*.exe *.bat)"));
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
    if(ui->tableWidgetClients->rowCount() == 0){
        ui->label_wd_noclients->show();
        ui->label_wd_noclients_2->show();
        ui->lineEdit_wdNoClientsTimer->show();
        wdNoClientsTimeSecs++;
        ui->lineEdit_wdNoClientsTimer->setText(QString::number(wdNoClientsTimeSecs));
        int wto = ui->lineEditWdTimeOutSecs->text().toInt();
        if(ui->checkBoxWdEnable->isChecked() && (bUnityStarted==false) && (wdNoClientsTimeSecs > wto)){          
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


    updateUptime();
}

void Dialog::on_checkBoxWdEnable_clicked(bool checked)
{
    settings.setValue("watchdog/ena", checked);
    appendLogString(QString("push button watchdog %1").arg(checked? "enable" : "disable"));

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

QString Dialog::todayLogName()
{
    return QDate::currentDate().toString("yyyy-MM-dd")+".txt";
}

QString Dialog::todayLogPath()
{
    QString pathStr = ui->lineEditLogPath->text();
    pathStr += "/" + todayLogName();
    return pathStr;
}

void Dialog::appendLogFileString(QString logString)
{
    QString pathStr = ui->lineEditLogPath->text();
    if(QDir().mkpath(pathStr) == true){

        QString logFileName = todayLogName();
        pathStr + logFileName;
        //qDebug() << "creates path ok " << qPrintable(pathStr + "/" + logFileName);
        QFile f(pathStr + "/" + logFileName);
        if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
            f.write(qPrintable(logString + "\r\n"));
            f.close();
        }

        qint64 size = 0;
        QDir dir(pathStr);
        //calculate total size of current directories' files
        for(QString filePath : dir.entryList(QStringList() << "*.txt", QDir::Files)) {
            QFileInfo fi(dir, filePath);
            size+= fi.size();

        }
        //qDebug() << "dir path" << size ;
        //qDebug() << logFiles;
        //ui->lineEditLogSize->setText(QString::number((float)size/1024., 'f', 2));
        ui->lineEditLogSize->setText(formatSize(size));


        if((ui->checkBoxLogClearIfSizeExceed->isChecked()) &&(size > (10*1024*1024))){
            QStringList logFiles = dir.entryList(QStringList() << "*.txt" << "*.JPG",QDir::Files);
            if((logFiles.count()>0))
                qDebug() << "remove file" << dir.absoluteFilePath(logFiles[0]) << QFile(dir.absoluteFilePath(logFiles[0])).remove();
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
    str = "\"" + str + "\"";
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

QString Dialog::formatSize(qint64 size)
{
    QStringList units = {"Bytes", "KB", "MB", "GB", "TB", "PB"};
    int i;
    double outputSize = size;
    for(i=0; i<units.size()-1; i++) {
        if(outputSize<1024) break;
        outputSize= outputSize/1024;
    }
    return QString("%0 %1").arg(outputSize, 0, 'f', 2).arg(units[i]);
}

void Dialog::sendCmd(const char* s)
{
    if(serial.isOpen()){
        QString debStr(s);
        debStr.remove('\n');
        qInfo("send: %s, sended %d", qPrintable(debStr), serial.write(s));
    }
}

void Dialog::setAudioEnable(bool bEna)
{
    appendLogString(QString("COM: set ") + QString(bEna?"audioOn":"audioOff"));
    if(bEna){
        sendCmd("audioOn\n");
    }
    else{
        sendCmd("audioOff\n");
    }
}

void Dialog::on_audioOn_clicked()
{
    setAudioEnable(true);
}

void Dialog::on_audioOff_clicked()
{
    setAudioEnable(false);
}

void Dialog::on_checkBoxAudioEnableOnStartup_clicked()
{
    settings.setValue("auidoEnableOnStartup", ui->checkBoxAudioEnableOnStartup->isChecked());
    appendLogString(QString("push button audio enable on startup %1").arg(ui->checkBoxAudioEnableOnStartup->isChecked()?"enable":"disable"));

}

void Dialog::on_pushButton_clicked()
{
    QString pathStr = ui->lineEditLogPath->text();
    if(QDir().mkpath(pathStr) == true){
        QDesktopServices::openUrl(QUrl::fromLocalFile(pathStr));
    }

}

void Dialog::updateUptime()
{
    uint64_t s = startTime.elapsed()/1000;
    int ss = s%60;
    int m = (s/60)%60;
    int h = (s/3600)%24;
    int d = (s/(3600*24));

    QString tStr;
    tStr.sprintf("%02d:%02d:%02d:%02d", d,h,m,ss);
    ui->lineEditUptime->setText(tStr);


    s = GetTickCount()/1000;
    ss = s%60;
    m = (s/60)%60;
    h = (s/3600)%24;
    d = (s/(3600*24));

    tStr.sprintf("%02d:%02d:%02d:%02d", d,h,m,ss);
    ui->lineEditSysUptime->setText(tStr);
}

void Dialog::handleLogUpdateTimeout()
{
    QString curTime = QTime::currentTime().toString();
    QString logString = curTime + ">" + "still alive, threshExceedCount: " + QString::number(distanceOverThreshCnt) + " ";
    logString += "comRecvd/err:" + QString::number(comPacketsRcvd) + "/" + QString::number(comErrorPacketsRcvd);

    appendLogFileString(logString);
}

void Dialog::on_pushButtonEncSetZero_clicked()
{
    enc1Offset = enc1Val;
    settings.setValue("encoders/enc1Offset", enc1Offset);
    //ui->lineEditEnc1Offset->setText(QString::number(enc1Offset));
    leEnc1Off->setText(QString::number(enc1Offset));

    enc2Offset = enc2Val;
    settings.setValue("encoders/enc2Offset", enc2Offset);
    //ui->lineEditEnc2Offset->setText(QString::number(enc2Offset));
    leEnc2Off->setText(QString::number(enc2Offset));

    sendPosData();

    appendLogString(QString("set new zero point: %1 %2 ").arg(enc1Offset).arg(enc2Offset));
}

void Dialog::sendPosData()
{
    CbDataUdp cbdata;
    if(horEncSelectBG->checkedId() == 0){
        cbdata.pos1 = (int16_t)((enc1Val-enc1Offset)&0x1fff);
        cbdata.pos2 = (int16_t)((enc2Val-enc2Offset)&0x1fff);
    }
    else if(horEncSelectBG->checkedId() == 1){
        cbdata.pos1 = (int16_t)((enc2Val-enc2Offset)&0x1fff);
        cbdata.pos2 = (int16_t)((enc1Val-enc1Offset)&0x1fff);
    }
    if(checkEnc1Inv->isChecked())
        cbdata.pos1 = 0x1fff - cbdata.pos1;
    if(checkEnc2Inv->isChecked())
        cbdata.pos2 = 0x1fff - cbdata.pos2;
    //cbdata.distance = (int16_t)dist;
    bool distThreshExceed = ui->checkBoxRangeAlwaysOn->isChecked() ||(distVal<rangeThresh);
    cbdata.rangeThresh = distThreshExceed? 1:0;

    ui->checkBoxThreshExcess->setChecked(distThreshExceed);

    if((distThreshExceed == true) && (lastDistTreshState == false)){
        distanceOverThreshCnt ++ ;
        ui->lineEditRangeTreshExceedCount->setText(QString::number(distanceOverThreshCnt));
    }
    lastDistTreshState = distThreshExceed;


    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
        TSenderInfo *sndInfo = (TSenderInfo*)ui->tableWidgetClients->item(r, 0)->data(Qt::UserRole).toInt();
        //qInfo() << sizeof(CbDataUdp);
        udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), sndInfo->addr, sndInfo->port);
    }
}

void Dialog::on_checkBoxRangeAlwaysOn_clicked(bool checked)
{
    //settings.setValue("rangeAlwaysOn", checked);
    sendPosData();
}


void Dialog::handleUiUpdate()
{
    ui->lineEditComPacketsRcv->setText(QString::number(comPacketsRcvd));
    ui->lineEditComPacketsRcvError->setText(QString::number(comErrorPacketsRcvd));
    leEnc1->setText(QString::number(enc1Val));
    leEnc2->setText(QString::number(enc2Val));
    ui->lineEditRange->setText(QString::number(distVal));

    on_pushButtonFindWnd_clicked();
}

void Dialog::initAppAutoStartCheckBox()
{
    ui->checkBoxAppAutostart->setToolTip(QCoreApplication::applicationFilePath());
    QSettings autoStartSettings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

    QString appAutoStartPath = autoStartSettings.value("bino-launcher").toString();
    appAutoStartPath.remove("\"");
    appAutoStartPath.replace("\\","/");

    if(appAutoStartPath == QCoreApplication::applicationFilePath())
        ui->checkBoxAppAutostart->setChecked(true);


    connect( ui->checkBoxAppAutostart, &QCheckBox::clicked,
             [=](bool bChecked){
        QSettings autoStartSettings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

        if (bChecked) {
            QString value = QCoreApplication::applicationFilePath(); //get absolute path of running exe

        #ifdef DEBUG
            ui->textEdit->append(QCoreApplication::applicationFilePath ());
        #endif

            value.replace("/","\\");
            value = "\"" + value + "\"" /*+ " --argument"*/;

        #ifdef DEBUG
            ui->textEdit->append(value);
        #endif
            //write value to the register
            autoStartSettings.setValue("bino-launcher", value);
        }
        else {
            autoStartSettings.remove("bino-launcher");
        }

    });
}

void Dialog::handleWriteCBParamsTimeout()
{
    if(serial.isWritable() == true){
        if(cbWriteParamsCount > 0){
            cbWriteParamsCount--;

            if(ui->checkBoxAudioEnableOnStartup->isChecked() == true)
                setAudioEnable(true);
        }
        else{
            //writeCbParamsTimer->stop();
        }
    }
}

void Dialog::on_pushButtonFindWnd_clicked()
{
    HWND wfp, hwndPar;
    wfp = FindWindow(NULL, L"VR");
    if(wfp == NULL){
        ui->checkBox_WS_MINIMIZE->setEnabled(false);
        ui->checkBox_WS_CAPTION->setEnabled(false);
        ui->checkBox_WS_MAXIMIZE->setEnabled(false);
        ui->checkBox_WS_POPUP->setEnabled(false);
        ui->checkBox_WS_DLGFRAME->setEnabled(false);
        ui->checkBox_WS_BORDER->setEnabled(false);
        ui->checkBox_BuildOnTop->setEnabled(false);
    }
    else{
        ui->checkBox_WS_MINIMIZE->setEnabled(true);
        ui->checkBox_WS_CAPTION->setEnabled(true);
        ui->checkBox_WS_MAXIMIZE->setEnabled(true);
        ui->checkBox_WS_POPUP->setEnabled(true);
        ui->checkBox_WS_DLGFRAME->setEnabled(true);
        ui->checkBox_WS_BORDER->setEnabled(true);
        ui->checkBox_BuildOnTop->setEnabled(true);

        hwndPar = GetParent(wfp);
    //    if(hwndPar != NULL)
    //        wfp = hwndPar;
        hwndPar = GetWindow(wfp, GW_OWNER);
    //    qInfo("%x %x", wfp, hwndPar);
        long Style = GetWindowLong(wfp, GWL_STYLE);
    //    qInfo("Style:%x", Style);

    //    if(IsIconic(wfp) == TRUE){
    //        qInfo("iconic");

    //    }
    //    qInfo() << IsIconic(wfp) << IsWindowVisible (wfp) << IsZoomed(wfp);

        uint32_t style;
        style = GetWindowLong(wfp, GWL_STYLE);
        QString str;
        str.sprintf("style: 0x%x ", style);
    //    if(style&WS_VISIBLE)
    //        str.append("WS_VISIBLE ");
    //    if(style&WS_MAXIMIZE)
    //        str.append("WS_MAXIMIZE ");
    //    if(style&WS_ICONIC)
    //        str.append("WS_ICONIC ");
    //    if(style&WS_OVERLAPPED)
    //        str.append("WS_OVERLAPPED ");
    //    if(style&WS_CAPTION)
    //        str.append("WS_CAPTION ");
        //qInfo() << qPrintable(str);

        ui->checkBox_WS_MINIMIZE->setChecked((style&WS_MINIMIZE)!=0);
        ui->checkBox_WS_CAPTION->setChecked((style&WS_CAPTION)!=0);
        ui->checkBox_WS_MAXIMIZE->setChecked((style&WS_MAXIMIZE)!=0);
        ui->checkBox_WS_POPUP->setChecked((style&WS_POPUP)!=0);
        ui->checkBox_WS_DLGFRAME->setChecked((style&WS_DLGFRAME)!=0);
        ui->checkBox_WS_BORDER->setChecked((style&WS_BORDER)!=0);

        if(ui->checkBox_WS_POPUP->isChecked() == false){
            //SetWindowLong(wfp, GWL_STYLE, WS_POPUP|WS_MAXIMIZE);
            SendMessage(wfp, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }

        if(IsZoomed(wfp)){
            //SendMessage(wfp, WM_SYSCOMMAND, SC_MINIMIZE, 0);

        }
        else{
            //SendMessage(wfp, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }

        ui->checkBox_BuildOnTop->setChecked(wfp == GetForegroundWindow());
        //qInfo("hwnd 0x%x tw 0x%x", wfp, GetForegroundWindow());


    }
}


void Dialog::handleNamReplyFinished(QNetworkReply* repl)
{
        QList<QNetworkReply::RawHeaderPair> headPairs = repl->rawHeaderPairs();

//        foreach(QNetworkReply::RawHeaderPair  p,  headPairs){
//            qDebug() << p.first << p.second;
//        }

        //qDebug() << readed;
        //qDebug() << uq.queryPairDelimiter() << uq.queryValueDelimiter();
        //uq.setQueryDelimiters('=', '|');
        //qDebug() << uq.queryPairDelimiter() << uq.queryValueDelimiter();
        //uq.allQueryItemValues()
        //qDebug() << "GUID: " << uq.queryItemValue("guid").toLatin1();

        QString reqStr = repl->request().url().toString();
        //qDebug() << "req: " << qPrintable(reqStr) << " repl: " << readed;

        if(reqStr.endsWith("login.php")){
            if(repl->error() == QNetworkReply::NoError){
                QString readed(repl->readAll());
                readed.replace('|', '&');
                QUrlQuery uq(readed);

                guid = uq.queryItemValue("guid");
                appendLogString("WEB: " + readed);
                if(guid.isEmpty() == false){
                    appendLogString("WEB: connection success");
                    setConnectionSuccess();
                    QTimer::singleShot(10*1000, this, SLOT(handlePostAliveTimeout()));
                    QTimer::singleShot(20*1000, this, SLOT(handlePostTasksTimeout()));
                    webState = connected;

                    //upload today log

                    QString tlPath = todayLogPath();
                    //QString pathStr = ui->lineEditLogPath->text();
                    QFileInfo fi(tlPath);
                    QString todayLogZipPath = fi.absoluteDir().absolutePath() + "/" + fi.baseName() + ".zip";
                    zip(tlPath, todayLogZipPath);
                    webUploadTodayLog(todayLogZipPath);


                }
                else{
                    appendLogString("WEB: no guid returned");
                    setConnectionError("no guid returned");
                    QTimer::singleShot(10*1000, this, SLOT(handleWbLoginTimeout()));
                }
            }
            else{
                setConnectionError(repl->errorString());
                QTimer::singleShot(10*1000, this, SLOT(handleWbLoginTimeout()));
            }
        }
        else if(reqStr.endsWith("tasks.php")){
            if(repl->error() == QNetworkReply::NoError){
                QString readed(repl->readAll());
                readed.replace('|', '&');
                QUrlQuery uq(readed);

                //qDebug() << "req: " << qPrintable(reqStr) << " repl: " << readed;
                QString tasks("WEB: incoming tasks: ");
                tasks += readed;
                appendLogString(tasks);                
                processTasks(readed);
            }
            else{
                appendLogString("WEB: error repl on tasks req: " + repl->errorString());
                setConnectionError(repl->errorString());
            }
            QTimer::singleShot(20*1000, this, SLOT(handlePostTasksTimeout()));

        }
        else if(reqStr.endsWith("alive.php")){

            if(repl->error() == QNetworkReply::NoError){
                QString readed(repl->readAll());
                readed.replace('|', '&');
                QUrlQuery uq(readed);

                if(readed == "ok"){
                    setConnectionSuccess();
                }
                else{
                    //qDebug() << "req: " << qPrintable(reqStr) << " repl: " << readed;
                    appendLogString("WEB: unexpected repl on alive req: " + readed);
                    setConnectionError("unexpected response");
                }
            }
            else{
                appendLogString("WEB: error repl on alive req: " + repl->errorString());
                setConnectionError(repl->errorString());
            }

            QTimer::singleShot(60*1000, this, SLOT(handlePostAliveTimeout()));
        }
        else if(reqStr.endsWith("upload_logs.php")){
            if(repl->error() == QNetworkReply::NoError){
                qDebug() << "upload_logs finished success:" << repl->readAll();

            }
            else{
                qDebug() << "upload_logs finished " << repl->errorString();

            }


        }
        else{
            //qDebug() << "unknown req: " << qPrintable(readed);
//            QString filename = saveFileName(url);
//            if (saveToDisk(filename, reply)) {
//                printf("Download of %s succeeded (saved to %s)\n",
//                       url.toEncoded().constData(), qPrintable(filename));
//            }

            QUrl url = repl->url();
            if(repl->error()){
                fprintf(stderr, "Download of %s failed: %s\n",
                        url.toEncoded().constData(),
                        qPrintable(repl->errorString()));
            }
            else{
                if (isHttpRedirect(repl)){
                    fputs("Request was redirected.\n", stderr);
                }
                else{
                    QString filename = saveFileName(url);
                    if (saveToDisk(filename, repl)) {
                        qDebug("Download of %s succeeded (saved to %s)\n",
                               url.toEncoded().constData(), qPrintable(filename));

                        QString rootPath = ui->lineEditRootPath->text();
                        QString filePath = rootPath + "/download/" + filename;

                        QFileInfo  fi(filename);
                        QString fileOutPath = rootPath + "/" + fi.baseName();
                        QDir().mkdir(fileOutPath);

                        unZip(filePath, fileOutPath);
                    }
                }
            }
        }

        if(webState == idle){

        }
        else if(webState == connected){
        }   
}

void Dialog::setConnectionError(QString errStr)
{
    //qDebug() << repl->error() << qPrintable(errStr);
    ui->lineEdit_manserver_stat->setText(QString("error: ")+errStr);
    QPalette palette;
    palette.setColor(QPalette::Base,Qt::red);
    //palette.setColor(QPalette::Text,Qt::white);
    ui->lineEdit_manserver_stat->setPalette(palette);
}

void Dialog::setConnectionSuccess()
{
    ui->lineEdit_manserver_stat->setText("connected");
    QPalette palette;
    palette.setColor(QPalette::Base,Qt::green);
    //palette.setColor(QPalette::Text,Qt::white);
    ui->lineEdit_manserver_stat->setPalette(palette);
}

void Dialog::handleWbLoginTimeout()
{
    webLogin();
}

void Dialog::handlePostAliveTimeout()
{
    webSendAlive();
}
void Dialog::handlePostTasksTimeout()
{
    webSendTasks();
}

void Dialog::webLogin()
{
    QString wbUser = ui->lineEdit_wbUser->text();
    QString wbPass = ui->lineEdit_wbPass->text();

    QString wbPath = ui->lineEdit_wbPath->text();
 //   QByteArray requestString = "login=bino&psswd=bino12345";
    QUrlQuery params;
    params.addQueryItem("login", wbUser);
    params.addQueryItem("psswd", wbPass);


    QNetworkRequest request;
    request.setUrl(QUrl(wbPath+"/bino/login.php"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

    nam->post(request, params.toString().toLatin1());
}

void Dialog::webSendAlive()
{
    QString wbPath = ui->lineEdit_wbPath->text();
    QUrlQuery params;
    params.addQueryItem("guid", guid.toLatin1());

    QNetworkRequest request;
    request.setUrl(QUrl(wbPath+"/bino/alive.php"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

    nam->post(request, params.toString().toLatin1());
}

void Dialog::webSendTasks()
{
    QString wbPath = ui->lineEdit_wbPath->text();
    QUrlQuery params;
    params.addQueryItem("guid", guid.toLatin1());

    QNetworkRequest request;
    request.setUrl(QUrl(wbPath+"/bino/tasks.php"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

    nam->post(request, params.toString().toLatin1());
}

void Dialog::webUploadTodayLog(QString todayLogPath)
{
    QHttpMultiPart * data = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));
    part.setHeader(QNetworkRequest::ContentDispositionHeader,  QVariant("form-data; name=\"text\""));
    QUrlQuery params;
    params.addQueryItem("guid", guid.toLatin1());
    //QString body = QString("guid=") + guid;
    //part.setBody( body.toLatin1());
    part.setBody( params.toString().toLatin1());
    //qDebug() << params.toString().toLatin1();
    //part.setRawHeader("Authorization", body.toLatin1());
    data->append(part);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("multipart/form-data; name=\"log\"; filename=\"log.zip\""));
    filePart.setRawHeader("Content-Transfer-Encoding","binary");

    QFile *file = new QFile(todayLogPath);
    if(file->open(QIODevice::ReadOnly) == false){
        appendLogString("WEB: error today log file open");
        return;
    }
    QByteArray ba = file->readAll();

//    filePart.setBodyDevice(file);
    data->append(filePart);

    QString wbPath = ui->lineEdit_wbPath->text();
    nam->post(QNetworkRequest(QUrl(wbPath+"/bino/upload_logs.php")), data);

    file->close();

//    //QString wbPath = ui->lineEdit_wbPath->text();
//    QUrlQuery params;
//    params.addQueryItem("guid", guid.toLatin1());
//    params.addQueryItem("data", ba);
//    //params.addQueryItem();

//    QNetworkRequest request;
//    request.setUrl(QUrl(wbPath+"/bino/upload_logs.php"));
//    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

////    //nam->post(request, params.toString().toLatin1());
//    nam->post(request, params.toString(QUrl::FullyEncoded).toUtf8());


//    QNetworkRequest request(QUrl(wbPath+"/bino/upload_logs.php")); //our server with php-script
//     QString bound="margin"; //name of the boundary
//     //according to rfc 1867 we need to put this string here:
//     QByteArray data = ("--margin\r\n");
//     data.append("Content-Disposition: form-data; name=\"upload\"; filename=\"log.zip\"\r\n");
//     data.append("Content-Type: application/octet-stream\r\n\r\n");
//     data.append(body.toLatin1());

////     QFile file(fileInfo.absoluteFilePath());
////     if (!file.open(QIODevice::ReadOnly))
////     return;
//     data.append(file->readAll()); //let's read the file
//     data.append("\r\n");
//     data.append("--margin--\r\n"); //closing boundary according to rfc 1867
//     request.setRawHeader("Content-Type","multipart/form-data; boundary=margin");
//     request.setRawHeader("Content-Length", QString::number(data.length()).toUtf8());
//     //connect(manager, SIGNAL(finished(QNetworkReply*)), SLOT(sendingFinished(QNetworkReply*)));
//     QNetworkReply *reply = nam->post(request,data);

}

void Dialog::on_lineEdit_wbPath_editingFinished()
{
    qDebug() << "on_lineEdit_wbPath_editingFinished";

    QString wbPath = ui->lineEdit_wbPath->text();
    settings.setValue("webManger/path", wbPath);
}


void Dialog::on_lineEdit_wbUser_editingFinished()
{
    qDebug() << "on_lineEdit_wbUser_editingFinished";
    QString wbUser = ui->lineEdit_wbUser->text();
    settings.setValue("webManger/user", wbUser);
}

void Dialog::on_lineEdit_wbPass_editingFinished()
{
    qDebug() << "on_lineEdit_wbPass_editingFinished";
    QString wbPass = ui->lineEdit_wbPass->text();
    settings.setValue("webManger/pass", wbPass);
}


void Dialog::processTasks(QString uq)
{
    qDebug() << qPrintable(uq);
    QStringList strList = uq.split(";");

    foreach(QString part, strList) {
        qDebug() << "task: " << part;
        processTask(part);
        qDebug() << " ";
    }
}

void Dialog::processTask(QString task)
{
    QStringList taskParts = task.split("&");
    if(taskParts.count() != 3){
        QString errStr = "Err task: " + task;
        appendLogString(errStr);
        return;
    }
    QString taskType = taskParts[1];
    if(taskType == "install_program"){
        appendLogString("install program task");
        qDebug() << "install program task " << taskParts;
        QStringList pathParts = taskParts[2].split("!");
        qDebug() << "install program task path " << pathParts;
        if(pathParts.count() != 2){
            QString errStr = "Err install task path: " + taskParts[2];
            appendLogString(errStr);
            return;
        }
        installProgram(pathParts[1]);
    }
    else if(taskType == "install_project"){
        appendLogString("install project task");
        qDebug() << "install project task";
    }
    else if(taskType == "uninstall_program"){
//        appendLogString("uninstall program task");
//        qDebug() << "uninstall program task";
//        uninstallProgram("");
    }
    else if(taskType == "uninstall_project"){
//        appendLogString("uninstall project task");
//        qDebug() << "uninstall project task";
    }
    else if(taskType == "upload"){
//        appendLogString("upload task");
//        qDebug() << "upload task";
    }
    else if(taskType == "delete"){
//        appendLogString("delete task");
//        qDebug() << "delete task";
    }
    else if(taskType == "restart"){
//        appendLogString("restart task");
//        qDebug() << "restart task";
//        restart();
    }
}

void Dialog::installProgram(QString path)
{
    qDebug(qPrintable(QString("install task: ") + path));
    QString wbPath = ui->lineEdit_wbPath->text();
    QUrl fileUrl(wbPath+path);
    appendLogString("download path: \"" + fileUrl.toString() + "\"");
    qDebug() << fileUrl;
    QNetworkRequest request(fileUrl);

    QNetworkReply *repl =  nam->get(request);
    connect(repl, &QNetworkReply::downloadProgress,
            [](qint64 bytesReceived, qint64 bytesTotal){
        qDebug("download process %d %d", bytesReceived, bytesTotal);
    });


}

void Dialog::uninstallProgram(QString path)
{

}

void Dialog::restart()
{

}

QString Dialog::saveFileName(const QUrl &url)
{
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())
        basename = "download";

    if (QFile::exists(basename)) {
        // already exists, don't overwrite
        int i = 0;
        basename += '.';
        while (QFile::exists(basename + QString::number(i)))
            ++i;

        basename += QString::number(i);
    }

    //basename = "download\\" + basename;

    return basename;
}

bool Dialog::saveToDisk(const QString &filename, QIODevice *data)
{
    QString rootPath = ui->lineEditRootPath->text();
    QDir().mkdir(rootPath + "/download/");
    QString filePath = rootPath + "/download/" + filename;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        appendLogString(QString("Could not open %1 for writing: %2\n")
                        .arg(filePath).arg(file.errorString()));
        return false;
    }

    file.write(data->readAll());
    file.close();

    return true;
}

bool Dialog::isHttpRedirect(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303
           || statusCode == 305 || statusCode == 307 || statusCode == 308;
}


void Dialog::zip(QString filename, QString zip_filename)
{
    qDebug() << "compress " << qPrintable(filename) << " to "
             << qPrintable(zip_filename);
    QProcess *zipProc = new QProcess(this);
    //zipProc->setWorkingDirectory(fileOutPath);
    QStringList pars;
    pars.append("-9");
    pars.append("-j");
    pars.append(zip_filename);
    pars.append(filename);
    zipProc->start("zip", pars);
    zipProc->waitForStarted(-1);
    //unZipProc->waitForFinished(-1);
    connect(zipProc,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus){
            qDebug("zipProc finished");
        });
}

void Dialog::unZip(QString zip_filename , QString outPath)
{
    QProcess *unZipProc = new QProcess(this);
    unZipProc->setWorkingDirectory(outPath);
    unZipProc->start("unzip", QStringList(zip_filename));
    unZipProc->waitForStarted(-1);
    //unZipProc->waitForFinished(-1);
    connect(unZipProc,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus){
            qDebug("unZipProc finished");
        });
}

//bool extract(const QString & filePath, const QString & extDirPath, const QString & singleFileName)
//{

//    QuaZip zip(filePath);

//    if (!zip.open(QuaZip::mdUnzip)) {
//        qWarning("testRead(): zip.open(): %d", zip.getZipError());
//        return false;
//    }

//    zip.setFileNameCodec("IBM866");

//    qWarning("%d entries\n", zip.getEntriesCount());
//    qWarning("Global comment: %s\n", zip.getComment().toLocal8Bit().constData());

//    QuaZipFileInfo info;

//    QuaZipFile file(&zip);

//    QFile out;
//    QString name;
//    char c;
//    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {

//        if (!zip.getCurrentFileInfo(&info)) {
//            qWarning("testRead(): getCurrentFileInfo(): %d\n", zip.getZipError());
//            return false;
//        }

//        if (!singleFileName.isEmpty())
//            if (!info.name.contains(singleFileName))
//                continue;

//        if (!file.open(QIODevice::ReadOnly)) {
//            qWarning("testRead(): file.open(): %d", file.getZipError());
//            return false;
//        }

//        name = QString("%1/%2").arg(extDirPath).arg(file.getActualFileName());

//        if (file.getZipError() != UNZ_OK) {
//            qWarning("testRead(): file.getFileName(): %d", file.getZipError());
//            return false;
//        }

//        //out.setFileName("out/" + name);
//        out.setFileName(name);

//        // this will fail if "name" contains subdirectories, but we don't mind that
//        out.open(QIODevice::WriteOnly);
//        // Slow like hell (on GNU/Linux at least), but it is not my fault.
//        // Not ZIP/UNZIP package's fault either.
//        // The slowest thing here is out.putChar(c).
//        while (file.getChar(&c)) out.putChar(c);

//        out.close();

//        if (file.getZipError() != UNZ_OK) {
//            qWarning("testRead(): file.getFileName(): %d", file.getZipError());
//            return false;
//        }

//        if (!file.atEnd()) {
//            qWarning("testRead(): read all but not EOF");
//            return false;
//        }

//        file.close();

//        if (file.getZipError() != UNZ_OK) {
//            qWarning("testRead(): file.close(): %d", file.getZipError());
//            return false;
//        }
//    }

//    zip.close();

//    if (zip.getZipError() != UNZ_OK) {
//        qWarning("testRead(): zip.close(): %d", zip.getZipError());
//        return false;
//    }

//    return true;
//}

