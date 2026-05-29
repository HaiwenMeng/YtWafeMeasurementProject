//
// Copyright 2023 SOPT
//
#include "log_widget.h"
#include "ui_log_widget.h"

#include <QMessageBox>
#include <QDateTime>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <qclipboard.h>
#include <QScrollBar>



LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::LogWidget) {
  ui->setupUi(this);

  m_currentDate = QDate::currentDate().toString("yyyy-MM-dd");
}

LogWidget::~LogWidget()
{
    if (ui)
    {
        delete ui;
        ui = nullptr;
    }
}

void LogWidget::OpenLogFile()
 {
    //创建文件夹
    QDir appDir = QCoreApplication::applicationDirPath();
    QString filePath = appDir.filePath("Log");
    QDir dir(filePath);
    if (!dir.exists()) 
    {
        dir.mkpath(filePath); // 创建文件夹，包括任何必要的父目录
    }

    m_currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString currentDateLogFileName = filePath + "/" + m_currentDate + ".txt";
    m_pLogFile = new QFile(currentDateLogFileName);
    bool success = m_pLogFile->open(QIODevice::WriteOnly | QIODevice::Append);
    if (!success) {
        // Handle error
        return;
    }
    m_pLogStream = new QTextStream(m_pLogFile);
}

void LogWidget::wirteLog(QString logStr)
{
    if (m_currentDate != QDate::currentDate().toString("yyyy-MM-dd"))
    {
        m_pLogFile->close();

        delete m_pLogStream;
        delete m_pLogFile;
        m_pLogStream = nullptr;
        m_pLogFile = nullptr;

        ui->plainTextEdit_log->clear();

        OpenLogFile();
    }
    
    QString tmpCurrentDateTimeLogInfo = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz ") + logStr + "\n";

    ui->plainTextEdit_log->appendPlainText(tmpCurrentDateTimeLogInfo);

    QScrollBar *scrollBar = ui->plainTextEdit_log->verticalScrollBar();
    if (scrollBar)
    {
        scrollBar->setSliderPosition(scrollBar->maximum());
    }

    if (m_pLogFile == nullptr || !m_pLogFile->isOpen())
    {
        OpenLogFile();
    }

    *m_pLogStream << tmpCurrentDateTimeLogInfo;
    m_pLogStream->flush();
}

// void LogWidget::on_pushButton_open_log_clicked()
// {
//     QString currentFileName = QFileDialog::getOpenFileName(this, u8"选择日志文件", "", "All Files (*.txt)");
//     if (currentFileName.isEmpty())
//     {
//         return;
//     }

//     QFile file(currentFileName);
//     if (file.open(QIODevice::ReadOnly))
//     {
//         QTextStream stream(&file);
//         ui->plainTextEdit_log->clear();
//         ui->plainTextEdit_log->appendPlainText(stream.readAll());
//     }
//     file.close();
// }
