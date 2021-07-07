#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Widget.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void mainProcess(void);
private:
    Ui::MainWindow *ui;
    startWidget *start_widget_;
    loadWidget *load_widget_;
};
#endif // MAINWINDOW_H
