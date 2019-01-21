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
#include <QMessageBox>

#include "unity.h"
#include "demo.h"
//#include <foldercompressor.h>
//#define QUAZIP_STATIC

//#include "quazip/quazip.h"
//#include "quazip/quazipfile.h"


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    //enc1(0),enc2(0),r(0),
    settings("Polden Ltd.", "bino-launcher"),   
    distanceOverThreshCnt(0),
    lastDistTreshState(false)
    //settings("murinets", "binoc-launcher")
{
    ui->setupUi(this);    


    startTime.start();
    updateUptime();
    QString compileDateTime = QString(__DATE__) + " " + QString(__TIME__);
    ui->label_buildTime->setText(compileDateTime);

    QString hostName = QHostInfo::localHostName();
//    QString logDirPath = settings.value("log/logPath").toString();
//    if(logDirPath.isEmpty()){
//        logDirPath = QDir::currentPath() + "/logs";
//    }

//    if(logDirPath.endsWith(hostName) == false){
//        logDirPath+= "_" + hostName;
//    }

    on_pushButtonFindWnd_clicked();

    //root dir
    QString rootPath = settings.value("rootPath").toString();
    if(rootPath.isEmpty()){
        rootPath = QDir::currentPath();
    }
    initPathStruct(rootPath);
    initRootPathControl();

    ui->widget_deleteOldLogsContainer->hide();
    ui->checkBoxLogClearIfSizeExceed->setChecked(settings.value("log/logCLearIfSizeExceed", true).toBool());

    ui->lineEditLogPath->setText(dirStruct.logsDir);
    appendLogFileString("\r\n");
    appendLogFileString("--- start");
    appendLogString(QString("root dir:\"")+(dirStruct.rootDir.isEmpty()? "n/a":dirStruct.rootDir)+ "\"");
    appendLogString(QString("log dir:\"")+(dirStruct.logsDir.isEmpty()? "n/a":dirStruct.logsDir)+ "\"");

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

    //debugPosTimer.setInterval(20);
    //QObject::connect(&debugPosTimer, SIGNAL(timeout()), this, SLOT(debugTimerOut()));
    //debugPosTimer.start();       

    bool bAudioEnableOnStartup = settings.value("auidoEnableOnStartup", true).toBool();
    appendLogString(QString("restore audio enable on startUp: ")+(bAudioEnableOnStartup? "true":"false"));
    ui->checkBoxAudioEnableOnStartup->setChecked(bAudioEnableOnStartup);

    initComPort();        
    com->rangeThresh = settings.value("rangeThresh", 20).toInt();

    demo = new Demo(this);
    connect(demo, SIGNAL(newPosData(uint16_t,uint16_t,int)), SLOT(handleComNewPosData(uint16_t,uint16_t,int)));
    connect(demo, &Demo::msg, [=](QString msg){
        appendLogString("DEMO:"+msg);
    });


    ui->lineEditRangeThresh->setValidator( new QIntValidator(0, 100, this) );
    ui->lineEditRangeThresh->setText(QString::number(com->rangeThresh));

    ui->lineEditRange->setText("n/a");
    ui->lineEditRange_2->setText("n/a");
    connect(ui->checkBoxRangeAlwaysOn, &QCheckBox::clicked, [=](bool checked){
        sendPosData();

    });

    //ui->lineEditDebugR->setValidator(new QIntValidator(0,50, this));
    //ui->lineEditDebugX->setValidator(new QIntValidator(0,8191, this));
    //ui->lineEditDebugY->setValidator(new QIntValidator(0,8191, this));



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
    ui->tableWidgetClients->setColumnCount(3);
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
    connect(uiUpdateTimer, SIGNAL(timeout()), this, SLOT(handleUpdateUi()));
    uiUpdateTimer->setInterval(250);
    uiUpdateTimer->start();

    uiUpdateTimer = new QTimer(this);
    connect(uiUpdateTimer, SIGNAL(timeout()),
            this, SLOT(handleUpdateUiHardwareParams()));
    uiUpdateTimer->setInterval(2000);
    if(securityStart() == true){
        uiUpdateTimer->start();
    }



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

    connect(ui->lineEdit_wbPath, &QLineEdit::editingFinished, [=](){
        //qDebug() << "on_lineEdit_wbPath_editingFinished";
        QString wbPath = ui->lineEdit_wbPath->text();
        if(web->wbPath != wbPath){
            web->wbPath = wbPath;
            settings.setValue("webManger/path", wbPath);
            appendLogString("WEB: url set \"" + wbPath + "\"");
        }
    });
    connect(ui->lineEdit_wbUser, &QLineEdit::editingFinished, [=](){
        //qDebug() << "on_lineEdit_wbUser_editingFinished";
        QString wbUser = ui->lineEdit_wbUser->text();
        if(web->wbUser != wbUser){
            web->wbUser = wbUser;
            settings.setValue("webManger/user", wbUser);
            appendLogString("WEB: user set \"" + wbUser + "\"");
        }
    });
    connect(ui->lineEdit_wbPass, &QLineEdit::editingFinished, [=](){
        //qDebug() << "on_lineEdit_wbPass_editingFinished";
        QString wbPass = ui->lineEdit_wbPass->text();
        if(web->wbPass != wbPass){
            web->wbPass = wbPass;
            settings.setValue("webManger/pass", wbPass);
            appendLogString("WEB: pass set \"" + wbPass + "\"");
        }
    });

    QPalette palette;
    palette.setColor(QPalette::Base,Qt::gray);
    ui->lineEdit_manserver_stat->setPalette(palette);
    ui->lineEdit_manserver_stat->setText("n/a");

    //QTimer::singleShot(100, this, SLOT(handleWbLoginTimeout()));

    bool bDemoEna = settings.value("demoEnable", 10).toBool();
    appendLogString(QString("DEMO: ")+(bDemoEna? "enable":"disable"));
    ui->checkBox_demoEna->setChecked(bDemoEna);
    on_checkBox_demoEna_clicked(bDemoEna);


    web = new Web(this);
    web->dirStruct = &dirStruct;
    web->wbUser = wbUser;
    web->wbPass = wbPass;
    web->wbPath = wbPath;

    connect(web, &Web::msg, [=](QString msg){
        appendLogString("WEB:"+msg);
    });
    connect(web, &Web::connSuccess, [=](){
        setConnectionSuccess();
    });
    connect(web, &Web::connError, [=](QString msg){
        setConnectionError(msg);
    });

    web->start();

    //unity
    initUnity();

    QTime startTime;
    startTime.start();
    float temp;
//    GetCpuTemperature(&temp);
//    qDebug("temp=%f\n", temp);
//    GetCpuTemperature(&temp);
//    qDebug("temp=%f\n", temp);
//    GetCpuTemperature(&temp);
//    qDebug("temp=%f\n", temp);


}



