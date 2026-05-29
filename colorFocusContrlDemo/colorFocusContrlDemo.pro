QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SDK_DIR = $$clean_path($$PWD/../BaseLib/SDK_V3_30_x64)
INCLUDEPATH += $$SDK_DIR
LIBS += -L$$SDK_DIR -lDLL_CCS

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    colorFocus_control.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    colorFocus_control.h \
    widget.h

FORMS += \
    widget.ui

DESTDIR = ../BIN

win32 {
    SDK_DLL = $$shell_path($$SDK_DIR/DLL_CCS.dll)
    OUT_DLL = $$shell_path($$DESTDIR/DLL_CCS.dll)
    QMAKE_POST_LINK += $$quote(cmd /c copy /Y "$$SDK_DLL" "$$OUT_DLL")
}
