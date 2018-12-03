#include "dialog.h"
#include <QApplication>

HRESULT GetCpuTemperature(uint32_t* pTemperature);

int main(int argc, char *argv[])
{
    uint32_t temp;
    GetCpuTemperature(&temp);
    qDebug("temp=%x\n", temp);
    GetCpuTemperature(&temp);
    qDebug("temp=%x\n", temp);
    GetCpuTemperature(&temp);
    qDebug("temp=%x\n", temp);

    QApplication a(argc, argv);
    Dialog w;
    w.setWindowFlags(w.windowFlags() & ~Qt::WindowContextHelpButtonHint );
    w.show();

    return a.exec();
}
