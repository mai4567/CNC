#include "Widget.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <mainwindow.h>
#include <QTimer>
#include <string.h>

void Delay_MSec(unsigned int msec)
{
    QEventLoop loop;
    QTimer::singleShot(msec, &loop, SLOT(quit()));
    loop.exec();
}


QString fileSelect(QWidget *parent){
    QString path = QFileDialog::getOpenFileName(parent,"choose your file",".","Text Files(*.dxf)",0,QFileDialog::DontUseNativeDialog);  //get files
    if (path.isEmpty()){
        QMessageBox::StandardButton file_chosen = QMessageBox::critical(NULL,"error","No files selected");
    }
    return path;
}


void setBotton(QPushButton *botton,QString text,int x,int y,int w,int h,QString font_size = "28px;"){
    botton->setGeometry(x,y,w,h);
    botton->setText(text);
    botton->setStyleSheet("QPushButton{font-size:"+font_size+"background-color:rgba(238,233,233,0);}"
                          "QPushButton:hover{background-color:rgba(190,180,180);}");
    botton->setFlat(false);
}


void setLabel(QLabel *label,QString text,int x,int y,int w,int h,QString font_size = "16px;"){
    label->setText(text);
    label->setGeometry(x,y,w,h);
    label->setStyleSheet("QLabel{font-size:"+font_size+"background:rgba(204, 0, 0,1);border:5px solid rgba(238, 59, 59,1);"
                                                       "color:rgba(255, 153, 0,1)}");
}


void setLineEdit(QLineEdit *line_edit,int x,int y,int w,int h){
    line_edit->setGeometry(x,y,w,h);
    line_edit->setStyleSheet("background:rgba(0, 255, 102,1);font-size:14px;color:rgba(0, 0, 204,1);border:5px solid rgba(0, 255, 255,1)");
    line_edit->setPlaceholderText("刀补半径(mm):");
    line_edit->setMaxLength(3);
}


void workWidget::setBaseButton(QWidget *mainwindow){
    int width = mainwindow->width();
    int height = mainwindow->height();

    img_ = new QPushButton(mainwindow);
    left_ = new QPushButton(mainwindow);
    right_ = new QPushButton(mainwindow);
    quit_ = new QPushButton(mainwindow);
    output_ = new QPushButton(mainwindow);

    img_->setDisabled(true);
    left_->setDisabled(true);
    right_->setDisabled(true);

    QString botton_text[5] = {"原图路径","左刀补路径","右刀补路径","退出程序","程序输出"};
    setBotton(img_,botton_text[0],int(3*width/4),int(2*height/7),int(width/4),int(height/7));
    setBotton(left_,botton_text[1],int(3*width/4),int(3*height/7),int(width/4),int(height/7));
    setBotton(right_,botton_text[2],int(3*width/4),int(4*height/7),int(width/4),int(height/7));
    setBotton(quit_,botton_text[3],int(3*width/4),int(6*height/7),int(width/4),int(height/7));
    setBotton(output_,botton_text[4],int(3*width/4),int(5*height/7),int(width/4),int(height/7));


    connect(quit_,&QPushButton::clicked,this,[=](){emit startWidget::pushQuitButton();});
    connect(img_,&QPushButton::clicked,this,[=](){emit workWidget::pushImgButton();});
    connect(left_,&QPushButton::clicked,this,[=](){emit workWidget::pushLeftButton();});
    connect(right_,&QPushButton::clicked,this,[=](){emit workWidget::pushRightButton();});
    connect(output_,&QPushButton::clicked,this,[=](){emit workWidget::pushOutPutButton();});

}


