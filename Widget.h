#ifndef WIDGET_H
#define WIDGET_H

#include <QMainWindow>
#include<iostream>
#include <QPushButton>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QMovie>
#include <QLineEdit>
#include <QPainter>
#include <cnc_code.h>
#include <QTextStream>

namespace Ui { class startWidget; }


class loadWidget;
class workWidget;
class paintWidget;


class startWidget : public QWidget{
public:
    startWidget(QWidget *mainwindow);
    ~startWidget();
    void loadProcess(QWidget *mainwindow);
private:
    Ui::startWidget *ui;
    QPushButton *start;
    QLabel *Label;
    QMovie *movie;
    loadWidget *load_widget_;
    workWidget *work_widget_;
public slots:
    void pushQuitButton();
private slots:
    void pushButtonClicked(QWidget *mianwindow);
};


class workWidget : public startWidget{
public:
    workWidget(QWidget *mainwindow);
    ~workWidget();
    int left_status_ = 0;
    int radius_ = 0;
    int files_status_ = 0;
    QPushButton *img_;
    QPushButton *left_;
    QPushButton *right_;
    dxfReader *reader_;
    DL_Dxf *dxf_;
    QString path_ = "NULL";
    QString previous_path_ = "NULL";
    std::vector<DL_LineData> offset_lines_;
    std::vector<DL_ArcData> offset_arcs_;
    std::vector<std::vector<QPoint>> insert_points_;
    std::vector<std::vector<QPoint>> insert_offset_points_;
    void mainProcess(void);
private:
    QLabel *Label;
    QMovie *movie;
    QPushButton *quit_;
    QPushButton *upload_;
    QPushButton *download_;
    QPushButton *sure_radius_;
    QPushButton *output_;
    QLabel *now_path_;
    QLineEdit *now_radius_;

    paintWidget *paint_widget_ = NULL;
    Insertion *insertion_ = NULL;

    void setBaseButton(QWidget *mainwindow);
    void setCNCButton(QWidget *mainwindow);
    void setAllLabel(QWidget *mainwindow);
    void outoutInit();
private slots:
    void pushUploadButton();
    void pushDownloadButton();
    void pushSureRadiusButton();
    void pushImgButton();
    void pushLeftButton();
    void pushRightButton();
    void pushOutPutButton();
};


class loadWidget : public startWidget{
public:
    loadWidget(QWidget *mainwindow);
    ~loadWidget();
private:
    QLabel *Label;
    QMovie *movie;
};


class paintWidget : public QMainWindow{
public:
    int width_,height_;
    workWidget *parent_;
    QPainter *painter_;
    paintWidget(workWidget *parent);
    ~paintWidget();
    virtual void paintEvent(QPaintEvent *event);
private:
    int src_x_ = 0;
    int src_y_ = 0;
    int scale_ = 1;
    std::vector<DL_LineData> lines_{};
    std::vector<DL_ArcData> arcs_{};
    std::vector<DL_ArcData> draw_arcs_{};
    std::vector<DL_PointData> points_{};
    std::vector<DL_CircleData> circles_{};
    void adjustScale();
};

class paintPath : public paintWidget{
public:
    paintPath(workWidget *parent);
    void paintEvent(QPaintEvent *event) override;
};

class paintOffset : public paintWidget{
public:
    paintOffset(workWidget *parent);
    void paintEvent(QPaintEvent *event) override;
};

#endif // WIDGET_H