Dialog::~Dialog()
{
    settings.setValue("rangeThresh", com->rangeThresh);

    appendLogString("--- quit");
    delete ui;
}

void Dialog::initComPort()
{
    com = new Com(this);
    connect(com, SIGNAL(newPosData(uint16_t,uint16_t,int)), SLOT(handleComNewPosData(uint16_t,uint16_t,int)));
    connect(com, &Com::msg, [=](QString msg){
        appendLogString("COM:"+msg);
    });

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
//                if(ui->checkBoxAudioEnableOnStartup->isChecked() == true)
//                    com->setAudioEnable(true);
                //if(ui->checkBoxInitOnStart->isChecked()){
                //    on_pushButtonInitiate_clicked();
                //}
                break;
            }
        }
    }

    connect(ui->pushButtonSelectFwPath, &QPushButton::clicked, [=](){
        QString fwPath = ui->lineEdit_fwPath->text();
        QFileInfo fwFi(fwPath);

        if(fwPath.isEmpty()){
            fwPath = QDir::currentPath();
        }
        fwPath = QFileDialog::getOpenFileName(this, tr("Select fw file"),
                                                        fwFi.absoluteDir().dirName(),
                                                        tr("Firmware HEX (*.hex)"));
        if(fwPath.isEmpty() == false){
            appendLogString(QString("new fw file selected:\"") + fwPath + "\"");
            settings.setValue("rootPath", fwPath);
            ui->lineEdit_fwPath->setText(fwPath);
            ui->lineEdit_fwPath->setToolTip(fwPath);
        }

        //appendLogString();
    });


    //void updateUptime(QString&);
    connect(com, QOverload<QString&>::of(&Com::updateUptime), [=](QString& comUptime){
        ui->lineEdit_ComUptime->setText(comUptime);
    });
}

