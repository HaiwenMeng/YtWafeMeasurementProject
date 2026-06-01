QT += core gui widgets network xml concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = CalibrationTestDemo
CONFIG += c++17
QMAKE_CXXFLAGS += /bigobj

ROOT_DIR = $$clean_path($$PWD/..)
SDK_DIR = $$clean_path($$ROOT_DIR/BaseLib/SDK_V3_30_x64)
DEFINES += CCS_SDK_V330

INCLUDEPATH += \
    $$PWD \
    $$ROOT_DIR/MotionCtrlDemo/src \
    $$ROOT_DIR/MotionCtrlDemo/src/comm \
    $$ROOT_DIR/MotionCtrlDemo/src/motion \
    $$ROOT_DIR/colorFocusContrlDemo \
    $$ROOT_DIR/AlgoDemo/src \
    $$ROOT_DIR/AlgoDemo/src/algo \
    $$ROOT_DIR/AlgoDemo/third_party/eigen3 \
    $$SDK_DIR

LIBS += -L$$SDK_DIR -lDLL_CCS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    xml_config_manager.cpp \
    $$ROOT_DIR/MotionCtrlDemo/src/comm/TcpCommunicator.cpp \
    $$ROOT_DIR/MotionCtrlDemo/src/motion/MotionController.cpp \
    $$ROOT_DIR/colorFocusContrlDemo/colorFocus_control.cpp \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferAlgorithm.cpp \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferCsvLoader.cpp

HEADERS += \
    mainwindow.h \
    paramsettings.h \
    xml_config_manager.h \
    $$ROOT_DIR/MotionCtrlDemo/src/comm/TcpCommunicator.h \
    $$ROOT_DIR/MotionCtrlDemo/src/motion/MotionController.h \
    $$ROOT_DIR/MotionCtrlDemo/src/motion/MotionTypes.h \
    $$ROOT_DIR/colorFocusContrlDemo/colorFocus_control.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferAlgorithm.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferAlgorithmResult.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferCsvLoader.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferTypes.h

FORMS += \
    mainwindow.ui

DESTDIR = ../BIN

win32 {
    SDK_DLL = $$shell_path($$SDK_DIR/DLL_CCS.dll)
    OUT_DLL = $$shell_path($$DESTDIR/DLL_CCS.dll)
    QMAKE_POST_LINK += $$quote(cmd /c copy /Y "$$SDK_DLL" "$$OUT_DLL")
}
