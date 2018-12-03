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

//#define _WIN32_DCOM
#include <guiddef.h>
#include <wbemprov.h>
//#include <iostream>
//using namespace std;
//#include <comdef.h>

#include <Wbemidl.h>


//#pragma comment(lib, "wbemuuid.lib")

//#define EXTERN_C extern "C" GUID CLSID_WbemAdministrativeLocator;
//DEFINE_GUID(CLSID_WbemAdministrativeLocator, 0xcb8555cc, 0x9128, 0x11d1, 0xad,0x9b, 0x00,0xc0,0x4f,0xd8,0xfd,0xff);


HRESULT GetCpuTemperature(uint32_t* pTemperature)
{
        if (pTemperature == NULL)
                return E_INVALIDARG;

        *pTemperature = -1;
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
            return 1;                    // Program has failed.
        }
        if (SUCCEEDED(hr))
        {
                IWbemLocator *pLocator;
                hr = CoCreateInstance(CLSID_WbemLocator /*CLSID_WbemAdministrativeLocator*/,
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
                    return 1;                 // Program has failed.
                }
                if (SUCCEEDED(hr))
                {
                        IWbemServices *pServices;
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
                            return 1;                 // Program has failed.
                        }

                        pLocator->Release();
                        SysFreeString(ns);
                        if (SUCCEEDED(hr))
                        {
                            //wmic /namespace:\\Root\OpenHardwareMonitor path sensor where "identifier='/intelcpu/0/temperature/0'" get value | findstr "[0-9]"
                                //BSTR query = SysAllocString(L"SELECT * FROM sensor where \"identifier=\'//intelcpu//0//temperature//0\'\"");

                            BSTR query = SysAllocString(L"SELECT * FROM sensor WHERE Name LIKE \"GPU Core\" AND SensorType LIKE \"Temperature\" ");
                                BSTR wql = SysAllocString(L"WQL");
                                IEnumWbemClassObject *pEnum;
                                hr = pServices->ExecQuery(wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
                                SysFreeString(wql);
                                SysFreeString(query);
                                pServices->Release();
                                if (SUCCEEDED(hr))
                                {
                                        IWbemClassObject *pObject;
                                        ULONG returned;
                                        hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &returned);
                                        pEnum->Release();
                                        if (SUCCEEDED(hr))
                                        {
                                                BSTR temp = SysAllocString(L"Value");
                                                VARIANT v;
                                                VariantInit(&v);
                                                hr = pObject->Get(temp, 0, &v, NULL, NULL);
//                                                SAFEARRAY* sfArray;
//                                                IWbemClassObject *pclsObj;
//                                                LONG lstart, lend;
//                                                VARIANT vtProp;
//                                                hr = pObject->GetNames(0,WBEM_FLAG_ALWAYS,0,&sfArray);
//                                                pObject->Release();
//                                                hr = SafeArrayGetLBound( sfArray, 1, &lstart );
//                                                if(FAILED(hr)) return hr;
//                                                hr = SafeArrayGetUBound( sfArray, 1, &lend );
//                                                if(FAILED(hr)) return hr;
//                                                BSTR* pbstr;
//                                                hr = SafeArrayAccessData(sfArray,(void HUGEP**)&pbstr);
//                                                int nIdx =0;
//                                                if(SUCCEEDED(hr))
//                                                {
//                                                    CIMTYPE pType;
//                                                    for(nIdx=lstart; nIdx < lend; nIdx++)
//                                                    {

//                                                        hr = pclsObj->Get(pbstr[nIdx], 0, &vtProp, &pType, 0);
//                                                        if (vtProp.vt== VT_NULL)
//                                                        {
//                                                            continue;
//                                                        }
//                                                        if (pType == CIM_STRING && pType != CIM_EMPTY && pType!= CIM_ILLEGAL  )
//                                                        {
//                                                            qDebug() << " OS Name : "<< nIdx << vtProp.bstrVal <<endl;
//                                                        }

//                                                        VariantClear(&vtProp);

//                                                    }
//                                                    hr = SafeArrayUnaccessData(sfArray);
//                                                    if(FAILED(hr)) return hr;
//                                                }

                                                SysFreeString(temp);
                                                if (SUCCEEDED(hr))
                                                {
                                                        *pTemperature = V_I4(&v);
                                                }
                                                VariantClear(&v);
                                        }
                                }
                        }
                        if (ci == S_OK)
                        {
                                CoUninitialize();
                        }
                }
        }
        return hr;
}

