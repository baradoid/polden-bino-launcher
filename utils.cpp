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

#include <wbemprov.h>
IWbemServices *pServices;
bool securityStart()
{
    HRESULT ci = CoInitialize(NULL);
    HRESULT hr = CoInitializeSecurity(NULL,
                                      -1,
                                      NULL,
                                      NULL,
                                      RPC_C_AUTHN_LEVEL_DEFAULT,
                                      RPC_C_IMP_LEVEL_IMPERSONATE,
                                      NULL,
                                      EOAC_NONE,
                                      NULL);
    if (FAILED(hr))
    {
//            std::cout << "Failed to initialize security. Error code = 0x"
//                << hex << hres << endl;
        CoUninitialize();
        return false;                    // Program has failed.
    }
    IWbemLocator *pLocator;
    hr = CoCreateInstance(CLSID_WbemLocator,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID*)&pLocator);
    if (FAILED(hr))
    {
        //                    cout << "Failed to create IWbemLocator object."
        //                        << " Err code = 0x"
        //                        << hex << hres << endl;
        CoUninitialize();
        return false;                 // Program has failed.
    }

    BSTR ns = SysAllocString(L"root\\OpenHardwareMonitor");
    hr = pLocator->ConnectServer(ns,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pServices);
    if (FAILED(hr))
    {
        //                            cout << "Failed to create IWbemLocator object."
        //                                << " Err code = 0x"
        //                                << hex << hres << endl;
        CoUninitialize();
        return false;                 // Program has failed.
    }

    pLocator->Release();
    SysFreeString(ns);

    return true;
}

bool GetTemperature(float* pCpuTemp, float* pGpuTemp)
{
//    QTime startTime;
//    startTime.start();
//    qDebug("getTemp st");

    bool ret = false;
    *pCpuTemp = *pGpuTemp = -1;
    try{
        HRESULT hr, ci;

        //QString qstrQuery = QString("SELECT Value FROM sensor WHERE Name LIKE \"") + key + QString("\" AND SensorType LIKE \"Temperature\" ");
        QString qstrQuery = QString("SELECT * FROM sensor WHERE (Name LIKE \"CPU Core #1\" OR Name LIKE \"GPU Core\") AND SensorType LIKE \"Temperature\" ");
        BSTR query = SysAllocString(qstrQuery.toStdWString().c_str());
        BSTR wql = SysAllocString(L"WQL");
        IEnumWbemClassObject *pEnum;
        hr = pServices->ExecQuery(wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
        SysFreeString(wql);
        SysFreeString(query);
        //pServices->Release();
        //qDebug("getTemp 3 time=%d", startTime.elapsed());
        while(hr == WBEM_S_NO_ERROR )
        {
            IWbemClassObject *pObject;
            ULONG returned;
            hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &returned);
            //qDebug("getTemp 4 time=%d", startTime.elapsed());
            if (hr == WBEM_S_NO_ERROR )
            {
                VARIANT v;
                VariantInit(&v);
                BSTR partName = SysAllocString(L"Name");
                hr = pObject->Get(partName, 0, &v, NULL, NULL);
                SysFreeString(partName);
                QString ss;
                if (SUCCEEDED(hr)){
                    ss = QString::fromWCharArray((wchar_t*)v.bstrVal);
                    //qDebug() <<"name" << qPrintable(ss);
                }
                BSTR temp = SysAllocString(L"Value");
                hr = pObject->Get(temp, 0, &v, NULL, NULL);
                SysFreeString(temp);
                if (SUCCEEDED(hr))
                {
                    if(ss == "CPU Core #1"){
                        *pCpuTemp = V_R4(&v);
                        ret = true;
                    }
                    if(ss == "GPU Core"){
                        *pGpuTemp = V_R4(&v);
                        ret = true;
                    }
                }
                VariantClear(&v);
            }
        }
        pEnum->Release();
    }
    catch(...){

    }

    //qDebug("getTemp 5 time=%d", startTime.elapsed());
    return ret;
}


//HRESULT GetCpuTemperature(float* pTemperature)
//{
//    return GetTemperature("CPU Core #1", pTemperature);
//}


//HRESULT GetGpuTemperature(float* pTemperature)
//{
//    return GetTemperature("GPU Core", pTemperature);
//}