void Dialog::initRootPathControl()
{

    connect(ui->lineEditRootPath, &QLineEdit::editingFinished,
            [=](){
        QString rootPath = ui->lineEditRootPath->text();
        //web->rootPath = ui->lineEditRootPath->text();
        initPathStruct(rootPath);

    });

    connect(ui->pushButtonSelectRootPath, &QPushButton::clicked,
        [this](){
        QString rootPath = ui->lineEditRootPath->text();
        if(rootPath.isEmpty()){
            rootPath = QDir::currentPath();
        }
        rootPath = QFileDialog::getExistingDirectory(this, tr("Select root dir"),
                                                        rootPath,
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
        if(rootPath.isEmpty() == false){
            appendLogString(QString("new root path selected:\"") + rootPath + "\"");
            settings.setValue("rootPath", rootPath);
            ui->lineEditRootPath->setText(rootPath);
            ui->lineEditRootPath->setToolTip(rootPath);
            initPathStruct(rootPath);
        }
    });

    connect(ui->pushButton_showRootPath, &QPushButton::clicked, [=](){
        QString pathStr = ui->lineEditRootPath->text();
        if(QDir().mkpath(pathStr) == true){
            QDesktopServices::openUrl(QUrl::fromLocalFile(pathStr));
        }
    });
}

void Dialog::initPathStruct(QString rootDir)
{
    dirStruct.rootDir = rootDir;
    dirStruct.downloadDir = rootDir + "/download";
    dirStruct.logsDir = rootDir + "/logs_" + QHostInfo::localHostName();


    QDir().mkdir(dirStruct.rootDir);        
    QDir().mkdir(dirStruct.downloadDir);
    QDir().mkdir(dirStruct.logsDir);

//    qDebug() << "root dir: \"" << qPrintable(dirStruct.rootDir) << "\"";
//    qDebug() << "download dir: \"" << qPrintable(dirStruct.downloadDir) << "\"";
//    qDebug() << "logs dir: \"" << qPrintable(dirStruct.logsDir) <<  "\"";

    ui->lineEditRootPath->setText(dirStruct.rootDir);
    ui->lineEditLogPath->setText(dirStruct.logsDir);


}

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

    com->enc1Offset = settings.value("encoders/enc1Offset", 0).toInt();
    //ui->lineEditEnc1Offset->setText(QString::number(enc1Offset));
    leEnc1Off->setText(QString::number(com->enc1Offset));

    com->enc2Offset = settings.value("encoders/enc2Offset", 0).toInt();
    //ui->lineEditEnc2Offset->setText(QString::number(enc2Offset));
    leEnc2Off->setText(QString::number(com->enc2Offset));

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

void Dialog::initUnity()
{

    unity = new Unity(this);

    QString unityBuildPath = settings.value("watchdog/unityBuildExePath").toString();
    appendLogString(QString("restore unity path:\"")+(unityBuildPath.isEmpty()? "n/a":unityBuildPath)+ "\"");
    ui->lineEditBuildPath->setText(unityBuildPath);
    ui->lineEditBuildPath->setToolTip(unityBuildPath);
    unity->setUnityPath(unityBuildPath);
    connect(ui->pushButtonWdSelectPath, &QPushButton::clicked, [=](){
        QString str = QFileDialog::getOpenFileName(this,
           tr("Select unity build exe file"), "/home/jana", tr("Unity Build exe file (*.exe *.bat)"));
        if(str.isEmpty() == false){
            settings.setValue("watchdog/unityBuildExePath", str);
            ui->lineEditBuildPath->setText(str);
            ui->lineEditBuildPath->setToolTip(str);
            unity->setUnityPath(str);
        }
    });

    QString unityBuildStartupParams = settings.value("watchdog/unityBuildStartupParams").toString();
    appendLogString(QString("restore unity startup params:\"")+(unityBuildStartupParams.isEmpty()? "n/a":unityBuildStartupParams)+ "\"");
    ui->lineEditBuldStartupParams->setText(unityBuildStartupParams);
    ui->lineEditBuldStartupParams->setToolTip(unityBuildStartupParams);
    unity->setUnityStartupParams(unityBuildStartupParams);
    connect(ui->lineEditBuldStartupParams, &QLineEdit::editingFinished, [=](){
        QString params =  ui->lineEditBuldStartupParams->text();
        if(params.isEmpty() == false){
            appendLogString(QString("WD:new unity cmd params:\"")+params+ "\"");
            settings.setValue("watchdog/unityBuildStartupParams", params);
            unity->setUnityStartupParams(params);
        }
    });

    ui->lineEditWdTimeOutSecs->setValidator(new QIntValidator(10,999, this));
    QString wdTo = settings.value("watchdog/timeout", 10).toString();
    appendLogString(QString("WD:restore watchdog time out:\"")+(wdTo.isEmpty()? "n/a":wdTo)+ "\"");
    ui->lineEditWdTimeOutSecs->setText(wdTo);
    unity->setWdTimeOutSecs(wdTo.toInt());
    connect(ui->lineEditWdTimeOutSecs, &QLineEdit::editingFinished, [=](){
        bool bOk;
        int wto = ui->lineEditWdTimeOutSecs->text().toInt(&bOk);
        if(bOk == true){
            appendLogString(QString("WD:set watchdog timeOut:\"")+QString::number(wto)+ "\"");
            settings.setValue("watchdog/timeout", wto);
            unity->setWdTimeOutSecs(wto);
        }
        else{
            appendLogString(QString("WD:set watchdog timeOut fail value\"")+(ui->lineEditWdTimeOutSecs->text())+ "\"");
        }
    });

    ui->lineEditFpsLimit->setValidator(new QIntValidator(1,999, this));
    QString fpsLimit = settings.value("watchdog/fpsLimit", 30).toString();
    appendLogString(QString("WD:restore fps limit:\"")+(fpsLimit.isEmpty()? "n/a":fpsLimit)+ "\"");
    ui->lineEditFpsLimit->setText(fpsLimit);
    unity->setFpsLimit(fpsLimit.toInt());

    connect(ui->lineEditFpsLimit, &QLineEdit::editingFinished, [=](){
        bool bOk;
        int fpsLimit = ui->lineEditFpsLimit->text().toInt(&bOk);
        if(bOk == true){
            appendLogString(QString("WD:set fps limit:\"%1\"").arg(fpsLimit));
            settings.setValue("watchdog/fpsLimit", fpsLimit);
            unity->setFpsLimit(fpsLimit);
        }
        else{
            appendLogString(QString("WD:set watchdog timeOut fail value\"")+(ui->lineEditFpsLimit->text())+ "\"");
        }
    });

    bool wdEna = settings.value("watchdog/ena", false).toBool();
    wdEna = true;
    ui->checkBoxWdEnable->setChecked(wdEna);
    unity->setWdEnable(wdEna);
    connect(ui->checkBoxWdEnable, &QCheckBox::clicked, [=](bool bChecked){
        settings.setValue("watchdog/ena", bChecked);
        appendLogString(QString("watchdog %1").arg(bChecked? "on" : "off"));
        unity->setWdEnable(bChecked);
    });

    connect(ui->pushButtonWDTest, &QPushButton::clicked, [=](){
        appendLogString("WD: test button");
        unity->restartUnityBuild();
    });

    connect(unity, &Unity::msg, [=](QString msg){
        appendLogString("UB:"+msg);
    });

    connect(unity, &Unity::needHwRestart, [=](){
        com->hwRestart();
    });


    bool startUnityOnStart = settings.value("watchdog/startUnityOnStart", false).toBool();
    if(settings.contains("watchdog/startUnityOnStart") == true){
        appendLogString(QString("WD:restore startUnityOnStart: ")+QString(startUnityOnStart?"on":"off"));
    }
    ui->checkBox_startUnityOnStart->setChecked(startUnityOnStart);
    connect(ui->checkBox_startUnityOnStart, &QCheckBox::clicked, [=](bool bChecked){
        settings.setValue("watchdog/startUnityOnStart", bChecked);
        appendLogString(QString("startUnityOnStart %1").arg(bChecked? "on" : "off"));
    });

    connect(unity, &Unity::newWindowState, [=](uint32_t style){
        updateWindowState(style);
    });
    unity->start();
    if(startUnityOnStart == true)
        unity->restartUnityBuild();
}
//void Dialog::handleReadPendingDatagrams()
//{
//    //qDebug("handleReadPendingDatagrams");
//    while (udpSocket->hasPendingDatagrams()) {
//        QNetworkDatagram datagram = udpSocket->receiveDatagram();

//        if(datagram.isValid() == false)
//            continue;

//        QString sAddrStr = datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort());

//        QString dData(datagram.data());
//        if(dData == "reg"){
//            bool bExist = false;

//            for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
//                QString itText = ui->tableWidgetClients->item(r, 0)->text();
//                if(itText == sAddrStr)
//                    bExist = true;
//            }
//            appendLogString("UDP client reg: "+datagram.senderAddress().toString() + ":" + QString::number(datagram.senderPort()));
//            if(bExist == false){
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
//            }
//        }
//        else if(dData.startsWith("hba") == true){
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

//        }
//        else{
//            qDebug()<< datagram.senderAddress() << datagram.senderPort() << ":" << datagram.data();
//        }



//    }
//    //qDebug("handleReadPendingDatagrams end");

//}



//#pragma pack(push,1)
//typedef struct{
//    uint16_t pos1;
//    uint16_t pos2;
//    uint8_t rangeThresh;
//} CbDataUdp;
//#pragma pack(pop)

//void Dialog::debugTimerOut()
//{
//    enc1++;
//    enc2 = 1278;
//    if(enc1 >=8192)
//        enc1 = 0;
//    CbDataUdp cbdata;
//    cbdata.pos1 = enc1;
//    cbdata.pos2 = enc2;
//    //cbdata.distance = 12;

//    //ui->lineEditEnc1->setText(QString::number(enc1));
//    //ui->lineEditEnc2->setText(QString::number(enc2));
//    //ui->lineEditRange->setText(QString::number(cbdata.distance));

//    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
//        TSenderInfo *sndInfo = (TSenderInfo*)ui->tableWidgetClients->item(r,0)->data(Qt::UserRole).toInt();
//        udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), sndInfo->addr, sndInfo->port);
//    }


//}

void Dialog::on_pushButtonComOpen_clicked()
{        
     if(ui->pushButtonComOpen->text() == "open"){         
         if(com->isOpen() == false){
             QString comName = ui->comComboBox->currentText();
             if(comName.length() > 0){
                 //UartThread.requestToStart(comName);
                 com->setPortName(comName);
                 if (com->open() == false) {
                     qDebug("%s port open FAIL", qUtf8Printable(comName));
                     appendLogString(QString("COM: %1 port open FAIL").arg(comName));
                     return;
                 }
//                 qDebug("%s port opened", qUtf8Printable(comName));
                 appendLogString(QString("COM: port %1 opened").arg(com->portName()));
//                 connect(&serial, SIGNAL(bytesWritten(qint64)),
//                         this, SLOT(handleSerialDataWritten(qint64)));
                 ui->pushButtonComOpen->setText("close");
                 //emit showStatusBarMessage("connected", 3000);
                 //ui->statusBar->showMessage("connected", 2000);                                                                    
                 settings.setValue("usbMain", comName);

                 recvdComPacks = 0;
                 startRecvTime = QTime::currentTime();
                 cbWriteParamsCount = 3;
             }
         }
     }
     else{
         com->close();
         //udpSocket->close();
//         qDebug("port closed");
         ui->pushButtonComOpen->setText("open");
         //contrStringQueue.clear();
         //ui->statusBar->showMessage("disconnected", 2000);
         appendLogString(QString("COM: port %1 closed").arg(com->portName()));
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

    QString logStr("COM: ports present in system: ");
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


void Dialog::on_pushButtonSet_clicked()
{
    com->rangeThresh = ui->lineEditRangeThresh->text().toInt();
    qDebug("thresh: %d", com->rangeThresh);
}

void Dialog::on_pushButtonDebugSend_clicked()
{
//    int x,y,r;
    //x = ui->lineEditDebugX->text().toInt();
    //y = ui->lineEditDebugY->text().toInt();
    //r = ui->lineEditDebugR->text().toInt();
//    CbDataUdp cbdata;
//    cbdata.pos1 = (int16_t)x;
//    cbdata.pos2 = (int16_t)y;
//    cbdata.rangeThresh = (int16_t)r;
//    for(int r=0; r<ui->listWidgetClients->count(); r++){
//        TSenderInfo *sndInfo = (TSenderInfo*)ui->listWidgetClients->item(r)->data(Qt::UserRole).toInt();
//        udpSocket->writeDatagram((const char*)&cbdata, sizeof(CbDataUdp), sndInfo->addr, sndInfo->port);
//    }

}

//void Dialog::on_lineEditLogPath_editingFinished()
//{
//    QString pathStr = ui->lineEditLogPath->text();
//    ui->lineEditLogPath->setToolTip(pathStr);
//    settings.setValue("log/logPath", pathStr);
//}

//void Dialog::on_pushButtonLogSelectPath_clicked()
//{
//    QString str = QFileDialog::getExistingDirectory(this, tr("Select dir for logs"));
//    if(str.isEmpty() == false){
//        //settings.setValue("watchdog/unityBuildExePath", str);
//        ui->lineEditLogPath->setText(str);
//        //ui->lineEditLogPath->setToolTip(str);
//        on_lineEditLogPath_editingFinished();
//    }
//}

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
    QString pathStr = dirStruct.logsDir; //ui->lineEditLogPath->text();
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

void Dialog::on_audioOn_clicked()
{
    com->setAudioEnable(true);
}

void Dialog::on_audioOff_clicked()
{
    com->setAudioEnable(false);
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
    logString += "comRecvd/err:" + QString::number(com->comPacketsRcvd) + "/" + QString::number(com->comErrorPacketsRcvd);

    appendLogFileString(logString);
}

void Dialog::on_pushButtonEncSetZero_clicked()
{
    com->setZero();

    settings.setValue("encoders/enc1Offset", com->enc1Offset);
    //ui->lineEditEnc1Offset->setText(QString::number(enc1Offset));
    leEnc1Off->setText(QString::number(com->enc1Offset));

    settings.setValue("encoders/enc2Offset", com->enc2Offset);
    //ui->lineEditEnc2Offset->setText(QString::number(enc2Offset));
    leEnc2Off->setText(QString::number(com->enc2Offset));

    sendPosData();

    appendLogString(QString("set new zero point: %1 %2 ").arg(com->enc1Offset).arg(com->enc2Offset));
}

void Dialog::sendPosData()
{
    Unity::CbDataUdp cbdata;
    if(horEncSelectBG->checkedId() == 0){
        cbdata.pos1 = (int16_t)((com->enc1Val-com->enc1Offset)&0x1fff);
        cbdata.pos2 = (int16_t)((com->enc2Val-com->enc2Offset)&0x1fff);
    }
    else if(horEncSelectBG->checkedId() == 1){
        cbdata.pos1 = (int16_t)((com->enc2Val-com->enc2Offset)&0x1fff);
        cbdata.pos2 = (int16_t)((com->enc1Val-com->enc1Offset)&0x1fff);
    }
    if(checkEnc1Inv->isChecked())
        cbdata.pos1 = 0x1fff - cbdata.pos1;
    if(checkEnc2Inv->isChecked())
        cbdata.pos2 = 0x1fff - cbdata.pos2;
    //cbdata.distance = (int16_t)dist;
    bool distThreshExceed = ui->checkBoxRangeAlwaysOn->isChecked() ||(com->distVal<com->rangeThresh);
    cbdata.rangeThresh = distThreshExceed? 1:0;

    ui->checkBoxThreshExcess->setChecked(distThreshExceed);

    if((distThreshExceed == true) && (lastDistTreshState == false)){
        distanceOverThreshCnt ++ ;
        ui->lineEditRangeTreshExceedCount->setText(QString::number(distanceOverThreshCnt));
    }
    lastDistTreshState = distThreshExceed;

//    for(int r=0; r<ui->tableWidgetClients->rowCount(); r++){
//        TSenderInfo *sndInfo = (TSenderInfo*)ui->tableWidgetClients->item(r, 0)->data(Qt::UserRole).toInt();
//        //qInfo() << sizeof(CbDataUdp);
//        udpSocket->writeDatagram((const char*)&cbdata, sizeof(Unity::CbDataUdp), sndInfo->addr, sndInfo->port);
//    }

    unity->sendCbData(cbdata);
}



void Dialog::handleUpdateUi()
{
    QTime startTime;
    startTime.start();

    ui->lineEditComPacketsRcv->setText(QString::number(com->comPacketsRcvd));
    ui->lineEditComPacketsRcvError->setText(QString::number(com->comErrorPacketsRcvd));
    QString enc1Str = com->enc1Val == -1 ? "n/a": QString::number(com->enc1Val);
    QString enc2Str = com->enc2Val == -1 ? "n/a": QString::number(com->enc2Val);
    QString distStr = com->distVal == -1? "n/a": QString::number(com->distVal);
    leEnc1->setText(enc1Str);
    leEnc2->setText(enc2Str);
    ui->lineEditRange->setText(distStr);

    //on_pushButtonFindWnd_clicked();

    QString state = QVariant::fromValue(demo->demoModeState).value<QString>();
    ui->lineEdit_ComDemoState->setText(state);    
    float demoRemTime = 0;
    if(demo->demoModeState != Demo::idle)
        demoRemTime = demo->demoModeCurStepInd/(1000/demo->demoModePeriod.interval());

    ui->lineEdit_ComDemoTime->setText(QString::number((int)demoRemTime/*, 'f', 1*/));
    ui->lineEdit_comFwmVer->setText(com->firmwareVer);

    ui->lineEdit_ComDemoModeCycleCounter->setText(QString::number(demo->demoCycleCount));

    //update unity section
    //check if all exists
    QTableWidget &tableWdg = *(ui->tableWidgetClients);
    QMap<QString, TSenderInfo> &sm = unity->clientsMap;
    foreach (QString n, sm.keys()) {
        bool bExist = false;
        int rc = tableWdg.rowCount();
        for(int ri=0; ri<rc; ri++){
            QTableWidgetItem *twi = tableWdg.item(ri, 0);
            if(twi->text() == n)
                bExist = true;
        }
        if(bExist == false){
            tableWdg.setRowCount(rc+1);
            QTableWidgetItem *twi = new QTableWidgetItem(n);
            twi->setTextAlignment(Qt::AlignCenter);
            tableWdg.setItem(rc, 0, twi);
            twi = new QTableWidgetItem("n/a");
            twi->setTextAlignment(Qt::AlignCenter);
            tableWdg.setItem(rc, 1, twi);
            twi = new QTableWidgetItem("n/a");
            twi->setTextAlignment(Qt::AlignCenter);
            tableWdg.setItem(rc, 2, twi);
            twi = new QTableWidgetItem("n/a");
            twi->setTextAlignment(Qt::AlignCenter);
            tableWdg.setItem(rc, 3, twi);
            tableWdg.resizeColumnsToContents();
        }
    }

    //remove row
    int rc = tableWdg.rowCount();
    for(int ri=0; ri<rc; ri++){
        QTableWidgetItem *twi = tableWdg.item(ri, 0);
        QString n = twi->text();
        if(sm.contains(n) == false){
            tableWdg.removeRow(ri);
            rc--;
        }
    }
    //update data

    for(int ri=0; ri<tableWdg.rowCount(); ri++){
        QString n = tableWdg.item(ri, 0)->text();
        TSenderInfo &si = sm[n];
        //tableWdg.item(ri, 1)->setText(QString::number(si.lastHbaRecvdTime));
        tableWdg.item(ri, 1)->setText(QString::number(si.fps,'f', 1));
        tableWdg.item(ri, 2)->setText(si.resolution);

    }
    tableWdg.resizeColumnsToContents();

    /*
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
*/

    updateUptime();

    //update WD
    if(unity->clientsMap.isEmpty() == true){
        ui->label_wd_noclients->show();
        ui->label_wd_noclients_2->show();
        ui->lineEdit_wdNoClientsTimer->show();
        ui->lineEdit_wdNoClientsTimer->setText(QString::number(unity->wdNoClientsTime.elapsed()/1000));
    }
    else{
        ui->label_wd_noclients->hide();
        ui->label_wd_noclients_2->hide();
        ui->lineEdit_wdNoClientsTimer->hide();
    }

    //qDebug("time=%d", startTime.elapsed());
}

void Dialog::handleUpdateUiHardwareParams()
{
    QTime startTime;
    startTime.start();
    float cpuTemp, gpuTemp;
    //GetGpuTemperature(&gpuTemp);
    //qDebug("temp=%f, temp=%f, time=%d", cpuTemp, gpuTemp, startTime.elapsed());
    if(GetTemperature(&cpuTemp, &gpuTemp) == true){
        ui->lineEditCpuTemp->setText(QString::number(cpuTemp) + " °C");
        ui->lineEditGpuTemp->setText(QString::number(gpuTemp) + " °C");
    }
    else{
        ui->lineEditCpuTemp->setText("n/a");
        ui->lineEditGpuTemp->setText("n/a");
    }
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
    if(com->isWritable() == true){
        if(cbWriteParamsCount > 0){
            cbWriteParamsCount--;

//            if(ui->checkBoxAudioEnableOnStartup->isChecked() == true)
//                com->setAudioEnable(true);
        }
        else{
            //writeCbParamsTimer->stop();
        }
    }
}

void Dialog::updateWindowState(uint32_t style)
{
    ui->checkBox_WS_MINIMIZE->setEnabled(true);
    ui->checkBox_WS_CAPTION->setEnabled(true);
    ui->checkBox_WS_MAXIMIZE->setEnabled(true);
    ui->checkBox_WS_POPUP->setEnabled(true);
    ui->checkBox_WS_DLGFRAME->setEnabled(true);
    ui->checkBox_WS_BORDER->setEnabled(true);
    //ui->checkBox_BuildOnTop->setEnabled(true);

    ui->checkBox_WS_MINIMIZE->setChecked((style&WS_MINIMIZE)!=0);
    ui->checkBox_WS_CAPTION->setChecked((style&WS_CAPTION)!=0);
    ui->checkBox_WS_MAXIMIZE->setChecked((style&WS_MAXIMIZE)!=0);
    ui->checkBox_WS_POPUP->setChecked((style&WS_POPUP)!=0);
    ui->checkBox_WS_DLGFRAME->setChecked((style&WS_DLGFRAME)!=0);
    ui->checkBox_WS_BORDER->setChecked((style&WS_BORDER)!=0);

    //ui->checkBox_BuildOnTop->setChecked(wfp == GetForegroundWindow());

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


void Dialog::handleComNewPosData(uint16_t xPos1, uint16_t xPos2, int dist)
{    
    //ui->lineEditEnc1->setText(QString::number(enc1Val));
    //ui->lineEditEnc2->setText(QString::number(enc2Val));
    //ui->lineEditTerm1->setText(QString::number(dist));

    /*enc1Val = xPos1;
    enc2Val = xPos2;
    distVal = dist;*/
    qDebug("np> %d %d %d", xPos1, xPos2, dist);
    sendPosData();
}

//void Dialog::on_pushButton_ComDemoStart_clicked()//
//{
//    if(ui->pushButton_ComDemoStart->text() == "Start demo"){
//        ui->pushButton_ComDemoStart->setText("Stop demo");
//        com->startDemo();
//    }
//    else if(ui->pushButton_ComDemoStart->text() == "Stop demo"){
//        ui->pushButton_ComDemoStart->setText("Start demo");
//        com->stopDemo();

//    }
//}

void Dialog::on_checkBox_demoEna_clicked(bool checked)
{
//    if(checked){
//        disconnect(SIGNAL(Com::newPosData));
//        connect(demo, SIGNAL(newPosData(uint16_t,uint16_t,int)), SLOT(handleComNewPosData(uint16_t,uint16_t,int)));
//    }
//    else{
//        disconnect(SIGNAL(Demo::newPosData));
//        connect(com, SIGNAL(newPosData(uint16_t,uint16_t,int)), SLOT(handleComNewPosData(uint16_t,uint16_t,int)));

//    }
    settings.setValue("demoEnable", checked);

    ui->lineEdit_ComDemoState->setEnabled(checked);
    ui->lineEdit_ComDemoTime->setEnabled(checked);
    ui->lineEdit_ComDemoModeCycleCounter->setEnabled(checked);
    //ui->pushButton_ComDemoStart->setEnabled(checked);
    //com->enableDemo(checked);
    demo->enableDemo(checked);
}

void Dialog::on_pushButton_ComStartIsp_clicked()
{    
    QString fwPath = ui->lineEdit_fwPath->text();
    if(fwPath.isEmpty() == true){
        appendLogString(QString("no flash hex file selected"));
        return;
    }

    int ret = QMessageBox::warning(this, QApplication::applicationName(),
                                   tr("Начать прошивку контроллера кроссплаты?\n"
                                      "Внимание! Неудачная прошивка потребует физический доступ к кроссплате для перепрошивки"),
                                   QMessageBox::No | QMessageBox::Yes,
                                   QMessageBox::No);

    if(ret == QMessageBox::Yes){        
        appendLogString(QString("start flash hex file \"")+fwPath+ "\"");
        com->startIsp(fwPath);
    }

}
