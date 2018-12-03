#pragma once
#include <QString>

QString todayLogName();
QString todayLogPath();

bool zip(QString filename , QString zip_filename);
void unZip(QString zip_filename , QString filename);

typedef struct{
    QString rootDir;
    QString downloadDir;
    QString logsDir;
} TDirStruct;


#include <windows.h>
//#define _WIN32_DCOM
//#include <iostream>
//using namespace std;
//#include <comdef.h>
//#include <Wbemidl.h>

//#ifdef 1
//    #define _WIN32_DCOM
//    #include <wbemidl.h>
//    #include <comdef.h>
//    #include <conio.h>
//    #pragma comment(lib, "wbemuuid.lib")
//#endif

HRESULT GetCpuTemperature(LPLONG pTemperature);
