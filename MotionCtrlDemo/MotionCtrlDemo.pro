QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TEMPLATE = app
TARGET = MotionCtrlDemo

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    src/comm/TcpCommunicator.cpp \
    src/motion/MotionController.cpp \
    src/ui/MainWindow.cpp

HEADERS += \
    src/comm/TcpCommunicator.h \
    src/motion/MotionController.h \
    src/motion/MotionTypes.h \
    src/ui/MainWindow.h

DESTDIR = ../BIN

