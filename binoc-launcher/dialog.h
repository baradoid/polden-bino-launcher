#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QUdpSocket>
#include <QNetworkDatagram>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;
    QUdpSocket *udpSocket;


private slots:
    void handleReadPendingDatagrams();
};

#endif // DIALOG_H
