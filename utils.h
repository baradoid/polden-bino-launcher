#pragma once
#include <QString>

QString todayLogName();
QString todayLogPath();

bool zip(QString filename , QString zip_filename);
void unZip(QString zip_filename , QString filename);
