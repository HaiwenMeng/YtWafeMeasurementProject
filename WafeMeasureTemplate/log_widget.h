//
// Copyright 2023 SOPT
//
#ifndef LOG_WIDGET_H
#define LOG_WIDGET_H

#include <QVBoxLayout>
#include <QWidget>
#include <QFile>
#include <QTextStream>

QT_BEGIN_NAMESPACE
namespace Ui {
class LogWidget;
}
QT_END_NAMESPACE


/**
 * @brief This is a class for PM Calibration widget UI, which include PM
 * indentation depth calibration module, PM verticality calibration module,
 * PM-OM Calibration module and PM motion error calibration module.
 **/
class LogWidget : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Default construct function, this will build the PM Calibration UI
   * in the mainwindow.
   * @param[in] parent Father class.
   **/
  LogWidget(QWidget *parent = nullptr);

  /**
   * @brief Destructor function.
   **/
  ~LogWidget();

  void OpenLogFile();


  private:
    Ui::LogWidget *ui{nullptr};

public slots:
    void wirteLog(QString logStr);
    // void on_pushButton_open_log_clicked();

  private:
    QString m_currentDate;
    QFile *m_pLogFile = nullptr;
    QTextStream *m_pLogStream = nullptr;
};


#endif  // PM_LOG_WIDGET_H
