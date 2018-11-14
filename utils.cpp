#include "utils.h"
#include <QDate>
#include <QProcess>
#include <QFile>
#include <QDebug>
#include <QObject>
#include <QDir>

QString todayLogName()
{
    return QDate::currentDate().toString("yyyy-MM-dd")+".txt";
}

QString todayLogPath()
{
    QString pathStr = "";//ui->lineEditLogPath->text();
    pathStr += "/" + todayLogName();
    return pathStr;
}


bool zip(QString filename, QString zip_filename)
{
    if(QFile("zip.exe").exists() == false){
        //emit msg("zip.exe not found");
        qDebug("zip.exe not found");
        return false;
    }
//    qDebug() << "compress " << qPrintable(filename) << " to "
//             << qPrintable(zip_filename);
    QProcess *zipProc = new QProcess();
    //zipProc->setWorkingDirectory(fileOutPath);
    QStringList pars;
    pars.append("-9");
    pars.append("-j");
    pars.append(zip_filename);
    pars.append(filename);
    zipProc->start("zip.exe", pars);
    zipProc->waitForStarted(-1);
    zipProc->waitForFinished(-1);
//    connect(zipProc,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
//        [=](int exitCode, QProcess::ExitStatus exitStatus){
//            qDebug("zipProc finished");
//        });
    return true;
}

void unZip(QString zip_filename , QString outPath)
{
    QProcess *unZipProc = new QProcess();
    unZipProc->setWorkingDirectory(outPath);
    unZipProc->start("unzip", QStringList(zip_filename));
    unZipProc->waitForStarted(-1);
    //unZipProc->waitForFinished(-1);
//    connect(unZipProc,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
//        [=](int exitCode, QProcess::ExitStatus exitStatus){
//            qDebug("unZipProc finished");
//        });
}
