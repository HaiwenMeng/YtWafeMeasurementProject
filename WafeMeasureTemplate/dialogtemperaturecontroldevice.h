#ifndef DIALOGTEMPERATURECONTROLDEVICE_H
#define DIALOGTEMPERATURECONTROLDEVICE_H

#include "qdialog.h"

#include "serial_communicator.h"
#include <QSerialPortInfo>

namespace Ui {
class DialogTemperatureControlDevice;
}

class DialogTemperatureControlDevice : public QDialog
{
    Q_OBJECT

public:
    explicit DialogTemperatureControlDevice(QWidget *parent = nullptr);
    ~DialogTemperatureControlDevice();
    void InitSerial();
    double GetTemperature();
    double GetHumidity();

private slots:
    void on_comboBox_port_currentIndexChanged(int index);

    void on_comboBox_baudRate_currentIndexChanged(int index);

    void on_pushButton_openOrCloseSerial_clicked();

    void on_pushButton_startOrStopScan_clicked();

private:
    Ui::DialogTemperatureControlDevice *ui;
    SerialCommunicator *serialCommunicator;
    bool IsOpen = false;
    int PortNameIndex = 1;
};

#endif // DIALOGTEMPERATURECONTROLDEVICE_H
