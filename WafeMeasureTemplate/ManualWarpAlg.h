#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include <algorithm>  // std::max_element
#include <cmath>
#include <stdexcept>




struct DataPoint {
	double x;
	double y;
	double a; // 上表面距离
	double b; // 下表面距离
	double t; // 厚度
	double zm; // 中位面位置
    int x_en;
    int y_en;
};

struct WarpResult {
    double BOW;
	double WARP;
    double CENTER_THK;
    double AVERAGE_THK;
	double TTV;
	double SORI;
};

enum MeasureMode 
{
	NormalMeasure, ACQNormalGravity, ACQOppsiteGravity
};
std::vector<DataPoint> readCSV(const std::string& filename);
extern "C" __declspec(dllexport) WarpResult ManualWarpAlg(const std::vector<DataPoint>& data, double center_x, double center_y, double D, double z_gravity);
extern "C" __declspec(dllexport) double calculateZGravity(const std::vector<DataPoint>& normalData, const std::vector<DataPoint>& invertData);



