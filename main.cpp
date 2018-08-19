#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    w.setWindowFlags(w.windowFlags() & ~Qt::WindowContextHelpButtonHint );
    w.show();

    return a.exec();
}
