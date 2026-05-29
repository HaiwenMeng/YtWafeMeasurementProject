#include "dialogtemperaturecontroldevice.h"
#include "ui_dialogtemperaturecontroldevice.h"

DialogTemperatureControlDevice::DialogTemperatureControlDevice(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogTemperatureControlDevice)
{
    ui->setupUi(this);
    this->InitSerial();
}

DialogTemperatureControlDevice::~DialogTemperatureControlDevice()
{
    if (serialCommunicator)
        delete serialCommunicator;
    delete ui;
}

void DialogTemperatureControlDevice::on_comboBox_port_currentIndexChanged(int index)
{

}


void DialogTemperatureControlDevice::on_comboBox_baudRate_currentIndexChanged(int index)
{

}


void DialogTemperatureControlDevice::on_pushButton_openOrCloseSerial_clicked()
{
    if (!IsOpen) {
        IsOpen=serialCommunicator->openPort(ui->comboBox_port->currentText(), 9600, QSerialPort::Data8,
            QSerialPort::NoParity,
            QSerialPort::OneStop);
    }
    else
    {
        serialCommunicator->closePort();
        IsOpen = false;
    }
    ui->pushButton_startOrStopScan->setEnabled(IsOpen);
    ui->comboBox_port->setEnabled(!IsOpen);
}


void DialogTemperatureControlDevice::on_pushButton_startOrStopScan_clicked()
{
    serialCommunicator->sendCommand(GET_TEMPER_COMMAND);
    ui->label_Temp->setText(QString::number(GetTemperature(),'f', 1));
    ui->label_RH->setText(QString::number(GetHumidity(), 'f', 1));
}
void DialogTemperatureControlDevice::InitSerial()
{
    serialCommunicator = new SerialCommunicator(this);
    ui->pushButton_startOrStopScan->setEnabled(false);
    auto ports = QSerialPortInfo::availablePorts();
    for (int i = 0; i < ports.size(); i++)
    {
        ui->comboBox_port->addItem(ports[i].portName());
        if (ports[i].portName().contains("COM4"))
            PortNameIndex = i;
    }
    if (PortNameIndex< ports.size())
    {
        ui->comboBox_port->setCurrentIndex(PortNameIndex);
        on_pushButton_openOrCloseSerial_clicked();
    }
   
}

double  DialogTemperatureControlDevice::GetTemperature() 
{
    return serialCommunicator->Temperature;
}
double  DialogTemperatureControlDevice::GetHumidity() 
{
    return serialCommunicator->Humidity;

}
