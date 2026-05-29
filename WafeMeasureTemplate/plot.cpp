//
// Copyright 2023 SOPT
//
#include "plot.h"
CustomPlotWidget::CustomPlotWidget(QWidget *parent) : QWidget(parent) {
  // Create the QCustomPlot widget
  custom_plot_ = new QCustomPlot(this);
  custom_plot_->setBackground(Qt::white);

  // Create a graph for the line plot
  custom_plot_->addGraph();
  custom_plot_->graph(0)->setPen(QPen(Qt::red));

  // Set the x-axis range and label
  custom_plot_->xAxis->setRange(0, 10);

  // Set the y-axis label and initial range
  custom_plot_->yAxis->setRange(-5, 5);

  custom_plot_->yAxis->ticker()->setTickCount(3);  // 只显示 3 个主要刻度
  custom_plot_->yAxis->ticker()->setTickStepStrategy(QCPAxisTicker::tssMeetTickCount);

  // Layout setup
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(custom_plot_);
  this->setLayout(layout);
}

void CustomPlotWidget::UpdatePlot(double value, double duration) {
  // Generate new data points for the plot
  current_x_ += 0.1;

  // Add data to the graph
  custom_plot_->graph(0)->addData(current_x_, value);

  nDataCount = custom_plot_->graph(0)->dataCount();
  if(nDataCount > 120)
  {
    custom_plot_->graph(0)->data()->removeBefore(nDataCount - 120);
  }
  
  // Update the y-axis range based on the new data
  custom_plot_->yAxis->setRange(value - 5, value + 5);

  custom_plot_->graph(0)->rescaleAxes();

  // Move the x-axis range to show a moving window of data
  custom_plot_->xAxis->setRange(current_x_, 10, Qt::AlignRight);

  // Replot the graph
  custom_plot_->replot();
}

void CustomPlotWidget::Clear() {
  custom_plot_->graph(0)->data()->clear();
  custom_plot_->replot();
}