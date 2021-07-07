#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/dl_dxf.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
}


void MainWindow::mainProcess(){
    start_widget_ = new startWidget(this);
    this->show();
    QMessageBox::StandardButton start_message = QMessageBox::information(this,"麦展浩:","点击屏幕开始");
}


MainWindow::~MainWindow()
{
    delete ui;
}