void workWidget::pushUploadButton(){
    if (paint_widget_!=NULL){
       paint_widget_->close();
    }
    QMessageBox::StandardButton tip = QMessageBox::information(NULL,"tip","File should under the size of 1189*841(A0)");
    reader_ = new dxfReader;
    dxf_ = new DL_Dxf;
    path_ = fileSelect(this);
    if (previous_path_ == "NULL"){
        previous_path_ = path_;
    }
    else {
        if (previous_path_ != path_){
            img_->setDisabled(true);
            left_->setDisabled(true);
            right_->setDisabled(true);
            previous_path_ = path_;
        }
    }
    if (!path_.isEmpty()){
        this->now_path_->setText(path_);
        this->now_path_->setStyleSheet("font-size:10px;background:rgba(0, 255, 102,1);color:rgba(0, 0, 204,1);"
                                       "border:5px solid rgba(0, 255, 255,1)");
        this->download_->setEnabled(true);
        this->img_->setEnabled(true);

        dxf_->in(path_.toStdString(), reader_);
//        reader_->reorderLine();
        reader_->messageOutput();

        paint_widget_ = new paintWidget(this);
        paint_widget_->show();
    }
    else {
        img_->setDisabled(true);
        left_->setDisabled(true);
        right_->setDisabled(true);
        output_->setDisabled(true);
    }
}


void workWidget::pushDownloadButton(){
    path_ = "当前路径:NULL";
    this->now_path_->setText(path_);
    this->now_path_->setStyleSheet("QLabel{font-size:16px;background:rgba(204, 0, 0,1);border:5px solid rgba(238, 59, 59,1);"
                                                                 "color:rgba(255, 153, 0,1)}");
    this->download_->setDisabled(true);
    this->img_->setDisabled(true);
    this->left_->setDisabled(true);
    this->right_->setDisabled(true);
    this->output_->setDisabled(true);

    paint_widget_->close();
    delete dxf_;
    delete reader_;
    if (insertion_ != NULL){
        delete insertion_;
        insertion_ = NULL;
    }
}


void workWidget::pushSureRadiusButton(){
    radius_ = this->now_radius_->text().toInt();
    if (radius_ == 0){
        QMessageBox::StandardButton radius_warn = QMessageBox::information(this,"warning","the radius is 0");
    }
    else {
        QMessageBox::StandardButton radius_set = QMessageBox::information(this,"message","the radius is set to "+QString::number(radius_)+"mm now");
    }
}


void workWidget::pushImgButton(){
    if (paint_widget_ != NULL){
        paint_widget_->close();
    }
    insertion_ = new Insertion(reader_);
    insertion_->all_points_ = insertion_->lineInsert(insertion_->lines_,insertion_->all_points_);
    insertion_->all_points_ = insertion_->circleInsert(insertion_->arcs_,insertion_->all_points_);
    insert_points_ = insertion_->getPoints();
    std::vector<std::vector<int>> fms = insertion_->getFm();
//    for (int i=0;i<int(insert_points_.size());i++){
//        for (int j=0;j<int(insert_points_[i].size());j++){
//            std::cout<<"x"+std::to_string(j)+":"<<insert_points_[i][j].x()<<",y"+std::to_string(j)<<
//                       ":"<<insert_points_[i][j].y()<<",fm:"<<std::to_string(fms[i][j])<<std::endl;
//        }
//        std::cout<<std::endl;
//    }
    paint_widget_ = new paintPath(this);
    paint_widget_->show();
    left_->setEnabled(true);
    right_->setEnabled(true);
}


void workWidget::pushLeftButton(){
    left_status_ = 1;
    if (paint_widget_ != NULL){
        paint_widget_->close();
    }
    insertion_ = new Insertion(reader_);
    offset_lines_.clear();
    offset_arcs_.clear();
    cutterOffset c = cutterOffset(radius_,left_status_,reader_);
    std::vector<std::vector<contour>> path = c.reorderPattern();
    c.fixOffset(path);

    offset_arcs_ = c.getOffsetArcs();
    offset_lines_ = c.getOffsetLines();
    insertion_->all_offset_points_ = insertion_->lineInsert(offset_lines_,insertion_->all_offset_points_);
    insertion_->all_offset_points_ = insertion_->circleInsert(offset_arcs_,insertion_->all_offset_points_);
    insert_offset_points_ = insertion_->getOffsetPoints();
    paint_widget_ = new paintOffset(this);
    paint_widget_->show();
}


