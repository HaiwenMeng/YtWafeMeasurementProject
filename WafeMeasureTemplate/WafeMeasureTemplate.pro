QT += core gui widgets serialport network xml sql datavisualization concurrent printsupport

TEMPLATE = app
TARGET = WafeMeasureTemplate
CONFIG += c++17
QMAKE_CXXFLAGS += /bigobj

ROOT_DIR = $$clean_path($$PWD/..)
SDK_DIR = $$clean_path($$ROOT_DIR/BaseLib/SDK_V3_30_x64)
#SDK_DIR = $$clean_path($$ROOT_DIR/BaseLib/SDK_V3_40_x64)


DEFINES += ENCODER_TRIGGER
DEFINES += ORGANIZE_STANDARD

INCLUDEPATH += \
    $$PWD \
    $$PWD/PermissionManager \
    $$PWD/QXlsx/header \
    $$ROOT_DIR/MotionCtrlDemo/src \
    $$ROOT_DIR/MotionCtrlDemo/src/comm \
    $$ROOT_DIR/MotionCtrlDemo/src/motion \
    $$ROOT_DIR/colorFocusContrlDemo \
    $$ROOT_DIR/AlgoDemo/src \
    $$ROOT_DIR/AlgoDemo/src/algo \
    $$ROOT_DIR/AlgoDemo/third_party/eigen3 \
    $$ROOT_DIR/CalibrationTestDemo \
    $$SDK_DIR

LIBS += -L$$SDK_DIR -lDLL_CCS
CONFIG(debug, debug|release) {
    LIBS += -L$$PWD/QXlsx/lib/DebugLib -lQXlsxQt5
} else {
    LIBS += -L$$PWD/QXlsx/lib/ReleaseLib -lQXlsxQt5
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    measure_control.cpp \
    dialog_setting.cpp \
    dialogmeasureparams.cpp \
    dialogmessagebox.cpp \
    dialogtemperaturecontroldevice.cpp \
    log_widget.cpp \
    wid_control.cpp \
    serial_communicator.cpp \
    tcp_communicator.cpp \
    RecipeSettingDialog.cpp \
    recipedialog.cpp \
    recipecontrol.cpp \
    RecipeDelegate.cpp \
    plot.cpp \
    qcustomplot.cpp \
    chipheatmapviewer.cpp \
    chipsurfacesampleviewer.cpp \
    waitingspinnerwidget.cpp \
    PermissionManager/adduserdialog.cpp \
    PermissionManager/authservice.cpp \
    PermissionManager/currentuser.cpp \
    PermissionManager/databasemanager.cpp \
    PermissionManager/delegate.cpp \
    PermissionManager/dialoglogin.cpp \
    PermissionManager/passwordhasher.cpp \
    PermissionManager/permissionmanager.cpp \
    PermissionManager/usermanagementdialog.cpp \
    $$ROOT_DIR/MotionCtrlDemo/src/comm/TcpCommunicator.cpp \
    $$ROOT_DIR/MotionCtrlDemo/src/motion/MotionController.cpp \
    $$ROOT_DIR/colorFocusContrlDemo/colorFocus_control.cpp \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferAlgorithm.cpp \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferCsvLoader.cpp \
    $$ROOT_DIR/CalibrationTestDemo/xml_config_manager.cpp

HEADERS += \
    mainwindow.h \
    measure_control.h \
    ManualWarpAlg.h \
    data_define.h \
    paramsettings.h \
    motion_control.h \
    colorFocus_control.h \
    dialog_setting.h \
    dialogmeasureparams.h \
    dialogmessagebox.h \
    dialogtemperaturecontroldevice.h \
    log_widget.h \
    wid_control.h \
    serial_communicator.h \
    tcp_communicator.h \
    RecipeSettingDialog.h \
    recipedialog.h \
    recipecontrol.h \
    RecipeDelegate.h \
    plot.h \
    qcustomplot.h \
    chipheatmapviewer.h \
    chipsurfacesampleviewer.h \
    waitingspinnerwidget.h \
    PermissionManager/adduserdialog.h \
    PermissionManager/authservice.h \
    PermissionManager/currentuser.h \
    PermissionManager/databasemanager.h \
    PermissionManager/delegate.h \
    PermissionManager/dialoglogin.h \
    PermissionManager/passwordhasher.h \
    PermissionManager/permissionmanager.h \
    PermissionManager/usermanagementdialog.h \
    $$ROOT_DIR/MotionCtrlDemo/src/comm/TcpCommunicator.h \
    $$ROOT_DIR/MotionCtrlDemo/src/motion/MotionController.h \
    $$ROOT_DIR/MotionCtrlDemo/src/motion/MotionTypes.h \
    $$ROOT_DIR/colorFocusContrlDemo/colorFocus_control.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferAlgorithm.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferAlgorithmResult.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferCsvLoader.h \
    $$ROOT_DIR/AlgoDemo/src/algo/WaferTypes.h \
    $$ROOT_DIR/CalibrationTestDemo/paramsettings.h \
    $$ROOT_DIR/CalibrationTestDemo/xml_config_manager.h

FORMS += \
    mainwindow.ui \
    dialog_setting.ui \
    dialogmeasureparams.ui \
    dialogmessagebox.ui \
    dialogtemperaturecontroldevice.ui \
    log_widget.ui \
    RecipeSettingDialog.ui \
    recipedialog.ui \
    recipecontrol.ui \
    PermissionManager/adduserdialog.ui \
    PermissionManager/dialoglogin.ui \
    PermissionManager/usermanagementdialog.ui

RESOURCES += resource.qrc
RC_FILE += app.rc
DESTDIR = ../BIN

win32 {
    SDK_DLL = $$shell_path($$SDK_DIR/DLL_CCS.dll)
    OUT_SDK_DLL = $$shell_path($$DESTDIR/DLL_CCS.dll)
    ALG_DLL = $$shell_path($$PWD/AlgApi.dll)
    OUT_ALG_DLL = $$shell_path($$DESTDIR/AlgApi.dll)
    QMAKE_POST_LINK = $$quote(copy /Y "$$SDK_DLL" "$$OUT_SDK_DLL") $$escape_expand(\n\t) $$quote(copy /Y "$$ALG_DLL" "$$OUT_ALG_DLL")
}
