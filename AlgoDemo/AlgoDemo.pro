QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = AlgoDemo
CONFIG += c++17
QMAKE_CXXFLAGS += /bigobj

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/algo \
    $$PWD/src/ui \
    $$PWD/third_party/eigen3

SOURCES += \
    src/main.cpp \
    src/ui/MainWindow.cpp \
    src/algo/WaferAlgorithm.cpp \
    src/algo/WaferCsvLoader.cpp

HEADERS += \
    src/ui/MainWindow.h \
    src/algo/WaferTypes.h \
    src/algo/WaferAlgorithm.h \
    src/algo/WaferAlgorithmResult.h \
    src/algo/WaferCsvLoader.h

FORMS += \
    src/ui/MainWindow.ui

DESTDIR = ../BIN

win32:CONFIG(release, debug|release): OBJECTS_DIR = build/release/obj
win32:CONFIG(release, debug|release): MOC_DIR = build/release/moc
win32:CONFIG(release, debug|release): UI_DIR = build/release/ui
win32:CONFIG(debug, debug|release): OBJECTS_DIR = build/debug/obj
win32:CONFIG(debug, debug|release): MOC_DIR = build/debug/moc
win32:CONFIG(debug, debug|release): UI_DIR = build/debug/ui