void workWidget::pushRightButton(){
    left_status_ = -1;
    if (paint_widget_ != NULL){
        paint_widget_->close();
    }
    insertion_ = new Insertion(reader_);
    offset_lines_.clear();
    offset_arcs_.clear();
    cutterOffset c = cutterOffset(radius_,left_status_,reader_);
    std::vector<std::vector<contour>> path = c.reorderPattern();
    c.fixOffset(path);

    offset_arcs_ = c.getOffsetArcs();
    offset_lines_ = c.getOffsetLines();
    insertion_->all_offset_points_ = insertion_->lineInsert(offset_lines_,insertion_->all_offset_points_);
    insertion_->all_offset_points_ = insertion_->circleInsert(offset_arcs_,insertion_->all_offset_points_);
    insert_offset_points_ = insertion_->getOffsetPoints();
    paint_widget_ = new paintOffset(this);
    paint_widget_->show();
}


void workWidget::pushOutPutButton(){
    cutterOffset c = cutterOffset(radius_,1,reader_);
    std::vector<std::vector<contour>> path = c.reorderPattern();
    std::vector<std::vector<contour>> path_ok;
    for (auto &pattern:path){
        std::vector<contour> contour_ok;
        for (auto &contour:pattern){
            if (contour.line.x1 != -1 || contour.arc.cx != -1){
                contour_ok.push_back(contour);
            }
        }
        if (!contour_ok.empty()){
           path_ok.push_back(contour_ok);
        }
    }
    outoutInit();
    QFile file;
    file.setFileName("/home/mai/QtProject/CNC/output.m");
    QTextStream out(&file);
    int step = 30;
    if (file.open(QFile::WriteOnly|QIODevice::Append)) {
        for (auto &pattern:path_ok){
            if (pattern[0].line.x1 != -1){
                out<<"N"+QString::number(step)<<" G00 X"+QString::number(pattern[0].line.x1)<<" Y"+QString::number(pattern[0].line.y1)<<endl;
                step += 10;
                out<<"N"+QString::number(step)<<" Z-5"<<endl;
                step += 10;
            }
            else if (pattern[0].arc.cx != -1){
                DL_PointData start_point = reader_->getArcStartPoint(pattern[0].arc);
                out<<"N"+QString::number(step)<<" G00 X"+QString::number(start_point.x)<<" Y"+QString::number(start_point.y)<<endl;
                step += 10;
                out<<"N"+QString::number(step)<<" Z-5"<<endl;
                step += 10;
            }
            for (auto &way:pattern){
                if (way.line.x1 != -1){
                    DL_PointData start_point(way.line.x1,way.line.y1);
                    DL_PointData end_point(way.line.x2,way.line.y2);
                    out<<"N"+QString::number(step)<<" G01 X"+QString::number(end_point.x)<<" Y"+QString::number(end_point.y)<<endl;
                    step += 10;
                }
                else if (way.arc.cx != -1){
                    DL_PointData start_point = reader_->getArcStartPoint(way.arc);
                    DL_PointData end_point = reader_->getArcEndPoint(way.arc);
                    QString circleway;
                    if (way.arc.angle1 > way.arc.angle2){
                        circleway = " G03";
                    }
                    else if (way.arc.angle2 >= way.arc.angle1){
                        circleway = " G02";
                    }
                    out<<"N"+QString::number(step)<<circleway+" X"+QString::number(end_point.x)<<" Y"+QString::number(end_point.y)
                       <<" I"+QString::number(way.arc.cx-start_point.x)<<" J"+QString::number(way.arc.cy-start_point.y)
                       <<" R"+QString::number(way.arc.radius)<<endl;
                    step += 10;
                }
            }
            out<<"N"+QString::number(step)<<" Z+5"<<endl;
            step += 10;
        }
        out<<"N"+QString::number(step)<<" Z+15"<<endl;
        step += 10;
        out<<"N"+QString::number(step)<<" G00 X0 Y0"<<endl;
        step += 10;
        out<<"N"+QString::number(step)<<" M002"<<endl;
    }
    QMessageBox::StandardButton output_message = QMessageBox::information(this,"message","the program have been saved in path:/home/mai/QtProject/CNC/output.m");
}


