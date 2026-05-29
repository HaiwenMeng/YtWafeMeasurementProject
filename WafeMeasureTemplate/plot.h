//
// Copyright 2023 SOPT
//
#ifndef PLOT_HEADER_H
#define PLOT_HEADER_H

#include <QTimer>
#include <QWidget>

#include "qcustomplot.h"
/**
 * @brief This is a plot display UI widget, which can display realtime data.
 **/
class CustomPlotWidget : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Dedfault constructor.
   * @param[in] parent Parent class.
   **/
  explicit CustomPlotWidget(QWidget *parent = nullptr);

  /**
   * @brief Update plot.
   * @param[in] value Current value.
   * @param[in] duration X axis duration.
   **/
  void UpdatePlot(double value, double duration);

  /**
   * @brief Clear plot.
   **/
  void Clear();

 private:
  // Custom plot object.
  QCustomPlot *custom_plot_{nullptr};
  // current x.
  double current_x_{0.0};

  int nDataCount = 0;
};

#endif  // ! PLOT_HEADER_H