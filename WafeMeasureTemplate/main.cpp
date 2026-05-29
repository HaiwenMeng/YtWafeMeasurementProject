#include <QApplication>
#include "PermissionManager/databasemanager.h"
#include "PermissionManager/dialoglogin.h"
#include "mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
  
//    if (!DatabaseManager::instance().initializeDatabase()) {
//        return 1;
//    }

    MainWindow w;
    w.show();
    return a.exec();

//    DialogLogin loginDlg;
//    if (loginDlg.exec() == QDialog::Accepted) {
//        MainWindow w;
//        w.show();
//        return a.exec();
//    }

//    return 0;
}