void workWidget::outoutInit(){
    QFile file;
    file.setFileName("/home/mai/QtProject/CNC/output.m");
    if (file.open(QFile::WriteOnly)) {
        QTextStream out(&file);
        out<<"%2000"<<"\n"<<"N10 G92 X0 Y0"<<endl;
        if (left_status_ == 1){
           out<<"N20 G90 G17 G00 G41 D01 X10 Y10"<<endl;
        }
        else if (left_status_ == -1){
           out<<"N20 G90 G17 G00 G42 D01 X10 Y10"<<endl;
        }
        else {
            out<<"N20 G90 G17 G00 X10 Y10"<<endl;
        }
        out<<"N30 Z-15 S400 M03"<<endl;
        file.close();
    }
}


void workWidget::setCNCButton(QWidget *mainwindow){
    int width = mainwindow->width();
    int height = mainwindow->height();
    upload_ = new QPushButton(mainwindow);
    download_ = new QPushButton(mainwindow);
    sure_radius_ = new QPushButton(mainwindow);

    QString botton_text[4] = {"选择文件","卸载文件","确认刀补半径"};
    setBotton(upload_,botton_text[0],int(3*width/4),0,int(width/4),int(height/14),"20px;");
    setBotton(download_,botton_text[1],int(3*width/4),int(2*height/14),int(width/4),int(height/14),"20px;");
    setBotton(sure_radius_,botton_text[2],int(3*width/4),int(3*height/14),int(width/8),int(height/14),"20px;");

    download_->setDisabled(true);

    connect(upload_,&QPushButton::clicked,this,[=](){emit pushUploadButton();});
    connect(download_,&QPushButton::clicked,this,[=](){emit pushDownloadButton();});
    connect(sure_radius_,&QPushButton::clicked,this,[=](){emit pushSureRadiusButton();});
}


void workWidget::setAllLabel(QWidget *mainwindow){
    int width = mainwindow->width();
    int height = mainwindow->height();
    now_radius_ = new QLineEdit(mainwindow);
    now_path_ = new QLabel(mainwindow);
    QString label_text[1] = {"当前路径:"};
    setLabel(now_path_,label_text[0]+path_,int(3*width/4),int(height/14),int(width/4),int(height/14));
    setLineEdit(now_radius_,int(7*width/8),int(3*height/14),int(width/8),int(height/14));
}


startWidget::startWidget(QWidget *mainwindow){
    int width = mainwindow->width();
    int height = mainwindow->height();

    Label = new QLabel(mainwindow);
    movie = new QMovie("./start5.gif");
    movie->setSpeed(130);
    start = new QPushButton(mainwindow);
    Label->resize(width,height);
    Label->setMovie(movie);
    movie->setScaledSize(QSize(width,height));
    movie->start();
    start->setFixedSize(width,height);
    start->adjustSize();
    start->setText("麦展浩     机电4班     3118000551\n\n数控技术课程设计\n\nclick to start");
    start->setFlat(true);
    start->setStyleSheet("text-align: top;""padding-top: 10px;""font-size:28px; ");

    connect(start,&QPushButton::clicked,this,[=](){emit pushButtonClicked(mainwindow);});
}


void startWidget::pushButtonClicked(QWidget *mainwindow){
    mainwindow->close();
    load_widget_ = new loadWidget(mainwindow);
    work_widget_ = new workWidget(mainwindow);
}


void startWidget::pushQuitButton(){
    load_widget_->close();
    work_widget_->close();
    this->close();
    delete load_widget_;
    delete work_widget_;
}


