#include "mainwindow.h"
#include "Widget.h"
#include "cnc_code.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow mainwindow ;// = new QMainWindow;
    mainwindow.resize(1080,640);
    mainwindow.setFixedSize(mainwindow.width(), mainwindow.height());
    mainwindow.mainProcess();



//    dxfReader f;
//    DL_Dxf dxf;
//    //读取.dxf文件
//    if (!dxf.in("c_line_c.dxf", &f)) {
//     std::cerr << "drawing.dxf could not be opened.\n";
//    }

    return a.exec();
}