startWidget::~startWidget(){
    delete start;
    delete Label;
    delete movie;
}


loadWidget::loadWidget(QWidget *mainwindow) : startWidget(mainwindow){
    int width = mainwindow->width();
    int height = mainwindow->height();
    Label = new QLabel(mainwindow);
    movie = new QMovie("./speed.gif");
    movie->setScaledSize(QSize(width,height));
    movie->setSpeed(70);
    Label->resize(width,height);
    Label->setMovie(movie);
    movie->start();
    mainwindow->show();
    Delay_MSec(2900);
    movie->stop();
    mainwindow->close();
}


loadWidget::~loadWidget(){
    delete Label;
    delete movie;
}


workWidget::workWidget(QWidget *mainwindow) : startWidget(mainwindow){
    int width = 1080;
    int height = 640;
    Label = new QLabel(mainwindow);
    movie = new QMovie("./start1.gif");
    Label->setAutoFillBackground(true);
    Label->resize(width,height);
    Label->setMovie(movie);
    movie->setScaledSize(QSize(int(3*width/4),height));
    movie->start();

    setBaseButton(mainwindow);
    setCNCButton(mainwindow);
    setAllLabel(mainwindow);

    mainwindow->show();
}


workWidget::~workWidget(){
    delete this;
}


paintWidget::paintWidget(workWidget *parent) : QMainWindow(parent){
    parent_ = parent;
    width_ = 1189;
    height_ = 841;
    lines_ = parent->reader_->lines_;
    circles_ = parent->reader_->circles_;
    points_ = parent->reader_->points_;
    arcs_ = parent->reader_->arcs_;
    draw_arcs_ = parent_->reader_->draw_arcs_;

    this->setPalette(QPalette(Qt::white));
    this->setAutoFillBackground(true);
    this->resize(width_,height_);
    this->setWindowTitle("图纸查看器");
}

paintWidget::~paintWidget(){
    delete painter_;
}

void paintWidget::adjustScale(){
    int x_max=0;
    int y_max=0;
    int x_min = 10000;
    int y_min = 10000;
    for (auto &line:lines_){
        int line_x_max = std::max(line.x1,line.x2);
        int line_y_max = std::max(line.y1,line.y2);
        int line_x_min = std::min(line.x1,line.x2);
        int line_y_min = std::min(line.y1,line.y2);
        if (line_x_max > x_max){
            x_max = line_x_max;
        }
        if (line_x_min < x_min){
            x_min = line_x_min;
        }
        if (line_y_max > y_max){
            y_max = line_y_max;
        }
        if (line_y_min < y_min){
            y_min = line_y_min;
        }
    }
    for (auto &arc:arcs_){
        int arc_x_max = arc.cx + arc.radius;
        int arc_x_min = arc.cx - arc.radius;
        int arc_y_max = arc.cy + arc.radius;
        int arc_y_min = arc.cy - arc.radius;
        if (arc_x_max > x_max){
            x_max = arc_x_max;
        }
        if (arc_x_min < x_min){
            x_min = arc_x_min;
        }
        if (arc_y_max > y_max){
            y_max = arc_y_max;
        }
        if (arc_y_min < y_min){
            y_min = arc_y_min;
        }
    }
    int len_x = x_max - x_min;
    int len_y = y_max - y_min;
    if (len_x <= width_ && len_y <= height_){
        scale_ = std::min((width_/x_max),(height_/y_max));
        if (scale_ > 3){
            scale_ = scale_ - 3;
        }
        else if (scale_ == 3){
            scale_ = 2;
        }
    }
//    if ((len_x > width_ || len_y >height_) || (x_max > width_ || y_max > height_)) {
//        QMessageBox::StandardButton error = QMessageBox::critical(NULL,"error","file is over size");
//        parent_->img_->setDisabled(true);
//        parent_->left_->setDisabled(true);
//        parent_->right_->setDisabled(true);
//        this->close();
//    }
//    std::cout<<"x:"<<x_max<<",y:"<<y_max<<" scale_:"<<scale_<<std::endl;
}

void paintWidget::paintEvent(QPaintEvent *event){
    QPen pen;
    pen.setWidth(1);  //宽度
    pen.setColor(Qt::red);  //划线颜色
    pen.setStyle(Qt::SolidLine);  //线的样式，实线、虚线等
    pen.setCapStyle(Qt::FlatCap);   //线端点的样式
    pen.setJoinStyle(Qt::BevelJoin);

    painter_ = new QPainter(this);
    adjustScale();
    painter_->setWindow(src_x_,src_y_,width_,height_);
    painter_->setPen(pen);
    for (auto & line:lines_){
        painter_->drawLine(int(line.x1*scale_),int(line.y1*scale_),int(line.x2*scale_),int(line.y2*scale_));
    }
    for (auto & arc:arcs_){
        int aleft = (arc.cx - arc.radius)*scale_;
        int atop= (arc.cy - arc.radius)*scale_;
        if (arc.angle1 > arc.angle2){
           painter_->drawArc(QRect(aleft,atop,2*arc.radius*scale_,2*arc.radius*scale_),(360-arc.angle1)*16,-1*(arc.angle2-arc.angle1+360)*16);
        }
        else {
           painter_->drawArc(QRect(aleft,atop,2*arc.radius*scale_,2*arc.radius*scale_),-1*arc.angle1*16,-1*(arc.angle2-arc.angle1)*16);
        }
    }
    painter_->drawText(width_-50,height_-10,"1:"+QString::number(scale_));
    delete painter_;
}

paintPath::paintPath(workWidget *parent) : paintWidget(parent){
    parent_ = parent;
    width_ = 1189;
    height_ = 841;

    this->setPalette(QPalette(Qt::white));
    this->setAutoFillBackground(true);
    this->resize(width_,height_);
    this->setWindowTitle("原图路径");
}


void paintPath::paintEvent(QPaintEvent *event){
    QPen pen;
    pen.setWidth(1);  //宽度
    pen.setColor(Qt::red);  //划线颜色
    pen.setStyle(Qt::SolidLine);  //线的样式，实线、虚线等
    pen.setCapStyle(Qt::FlatCap);   //线端点的样式
    pen.setJoinStyle(Qt::BevelJoin);

    painter_ = new QPainter(this);
    painter_->setPen(pen);
    for (auto &points:parent_->insert_points_){
        for (int i=0;i<int(points.size()-1);i++){
            painter_->drawLine(points[i],points[i+1]);
        }
    }
    delete painter_;
}


paintOffset::paintOffset(workWidget *parent) : paintWidget(parent){
    parent_ = parent;
    width_ = 1189;
    height_ = 841;

    this->setPalette(QPalette(Qt::white));
    this->setAutoFillBackground(true);
    this->resize(width_,height_);
    this->setWindowTitle("左刀补路径");
}

void paintOffset::paintEvent(QPaintEvent *event){
    QPen pen;
    pen.setWidth(1);  //宽度
    pen.setColor(Qt::red);  //划线颜色
    pen.setStyle(Qt::SolidLine);  //线的样式，实线、虚线等
    pen.setCapStyle(Qt::FlatCap);   //线端点的样式
    pen.setJoinStyle(Qt::BevelJoin);

    painter_ = new QPainter(this);
    painter_->setPen(pen);
    for (auto &points:parent_->insert_points_){
        for (int i=0;i<int(points.size()-1);i++){
            painter_->drawLine(points[i],points[i+1]);
        }
    }
    painter_->setPen(QPen(Qt::blue,1));
    for (auto &points:parent_->insert_offset_points_){
        for (int i=0;i<int(points.size()-1);i++){
            painter_->drawLine(points[i],points[i+1]);
        }
    }
    parent_->offset_lines_.clear();
    delete painter_;
}
