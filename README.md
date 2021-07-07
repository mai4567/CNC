# 数控技术：数控系统刀补功能的软件实现及其仿真--数控仿真程序开发实战

主要功能是传入一个.dxf文件，可以读取该文件显示其外形轮廓，然后通过数控技术的逐点比较法对该路径进行插补，还可以对其进行刀补路径的仿真，可以处理圆弧（没有圆因为时间不够）、直线两种图形之间任意组合，传入的.dxf有大小尺寸要求，在AutoCAD上以原点为基准，图像不大于A0尺寸（1189*841），先上几张图片展示一下程序效果。

<img src="https://github.com/mai4567/CNC-markdown-picture/raw/main/%E5%9B%BE%E7%89%87/1.png?raw=true" style="zoom: 25%;" />

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/2.png?raw=true" style="zoom: 25%;" />

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/3.png?raw=true" style="zoom: 25%;" />

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/4.png?raw=true" style="zoom: 25%;" />

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/5.png?raw=true" style="zoom: 25%;" />

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/6.png?raw=true" style="zoom: 25%;" />

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/7.png?raw=true" style="zoom: 25%;" />

## 前言：

本人环境：ubuntu16.07 + Qt5.9.9，本文设计到的内容，主要是C++数控逐点比较法的实现，数控的刀补的实现，以及数控刀补圆弧与直线、圆弧与圆弧、直线与直线间几种转接形式的实现。关于软件设计，只是运用了几个简单的Qt控件（QPushbutton等）实现的，别的文章介绍的比我好，本文不会介绍。本文会有很多相关图片介绍原理，制作不易，但本人字丑，画图也不好看，凑合着看下就好。本人的代码在写完后没有经过很多的测试，可能有些地方会有意想不到的bug，大家只需参考思路，如果发现了有bug也可以与本人联系。完整代码会在文末给出。

## 整体思路：

1、读.dxf文件,直接画出文件外型轮廓

2、根据1中的外形轮廓进行插补计算得到点集，点间直线相连模拟数控插补

3、将1中外型轮廓重新按加工顺序排好得到新的加工顺序。

4、根据3中外型轮廓进行刀具半径偏移得到偏移后的外型轮廓

5、根据偏移后的外型轮廓进行C刀补修正

6、按修正后的轮廓插补

## 读.dxf文件：

本文的程序是基于dxflib这个C++开源库做的二次开发。

源码网址：http://www.ribbonsoft.com/en/dxflib-downloads

使用demo可以参考：https://blog.csdn.net/huanghxyz/article/details/73655608

这个开源库可以很容易的读取.dxf文件，得到文件中外型轮廓的信息（线、圆弧、圆、点等）。可以看出输出的信息是无序的轮廓的信息，这个库貌似没有将这些轮廓排序输出等操作（有可能是我了解不清楚），所以我在这里的工作主要是将读取到的单个轮廓信息，将其组合成一组组轮廓，因为要实现刀补，所以还要出给同组轮廓的加工方向。（但这一步是在刀补的实现上做的）。

这一步只做简单的操作：把外型轮廓保存在容器里面方便后面开发。

```cpp
//cnc_code.h
virtual void addLine(const DL_LineData& d);
virtual void addPoint(const DL_PointData & data);
virtual void addArc(const DL_ArcData& data);
virtual void addCircle(const DL_CircleData & data);
```

```cpp
//cnc_code.cpp
void dxfReader::addLine(const DL_LineData& d) { 
     lines_.push_back(d);
}
void dxfReader::addArc(const DL_ArcData& data){
    arcs_.push_back(data);
    draw_arcs_.push_back(data);
}

void dxfReader::messageOutput(){  //打印轮廓信息
    std::cout<<std::endl;
    if (!lines_.empty()){
        std::cout<<"Line:"<<std::endl;
        for (auto &line:lines_){
           std::cout<< line.x1 << "/" << line.y1<< " " << line.x2 << "/" << line.y2 << std::endl;
        }
    }
    if (!arcs_.empty()){
        std::cout<<"ARC:"<<std::endl;
        for (auto &arc:arcs_){
            if (arc.angle1 > arc.angle2){
                arc.angle1 = arc.angle1 - 360;
            }
            std::cout<<arc.cx<<" , "<<arc.cy<<" begin:"<<arc.angle1<<" end:"<<arc.angle2<<" radius:"<<arc.radius<<std::endl;
        }
    }
}
```

```cpp
//Widget.cpp
reader_ = new dxfReader;
dxf_ = new DL_Dxf;
if (!path_.isEmpty()){  //path_是文件路径
	dxf_->in(path_.toStdString(), reader_);
    reader_->messageOutput();
}
```

这边补充一点信息，方便大家理解。dxflib库里有DL_LineData、DL_ArcData和DL_PointData(本文主要用这三个)，详细的信息可以自己翻一下源码。

```cpp
DL_LineData(double x1,double y1,double z1,double x2,double y2,double z2);
DL_ArcData(double cx,double cy,double cz,double radius,double angle1,double angle2);
DL_PointData(double x,double y,double z);
```

看参数都比较好理解是什么意思，angle1是圆弧开始点的角度，angle2则是结束的角度。

顺便给出求圆弧起点、终点的方法：将原点放到圆弧的圆点，根据角度投影。

```cpp
DL_PointData dxfReader::getArcStartPoint(DL_ArcData arc){  //获得圆弧开始的点
    DL_PointData start_point;
    start_point.z = 0;
    start_point.x = arc.cx + arc.radius*cos(arc.angle1*M_PI/180);
    start_point.y = arc.cy + arc.radius*sin(arc.angle1*M_PI/180);
    return start_point;
}
DL_PointData dxfReader::getArcEndPoint(DL_ArcData arc){  //获得圆弧结束的点
    DL_PointData end_point;
    end_point.z = 0;
    end_point.x = arc.cx + arc.radius*cos(arc.angle2*M_PI/180);
    end_point.y = arc.cy + arc.radius*sin(arc.angle2*M_PI/180);
    return end_point;
}
```

为了给出路径我定义了一个结构体来表示

```cpp
struct contour{
    DL_LineData line = DL_LineData(-1,-1,-1,-1,-1,-1);
    DL_ArcData arc = DL_ArcData(-1,-1,-1,-1,-1,-1);
};
```

通过判断line.x1或arc.cx是否等于-1可以判断该轮廓是线还是圆弧。

## 插补算法（逐点比较法）：

### 直线插补：

先上图，理解一下，要注意的是，我们的图像坐标系和平时的直角坐标系不一样，y轴的方向是反的，所以我们显示的图和原图是反过来的（无伤大雅我就没有管他），我们推公式的时候也要注意这一点，圆弧插补也是一样的。

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/8.jpg?raw=true" style="zoom: 50%;" />

```cpp
lineInserter::lineInserter(DL_LineData line){
    X0 = line.x1;
    Y0 = line.y1;
    Xm_ = 0;
    Ym_ = 0;
    Xe_ = double(line.x2) - double(line.x1);
    Ye_ = double(line.y2) - double(line.y1);
    step_ = std::abs(std::round(line.x2-line.x1)) + std::abs(std::round(line.y2-line.y1));
    Fm_ = 0;
}

void lineInserter::lineInsert(){
    points_.push_back(QPoint((Xm_+X0),(Ym_+Y0)));
    Fms_.push_back(Fm_);
    while (step_ != 0){
        //first quadrant
        if (Xe_>0 && Ye_<0){
            if(Fm_ < 0){
                Xm_ += 1;
                Fm_ -= Ye_;
                step_ -= 1;
            }
            else{
                Ym_ -= 1;
                Fm_ -= Xe_;
                step_ -= 1;
            }
        }
        //second quadrant
        else if(Xe_<0 && Ye_<0){
            if (Fm_ >= 0){
                Xm_ -= 1;
                Fm_ += Ye_;
                step_ -= 1;
            }
            else{
                Ym_ -= 1;
                Fm_ -= Xe_;
                step_ -= 1;
            }
        }
        //third quadrant
        else if (Xe_<0 && Ye_>0){
            if (Fm_ >= 0){
                Ym_ += 1;
                Fm_ += Xe_;
                step_ -= 1;
            }
            else{
                Xm_ -= 1;
                Fm_ += Ye_;
                step_ -= 1;
            }
        }
        //fourth quadrant
        else if (Xe_>0 && Ye_>0){
            if (Fm_ < 0){
                Ym_ += 1;
                Fm_ += Xe_;
                step_ -= 1;
            }
            else{
                Xm_ += 1;
                Fm_ -= Ye_;
                step_ -= 1;
            }
        }
        // +X axis
        else if  (Xe_>0 && Ye_==0){
            Xm_ += 1;
            step_ -= 1;
        }
        // -X axis
        else if (Xe_<0 && Ye_==0){
            Xm_ -= 1;
            step_ -= 1;
        }
        // +Y axis
        else if (Ye_>0 && Xe_==0){
            Ym_ += 1;
            step_ -= 1;
        }
        // -Y axis
        else if (Ye_<0 && Xe_==0){
            Ym_ -= 1;
            step_ -= 1;
        }
        points_.push_back(QPoint(Xm_+X0,Ym_+Y0));
        Fms_.push_back(Fm_);
    }
}
```



### 圆弧插补：

圆弧插补不同的是，你的步数不能直接通过起点和终点坐标的X，Y差值的绝对值之和求出，因为你的圆弧有可能大于90度，可能会因为步数缺少而导致插补提前结束。所以我的做法是把弧等分成>=4份，再分别算每一份的步数相加作为总步数。除此以外，圆弧插补还分方向，顺时针和逆时针的公式不一样。将圆心移到坐标零点计算，一旦方向、起点（就是确定了半径）确定，插补终点就确定下来了。

<img src="https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/9.jpg?raw=true" style="zoom:50%;" />

```cpp
circleInserter::circleInserter(DL_ArcData circle){
    X0 = circle.cx;
    Y0 = circle.cy;
    Xm_ = std::round(circle.radius*cos(double(circle.angle1*M_PI/180)));  //四舍五入是否会更精确
    Ym_ = std::round(circle.radius*sin(double(circle.angle1*M_PI/180)));
    Xe_ = std::round(circle.radius*cos(double(circle.angle2*M_PI/180)));
    Ye_ = std::round(circle.radius*sin(double(circle.angle2*M_PI/180)));
    if (std::abs(circle.angle1 - circle.angle2) <= 90){
        step_ = std::abs(Xe_-Xm_)+std::abs(Ye_-Ym_);
    }
    else {
        double angle = (circle.angle2 - circle.angle1)/4;
        double angle1 = circle.angle1 + angle;
        double angle2 = circle.angle1 + 2*angle;
        double angle3 = circle.angle1 + 3*angle;
        double X1 = circle.radius*cos(double(angle1*M_PI/180));
        double Y1 = circle.radius*sin(double(angle1*M_PI/180));
        double X2 = circle.radius*cos(double(angle2*M_PI/180));
        double Y2 = circle.radius*sin(double(angle2*M_PI/180));
        double X3 = circle.radius*cos(double(angle3*M_PI/180));
        double Y3 = circle.radius*sin(double(angle3*M_PI/180));
        step_ = std::abs(std::round(X1-Xm_))+std::abs(std::round(Y1-Ym_))+std::abs(std::round(X2-X1))
                +std::abs(std::round(Y2-Y1))+std::abs(std::round(X3-X2))+std::abs(std::round(Y3-Y2))
                +std::abs(std::round(Xe_-X3))+std::abs(std::round(Ye_-Y3));
    }
    Fm_ = 0;
    clock_ = (circle.angle2 - circle.angle1)/std::abs(circle.angle2 - circle.angle1);  //判断顺逆时针
}

void circleInserter::circleInsert(){
    points_.push_back(QPoint((Xm_+X0),(Ym_+Y0)));
    Fms_.push_back(Fm_);
    while (step_ != 0){
        //fourth quadrant clockwise
        if ((Xm_>0 && Ym_>=0) && clock_ == 1){
            if (Fm_>=0){
                Fm_ = Fm_-2*Xm_+1;
                Xm_ -= 1;
                step_ -= 1;
            }
            else {
                Fm_ = Fm_+2*Ym_+1;
                Ym_ += 1;
                step_ -= 1;
            }
        }
        //third quadrant clockwise
        else if ((Xm_<=0 && Ym_>0) && clock_ == 1){
            if (Fm_>=0){
                Fm_ = Fm_-2*Ym_+1;
                Ym_ -= 1;
                step_ -= 1;
            }
            else {
                Fm_ = Fm_-2*Xm_+1;
                Xm_ -= 1;
                step_ -= 1;
            }
        }
        //scond quadrant clockwise
        else if ((Xm_<0 && Ym_<=0) && clock_ == 1){
            if (Fm_>=0){
                Fm_ = Fm_+2*Xm_+1;
                Xm_ += 1;
                step_ -= 1;
            }
            else {
                Fm_ = Fm_-2*Ym_-1;
                Ym_ -= 1;
                step_ -= 1;
            }
        }
        //first quadrant clockwise
        else if ((Xm_>=0 && Ym_<0) && clock_ == 1){
            if (Fm_>=0){
                Fm_ = Fm_+2*Ym_+1;
                Ym_ += 1;
                step_ -= 1;
            }
            else {
                Fm_ = Fm_+2*Xm_+1;
                Xm_ += 1;
                step_ -= 1;
            }
        }

        //four quadrant anti-clockwise
        else if ((Xm_>=0 && Ym_>0) && clock_ == -1){
            if (Fm_>=0){
                Fm_ = Fm_-2*Ym_+1;
                Ym_ -= 1;
                step_ -= 1;
            }
            else {
                Fm_ = Fm_+2*Xm_+1;
                Xm_ += 1;
                step_ -= 1;
            }
        }
        //first quadrant anti-clockwise
        else if ((Xm_>0 && Ym_<=0) && clock_==-1){
            if (Fm_ >= 0){
                Fm_ = Fm_-2*Xm_+1;
                Xm_ -= 1;
                step_ -= 1;
            }
            else {
                Fm_ = Fm_-2*Ym_+1;
                Ym_ -= 1;
                step_ -= 1;
            }
        }
        //second quadrant anti-clockwise
        else if ((Xm_<=0 && Ym_<0) && clock_ == -1){
            if (Fm_>=0){
                Fm_ = Fm_+2*Ym_+1;
                Ym_ += 1;
                step_ -= 1;
            }
            else {
                Fm_ = Fm_-2*Xm_+1;
                Xm_ -= 1;
                step_ -= 1;
            }
        }
        //third quadrant anti-clockwise
        else if ((Xm_<0 && Ym_>=0) && clock_ == -1){
            if (Fm_>=0){
               Fm_ = Fm_+2*Xm_+1;
               Xm_ += 1;
               step_ -= 1;
            }
            else {
                Fm_ = Fm_+2*Ym_+1;
                Ym_ += 1;
                step_ -= 1;
            }
        }
        points_.push_back(QPoint((Xm_+X0),(Ym_+Y0)));
        Fms_.push_back(Fm_);
    }
}
```

写到这里，插补的算法已经实现了。

## 外型轮廓重新排序：

基本思路是：

1、先遍历所有图形找到没有与2个或2个以上的图形重合的点，它可以作为开始加工的点，在这些点中我选择了y值最小的点作为开始，但其实可以任意选择，当然若图像为封闭图像，则找不到这样的点，这时可以任意选一个点开始，因为从这点开始沿着一个方向走到最终一定会遍历走完整个封闭图形。

2、在得到开始点之后在与该点组成的同一图形上找到结束的点作为新的开始的点，同时从图形的容器中删去该图形证明这个图形已经被我们重新排序过了，然后遍历所有图形，找到与新的开始点重合的点的图形，其结束的点又作为新的开始点，删掉该图形。重复以上操作直到找不到与新的开始点重合的点，证明相连的 一组图形已经被排序。然后重复1的步骤，找到新的一组图形开始的点。再重复2,直到容器里面不存在任何图形，则全部图形排好序。

最后得到的容器有两层，第一层代表了不相连的图形，第二层是按顺序排好的相连的图形

```cpp
std::vector<std::vector<contour>> cutterOffset::reorderPattern(){  //将线与弧按一定方向重新排序
    std::vector<std::vector<contour>> all_contour_;
    //存放排好序的弧和线
    std::vector<DL_LineData> reorder_lines;
    std::vector<DL_ArcData> reorder_arcs;
    //先找出整个图形的最上方的点
    while (!(reader_->lines_.empty() && reader_->arcs_.empty())){
        std::vector<contour> contours_(reader_->lines_.size()+reader_->arcs_.size());
        DL_PointData start_point(-1,-1,-1);
        //先找出图形中没有重合且位于最上方的的点
        int line_swap = 0;
        int arc_swap = 0;
        int lines_y_min = 10000;
        int arcs_y_min = 10000;
        int line_index = -1;
        int arc_index = -1;
        for (int i=0;i<int(reader_->lines_.size());i++){
            DL_PointData line_p1 = DL_PointData(reader_->lines_[i].x1,reader_->lines_[i].y1);
            DL_PointData line_p2 = DL_PointData(reader_->lines_[i].x2,reader_->lines_[i].y2);
            if ((reader_->judgeStartPoint(line_p1,reader_->lines_,reader_->arcs_,i,-1)==1)&&(line_p1.y < lines_y_min)){
                lines_y_min = line_p1.y;
                line_index = i;
                line_swap = 0;
            }
            else if ((reader_->judgeStartPoint(line_p2,reader_->lines_,reader_->arcs_,i,-1)==1)&&(line_p2.y < lines_y_min)){
                lines_y_min = line_p2.y;
                line_index = i;
                line_swap = 1;
            }
        }
        for (int j=0;j<int(reader_->arcs_.size());j++){
            DL_PointData arc_p1 = reader_->getArcStartPoint(reader_->arcs_[j]);
            DL_PointData arc_p2 = reader_->getArcEndPoint(reader_->arcs_[j]);
            if ((reader_->judgeStartPoint(arc_p1,reader_->lines_,reader_->arcs_,-1,j)==1)&&(arc_p1.y < arcs_y_min)){
                arcs_y_min = arc_p1.y;
                arc_index = j;
                arc_swap = 0;
            }
            else if ((reader_->judgeStartPoint(arc_p2,reader_->lines_,reader_->arcs_,-1,j)==1)&&(arc_p2.y < arcs_y_min)){
                arcs_y_min = arc_p2.y;
                arc_index = j;
                arc_swap = 1;
            }
        }
        if (lines_y_min <= arcs_y_min){
            DL_LineData line =reader_->lines_[line_index];
            if (line_swap == 1){
                line =  reader_->swapLinePoints(line);
            }

            start_point = DL_PointData(line.x2,line.y2,0);

            reorder_lines.push_back(line);
            //保存轮廓
            contour shape;
            shape.line = line;
            contours_.push_back(shape);
            reader_->lines_ = reader_->deleteIndexLine(reader_->lines_,line_index);
        }
        else {
            DL_ArcData arc = reader_->arcs_[arc_index];
            reorder_arcs.push_back(arc);
            if (arc_swap == 0){
                //保存轮廓
                contour shape;
                shape.arc = arc;
                contours_.push_back(shape);

                start_point = reader_->getArcEndPoint(arc);
            }
            else {
                arc = reader_->swapArcPoints(arc);
                //保存轮廓
                contour shape;
                shape.arc = arc;
                contours_.push_back(shape);

                arc = reader_->swapArcPoints(arc);
                start_point = reader_->getArcStartPoint(arc);
            }
            reader_->arcs_ = reader_->deleteIndexArc(reader_->arcs_,arc_index);
        }
        if (start_point.x == -1){  //若图象为封闭图像，则随意找一个点开始遍历
            if (reader_->lines_.size() != 0){
                start_point = DL_PointData(reader_->lines_[0].x1,reader_->lines_[0].y1,0);
            }
            else if(reader_->arcs_.size() != 0){
                start_point = DL_PointData(reader_->arcs_[0].cx,reader_->arcs_[0].cy,0);
            }
        }

        //遍历直线找距离startpoint为0的点
        int line_size = int(reader_->lines_.size());
        for (int i=0;i<line_size;i++){
            double dis11 = std::sqrt(std::pow(reader_->lines_[i].y1-start_point.y,2)+std::pow(reader_->lines_[i].x1-start_point.x,2));
            double dis12 = std::sqrt(std::pow(reader_->lines_[i].y2-start_point.y,2)+std::pow(reader_->lines_[i].x2-start_point.x,2));
            if (dis11 <= 1){
                reorder_lines.push_back(reader_->lines_[i]);
                start_point = DL_PointData(reader_->lines_[i].x2,reader_->lines_[i].y2,0);
                //保存轮廓
                contour shape;
                shape.line = reader_->lines_[i];
                contours_.push_back(shape);

                reader_->lines_ = reader_->deleteIndexLine(reader_->lines_,i);
                i = 0;
                line_size = int(reader_->lines_.size());
            }
            else if (dis12 <= 1){
                DL_LineData line = reader_->swapLinePoints(reader_->lines_[i]);
                reorder_lines.push_back(line);
                //保存轮廓
                contour shape;
                shape.line = line;
                contours_.push_back(shape);

                start_point = DL_PointData(line.x2,line.y2);

                reader_->lines_ = reader_->deleteIndexLine(reader_->lines_,i);
                i = 0;
                line_size = int(reader_->lines_.size());
            }
        }
        //遍历弧找距离startpoint为0的点
        int arc_size = reader_->arcs_.size();
        for (int j=0;j<arc_size;j++){
            DL_PointData p1 = reader_->getArcStartPoint(reader_->arcs_[j]);
            DL_PointData p2 = reader_->getArcEndPoint(reader_->arcs_[j]);
            double dis21 = std::sqrt(std::pow(p1.y-start_point.y,2)+std::pow(p1.x-start_point.x,2));
            double dis22 = std::sqrt(std::pow(p2.y-start_point.y,2)+std::pow(p2.x-start_point.x,2));
            if (dis21 <= 1){
                reorder_arcs.push_back(reader_->arcs_[j]);
                //保存轮廓
                contour shape;
                shape.arc = reader_->arcs_[j];
                contours_.push_back(shape);

                start_point = p2;
                reader_->arcs_ = reader_->deleteIndexArc(reader_->arcs_,j);
                j = 0;
            }
            else if (dis22 <= 1){
                DL_ArcData arc = reader_->swapArcPoints(reader_->arcs_[j]);
                //保存轮廓
                contour shape;
                shape.arc = arc;
                contours_.push_back(shape);

                arc = reader_->swapArcPoints(arc);
                reorder_arcs.push_back(arc);
                start_point = p1;

                reader_->arcs_ = reader_->deleteIndexArc(reader_->arcs_,j);
                j = 0;
            }
        }
        all_contour_.push_back(contours_);
    }
    reader_->lines_ = reorder_lines;
    reader_->arcs_ = reorder_arcs;
    std::cout<<std::endl;
    return all_contour_;
}
```

## 刀具半径偏移：

### 直线：

假如我们知道了刀补是左刀补还是右刀补，也知道了直线的方向和刀补半径，我们就可以直接求出直线基于刀补半径偏移后的直线。但这条直线仅仅是原本直线偏移了一个半径的距离，后续还需要修正。老规矩，先上图帮助大家理解原理。

![](https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/10.jpg?raw=true)

```cpp
DL_LineData cutterOffset::lineOffset(DL_LineData line){
    DL_LineData offset_line(0,0,0,0,0,0);
    if ((line.x1 != line.x2) && (line.y1 != line.y2)){  //if there is slope in this line
        double k = (line.y2 - line.y1)/(line.x2 - line.x1);
        double b = line.y1 - k*line.x1;
        double alpha = atan(k)*180/M_PI;
        double b1 = 0;
        if (line.x2 >= line.x1 && left_status_ == 0){
            b1 = b + radius_ / cos(alpha*M_PI/180);
        }
        else if (line.x2 >= line.x1 && left_status_ == 1){
            b1 = b - radius_ / cos(alpha*M_PI/180);
        }
        else if (line.x2 < line.x1 && left_status_ == 0){
            b1 = b - radius_ / cos(alpha*M_PI/180);
        }
        else if (line.x2 < line.x1 && left_status_ == 1){
            b1 = b + radius_ / cos(alpha*M_PI/180);
        }
        double b2 = (1/k)*line.x1 + line.y1;
        double b3 = (1/k)*line.x2 + line.y2;
        offset_line.x1 = (b2-b1)/(k+(1/k));
        offset_line.y1 = k*offset_line.x1+b1;
        offset_line.x2 = (b3-b1)/(k+(1/k));
        offset_line.y2 = k*offset_line.x2+b1;
    }
    else if (line.x1 == line.x2) {
        if (line.y2 > line.y1 && left_status_ == 1){
            offset_line.x1 = line.x1 + radius_;
            offset_line.x2 = line.x2 + radius_;
        }
        else if (line.y2 < line.y1 && left_status_ == 1){
            offset_line.x1 = line.x1 - radius_;
            offset_line.x2 = line.x2 - radius_;
        }
        else if (line.y2 > line.y1 && left_status_ == 0){
            offset_line.x1 = line.x1 - radius_;
            offset_line.x2 = line.x2 - radius_;
        }
        else if (line.y2 < line.y1 && left_status_ == 0){
            offset_line.x1 = line.x1 + radius_;
            offset_line.x2 = line.x2 + radius_;
        }
        offset_line.y1 = line.y1;
        offset_line.y2 = line.y2;
    }
    else if (line.y1 == line.y2) {
        if (line.x2 > line.x1 && left_status_ == 1){
            offset_line.y1 = line.y1 - radius_;
            offset_line.y2 = line.y2 - radius_;
        }
        else if (line.x2 < line.x1 && left_status_ == 1){
            offset_line.y1 = line.y1 + radius_;
            offset_line.y2 = line.y2 + radius_;
        }
        else if (line.x2 > line.x1 && left_status_ == 0){
            offset_line.y1 = line.y1 + radius_;
            offset_line.y2 = line.y2 + radius_;
        }
        else if (line.x2 < line.x1 && left_status_ == 0){
            offset_line.y1 = line.y1 - radius_;
            offset_line.y2 = line.y2 - radius_;
        }
        offset_line.x1 = line.x1;
        offset_line.x2 = line.x2;
    }
    return offset_line;
}
```

### 圆弧：

圆弧的半径偏移就相对比较简单，直接改半径即可。应该比较容易理解，我就不作图了。

```cpp
DL_ArcData cutterOffset::arcOffset(DL_ArcData arc){
    DL_ArcData offset_arc = DL_ArcData(0,0,0,0,0,0);
    if ((arc.angle2 - arc.angle1 < 0) && left_status_ == 1){
        offset_arc = DL_ArcData(arc.cx,arc.cy,arc.cz,arc.radius - radius_,arc.angle1,arc.angle2);
    }
    else if ((arc.angle2 - arc.angle1 < 0) && left_status_ == 0){
        offset_arc = DL_ArcData(arc.cx,arc.cy,arc.cz,arc.radius + radius_,arc.angle1,arc.angle2);
    }
    else if ((arc.angle2 - arc.angle1 >= 0) && left_status_ == 1){
        offset_arc = DL_ArcData(arc.cx,arc.cy,arc.cz,arc.radius + radius_,arc.angle1,arc.angle2);
    }
    else if ((arc.angle2 - arc.angle1 >= 0) && left_status_ == 0){
        offset_arc = DL_ArcData(arc.cx,arc.cy,arc.cz,arc.radius - radius_,arc.angle1,arc.angle2);
    }
    return offset_arc;
}
```

在做完这些之后，我们应该遍历之前得到的路径，对组成路径的每个图形都做对应的半径偏移，在不改变原来路径的结构的情况下，得到新的路径的容器。

```cpp
contour cutterOffset::contourOffset(contour path){
    contour offset;
    if (path.line.x1 != -1){
        DL_LineData offset_line = lineOffset(path.line);
        offset.line = offset_line;
        offset.arc = DL_ArcData(-1,-1,-1,-1,-1,-1);
    }
    else if (path.arc.cx != -1){
        DL_ArcData offset_arc = arcOffset(path.arc);
        offset.arc = offset_arc;
        offset.line = DL_LineData(-1,-1,-1,-1,-1,-1);
    }
    return offset;
}

void cutterOffset::fixOffset(std::vector<std::vector<contour> > path){
    //存放未修补刀补路径
    std::vector<std::vector<contour>> offsets(reader_->lines_.size()+reader_->arcs_.size());
    offsets.clear();
    for (auto &contours : path){
        std::vector<contour> offsetpath;
        for (auto &tour:contours){
            offsetpath.push_back(contourOffset(tour));
        }
        offsets.push_back(offsetpath);
    }
    。。。。。。。。。。。。下面的与该功能无关
}
```

## C刀补修正：

C刀补修正的一次对两段进行处理，先预处理本段，然后根据下一段的方向来确定刀具中心轨迹的段间过渡状态。

### 矢量夹角计算：

指两编程轨迹在交点处非加工侧的夹角，这个夹角会直接决定刀补的过渡方式是缩短型、伸长型还是插入型，算对矢量夹角非常重要。

#### 直线与直线：

实际上，圆弧与直线、直线与圆弧、圆弧与圆弧的矢量夹角的算法都可以转化为直线与直线之间矢量夹角的计算。

我们在设计算法的时候要传入两条头尾相接(沿着固定一个方向)的直线，然后通过简单的余弦定理可以直接求出两直线的夹角，但这个角还不是矢量夹角。我们通过求第一条直线的偏移直线得到新的直线1’，通过直线2的终点坐标和直线1的起点坐标作差，求出向量direction，通过直线1’的起点（终点）坐标和直线1的起点（终点）坐标作差，求出向量change。通过两个向量的夹角判断矢量夹角是否需要用360-两直线夹角。

![](https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/11.png?raw=true)

```cpp
int cutterOffset::countVectorAngle(DL_LineData line1, DL_LineData line2){  //all lines are one way segment
    double c = std::sqrt(double(std::pow(line2.y2 - line1.y1,2)) + double(std::pow(line2.x2 - line1.x1,2)));
    double a = std::sqrt(double(std::pow(line1.y2 - line1.y1,2)) + double(std::pow(line1.x2 - line1.x1,2)));
    double b = std::sqrt(double(std::pow(line2.y2 - line2.y1,2)) + double(std::pow(line2.x2 - line2.x1,2)));
    double cosa = (pow(c,2)-pow(a,2)-pow(b,2))/(-2*a*b);
    int angle = acos(cosa)*180/M_PI;
    DL_LineData offset_line1 = lineOffset(line1);
    int x_direction = line2.x2 - line1.x1;
    int y_direction = line2.y2 - line1.y1;
    int x_change = offset_line1.x1 - line1.x1;
    int y_change = offset_line1.y1 - line1.y1;
    double cosb = (x_direction*x_change)+(y_direction*y_change);
    if (cosb > 0){
        angle = 360 - angle;
    }
    return angle;
}
```

#### 圆弧与直线（直线与圆弧）、圆弧与圆弧：

这类带圆弧的矢量夹角的求法其实就是求圆弧的等效直线，转化为直线与直线的矢量夹角求法。这里的求解非常简单，其实就是求与圆弧的相切直线。

![](https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/12.png?raw=true)

```cpp
DL_LineData cutterOffset::getArcLine2(DL_ArcData arc){
    DL_PointData startpoint = reader_->getArcStartPoint(arc);
    DL_PointData zeropoint(arc.cx,arc.cy,0);
    double slope = (zeropoint.y-startpoint.y)/(zeropoint.x-startpoint.x);
    double k_line2 = -1/slope;
    double b_line2 = startpoint.y - k_line2 * startpoint.x;
    double angle_point2 = arc.angle1 + (arc.angle2 - arc.angle1)/4;
    double x_point2 = arc.cx + arc.radius * cos(angle_point2 * M_PI/180);
    double y_point2 = k_line2 * x_point2 + b_line2;
    DL_LineData line2(startpoint.x,startpoint.y,0,x_point2,y_point2,0);
    return line2;
}
```

结合以上算法可得

```cpp
int cutterOffset::getVectorAngle(contour tour1,contour tour2){
    //线与线的转接形式
    int angle = -1;
    if (tour1.line.x1 != -1 && tour2.line.x1 != -1){
        DL_LineData line1 = tour1.line;
        DL_LineData line2 = tour2.line;
        angle = countVectorAngle(line1,line2);
    }
    //线与弧的转接形式
    else if (tour1.line.x1 != -1 && tour2.arc.cx != -1){
        DL_LineData line1 = tour1.line;
        DL_ArcData arc = tour2.arc;
        DL_LineData line2 = getArcLine2(arc);
        angle = countVectorAngle(line1,line2);
    }
    //弧与线的转接形式
    else if (tour1.arc.cx != -1 && tour2.line.x1 != -1){
        DL_LineData line2 = tour2.line;
        DL_ArcData arc = tour1.arc;
        DL_LineData line1 = getArcLine1(arc);
        angle = countVectorAngle(line1,line2);
    }
    //弧与弧的转接形式
    else if (tour1.arc.cx != -1 && tour2.arc.cx != -1){
        DL_ArcData arc1 = tour1.arc;
        DL_LineData line1 = getArcLine1(arc1);
        DL_ArcData arc2 = tour2.arc;
        DL_LineData line2 = getArcLine2(arc2);
        angle = countVectorAngle(line1,line2);
    }
    return angle;
}
```

### C刀补的转接形式：

C刀补一次读取两段，我们遍历由偏移图形组成的偏移路径，一次读两个图形，求出矢量夹角，并修正第一段的终点和第二段的起点，直到遍历完整个容器，所有偏移路径也就全部得到了修正。

#### 直线与直线转接：

![](https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/13.png?raw=true)

##### 缩短型、增长型：

这两种类型的求法在本质上是一致的，只是通过求两偏移直线的交点，然后根据交点分别对第一条直线的终点和第二条直线的起点作修正。

```cpp
DL_PointData cutterOffset::getCrossPoint(DL_LineData line1,DL_LineData line2){
    DL_PointData cross_point;
    double k1 = (line1.y1-line1.y2)/(line1.x1 - line1.x2);
    double b1 = line1.y1 - k1 * line1.x1;
    double k2 = (line2.y1 - line2.y2)/(line2.x1 - line2.x2);
    double b2 = line2.y1 - k2 * line2.x1;
    cross_point.y = (k1 * b2 - k2 * b1) / (k1 - k2);
    cross_point.x = (b1 - b2) / (k2 - k1);
    return cross_point;
}
```

##### 插入型：

插入型（下面的插入型也一样），因为在其中插入了一段直线，所以返回值的个数与缩短型和增长型不一样，所以我最后才用了用一个容器装修正好的偏移路径，也能使调用转接形式修正的函数通过判断返回容器的size()判断是否为插入型，并调整路径后续的排列。

直线的插入型，就是在分别直线偏移路径的延长线上找出距离偏移路径为一个刀补半径位置的点，然后你会得到两组解，因为插入型插入的点一定是在直线的延长线上的，通过这一点可以排除另外一组解。最后插入的直线为这两个点的连线。

#### 直线与圆弧转接（圆弧与直线转接）：

![](https://github.com/mai4567/CNC-markdown-picture/blob/main/%E5%9B%BE%E7%89%87/14.png?raw=true)

##### 缩短型、增长型：

本质上都是求交点坐标，只是求法略有不同。列出直线方程和圆方程，联立后使用求根公式（确保有交点的情况下，不然无法求出解析解），可得交点坐标。这里解一元二次方程组也是可能会有两个根，这里可以通过判断两个根，哪个离圆弧的起点/终点（直线与圆弧/圆弧与直线）或哪个离直线的终点/起点（直线与圆弧/圆弧与直线）更近，来舍去另一个根。对于直线的修正，可以直接用点修正，而圆弧则需要变换为角度才能进行修正，这个的求解非常简单，前面有些函数或多或少介绍了相关过程，这里不做重复。

```cpp
std::vector<DL_PointData> cutterOffset::getCrossPoint(DL_ArcData arc1,DL_LineData line2){
    std::vector<DL_PointData> cross_points;
    DL_PointData cross_point1,cross_point2;
    double k1 = (line2.y1-line2.y2)/(line2.x1 - line2.x2);
    double b1 = line2.y1 - k1 * line2.x1;

    double c = -1*arc1.cx;
    double d = -1*arc1.cy;
    double r = arc1.radius;
    cross_point1.x = -1*(sqrt((k1*k1+1)*(r*r) - c*c*k1*k1 + (2*c*d+2*b1*c)*k1 - d*d -2*b1*d - b1*b1) + (b1+d)*k1 + c) / (k1*k1+1);
    cross_point1.y = k1 * cross_point1.x + b1;
    cross_point2.x = (sqrt((k1*k1+1)*(r*r) - c*c*k1*k1 + (2*c*d+2*b1*c)*k1 - d*d -2*b1*d - b1*b1) + (-b1-d)*k1 - c) / (k1*k1+1);
    cross_point2.y = k1 * cross_point2.x + b1;
    cross_points.push_back(cross_point1);
    cross_points.push_back(cross_point2);
//    std::cout<<"cross_point1:("<<cross_point1.x<<","<<cross_point1.y<<")"<<std::endl;
//    std::cout<<"cross_point2:("<<cross_point2.x<<","<<cross_point2.y<<")"<<std::endl;
    return cross_points;
}
```

因为求解圆弧角度和圆弧本身的表达方式不一样，所以我做了一些投机取巧的操作--根据一个角度变换出圆弧几种可能正确的角度取最相近的那个。

```cpp
double cutterOffset::getCloseAngle(double angle,double compare){
    double get_angle;
    double middle_angle1;
    double middle_angle2;
    double p1 = 360 + angle;
    double p2 = 360 - angle;
    double p3 = angle - 360;
    double p4 = angle;
    double p5 = -1*angle;
    if (std::abs(p1-compare) > std::abs(p2-compare)){
        middle_angle1 = p2;
    }
    else{
        middle_angle1 = p1;
    }
    if (std::abs(p3-compare) > std::abs(p4-compare)){
        middle_angle2 = p4;
    }
    else{
        middle_angle2 = p3;
    }
    if (std::abs(p5 - compare) < std::abs(middle_angle2 - compare)){
        middle_angle2 = p5;
    }
    if (std::abs(middle_angle1-compare) > std::abs(middle_angle2-compare)){
        get_angle = middle_angle2;
    }
    else{
        get_angle = middle_angle1;
    }
    return get_angle;
}
```

##### 插入型：

这个部分和直线的插入型又略有不同，直线段插入点计算与前面介绍的相同。圆弧段插入点根据刀补半径，以圆弧起点/终点（直线与圆弧/圆弧与直线）为圆心构建新圆。主要通过新圆与旧圆的交点得到插入的点，然后通过判断插入点是否处于偏移路径的延长线上得到确切的解。这里的修正方法和上面的一样，但返回值需要多增加一条直线。

```cpp
std::vector<DL_PointData> cutterOffset::getCrossPoint(DL_ArcData arc1,DL_ArcData arc2){
    std::vector<DL_PointData> cross_points;
    double c1 = arc1.cx;
    double d1 = arc1.cy;
    double r1 = arc1.radius;
    double c2 = arc2.cx;
    double d2 = arc2.cy;
    double r2 = arc2.radius;
    double k = (c2 - c1) / (d1- d2);
    double b = ((c1*c1) - (c2*c2) + (d1*d1) - (d2*d2) + (r2*r2) - (r1*r1)) / (2*(d1-d2));
    cross_points = getCrossPoint(arc1,k,b);
    return cross_points;
}
```

#### 圆弧与圆弧转接：

三种过渡方式的求法其实已经介绍清楚了，根据圆弧与圆弧的矢量夹角灵活运用即可。

#### 刀补转接形式的代码：

```cpp
std::vector<contour> cutterOffset::fixOffsetPath(contour offset1,contour offset2,int angle){
    std::vector<contour> fixed_offset;
    //线与线的转接形式
    if (offset1.line.x1 != -1 && offset2.line.x2 != -1){
        DL_LineData line1 = offset1.line;
        DL_LineData line2 = offset2.line;
        //缩短型
        if (angle >= 180){
            contour fixpath1,fixpath2;
            DL_PointData cross_point = getCrossPoint(line1,line2);
            line1.x2 = cross_point.x;
            line1.y2 = cross_point.y;
            line2.x1 = cross_point.x;
            line2.y1 = cross_point.y;         
            fixpath1.line = line1;
            fixpath2.line = line2;
            fixed_offset.clear();
            fixed_offset.push_back(fixpath1);
            fixed_offset.push_back(fixpath2);
        }
        //伸长型
        else if (angle >= 90 && angle < 180){
            contour fixpath1,fixpath2;
            DL_PointData cross_point = getCrossPoint(line1,line2);
            line1.x2 = cross_point.x;
            line1.y2 = cross_point.y;
            line2.x1 = cross_point.x;
            line2.y1 = cross_point.y;          
            fixpath1.line = line1;
            fixpath2.line = line2;
            fixed_offset.clear();
            fixed_offset.push_back(fixpath1);
            fixed_offset.push_back(fixpath2);
        }
        //插入型
        else if (angle < 90){
            contour fixpath1,fixpath2,fixpath3;
            DL_PointData extend_point1,extend_point2;
            double k1 = (line1.y1-line1.y2)/(line1.x1 - line1.x2);
            double b1 = line1.y1 - k1 * line1.x1;
            double k2 = (line2.y1 - line2.y2)/(line2.x1 - line2.x2);
            double b2 = line2.y1 - k2 * line2.x1;
            double extendx11 = sqrt(pow(radius_,2)/(pow(k1,2)+1)) + line1.x2;
            double extendx12 = -1*sqrt(pow(radius_,2)/(pow(k1,2)+1)) + line1.x2;
            if ((extendx11 - line1.x2)/(line1.x2 - line1.x1) >= 0){
                extend_point1.x = extendx11;
            }
            else {
                extend_point1.x = extendx12;
            }
            extend_point1.y = k1 * extend_point1.x + b1;
            double extendx21 = sqrt(pow(radius_,2)/(pow(k2,2)+1)) + line2.x1;
            double extendx22 = -1*sqrt(pow(radius_,2)/(pow(k2,2)+1)) + line2.x1;
            if ((extendx21 - line2.x1)/(line2.x1 - line2.x2) >= 0){
                extend_point2.x = extendx21;
            }
            else {
                extend_point2.x = extendx22;
            }
            extend_point2.y = k2 * extend_point2.x + b2;
            line1.x2 = extend_point1.x;
            line1.y2 = extend_point1.y;
            line2.x1 = extend_point2.x;
            line2.y1 = extend_point2.y;       
            fixpath1.line = line1;
            fixpath2.line = DL_LineData(line1.x2,line1.y2,0,line2.x1,line2.y1,0);
            fixpath3.line = line2;
            fixed_offset.clear();
            fixed_offset.push_back(fixpath1);
            fixed_offset.push_back(fixpath2);
            fixed_offset.push_back(fixpath3);
        }
    }
    //线与弧的转接形式
    else if (offset1.line.x1 != -1 && offset2.arc.cx != -1){
        DL_LineData line1 = offset1.line;
        DL_ArcData arc2 = offset2.arc;
        //缩短型
        if (angle >= 180){
            contour fix_path1,fix_path2;
            std::vector<DL_PointData> cross_points = getCrossPoint(arc2,line1);
            DL_PointData cross_point;
            DL_PointData cross_point1 = cross_points[0];
            DL_PointData cross_point2 = cross_points[1];
            std::cout<<"point1:"<<cross_point1.x<<","<<cross_point1.y<<std::endl;
            std::cout<<"point2:"<<cross_point2.x<<","<<cross_point2.y<<std::endl;
            double cross_angle;
            double cross_angle1 = acos((cross_point1.x - arc2.cx)/arc2.radius)*180/M_PI;
            double cross_angle2 = acos((cross_point2.x - arc2.cx)/arc2.radius)*180/M_PI;
            cross_angle1 = getCloseAngle(cross_angle1,arc2.angle1);
            cross_angle2 = getCloseAngle(cross_angle2,arc2.angle1);
            if (std::abs(cross_angle1 - arc2.angle1) < std::abs(cross_angle2 - arc2.angle1)){
                cross_angle = cross_angle1;
                cross_point = cross_point1;
            }
            else {
                cross_angle = cross_angle2;
                cross_point = cross_point2;
            }

            fix_path1.line = DL_LineData(line1.x1,line1.y1,0,cross_point.x,cross_point.y,0);
            fix_path2.arc = DL_ArcData(arc2.cx,arc2.cy,arc2.cz,arc2.radius,cross_angle,arc2.angle2);

            fixed_offset.clear();
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
        }
        //伸长型
        else if (angle >= 90 && angle <180){
            contour fix_path1,fix_path2;
            std::vector<DL_PointData> cross_points = getCrossPoint(arc2,line1);
            DL_PointData cross_point;
            DL_PointData cross_point1 = cross_points[0];
            DL_PointData cross_point2 = cross_points[1];
            double cross_angle;
            double cross_angle1 = acos((cross_point1.x - arc2.cx)/arc2.radius)*180/M_PI;
            double cross_angle2 = acos((cross_point2.x - arc2.cx)/arc2.radius)*180/M_PI;
            cross_angle1 = getCloseAngle(cross_angle1,arc2.angle1);
            cross_angle2 = getCloseAngle(cross_angle2,arc2.angle1);
            if (std::abs(cross_angle1 - arc2.angle1) < std::abs(cross_angle2 - arc2.angle1)){
                cross_angle = cross_angle1;
                cross_point = cross_point1;
            }
            else {
                cross_angle = cross_angle2;
                cross_point = cross_point2;
            }         
            fix_path1.line = DL_LineData(line1.x1,line1.y1,0,cross_point.x,cross_point.y,0);
            fix_path2.arc = DL_ArcData(arc2.cx,arc2.cy,arc2.cz,arc2.radius,cross_angle,arc2.angle2);
            fixed_offset.clear();
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
        }
        //插入型
        else if (angle < 90){
            contour fix_path1,fix_path2,fix_path3;
            //修复直线段
            DL_PointData extend_point;
            double k = (line1.y1-line1.y2)/(line1.x1 - line1.x2);
            double b = line1.y1 - k * line1.x1;
            double extendx1 = sqrt(pow(radius_,2)/(pow(k,2)+1)) + line1.x2;
            double extendx2 = -1*sqrt(pow(radius_,2)/(pow(k,2)+1)) + line1.x2;
            if ((extendx1 - line1.x2)/(line1.x2 - line1.x1) >= 0){
                extend_point.x = extendx1;
            }
            else {
                extend_point.x = extendx2;
            }
            extend_point.y = k * extend_point.x + b;
            line1.x2 = extend_point.x;
            line1.y2 = extend_point.y;
            //修复圆弧段
            DL_PointData start_point = reader_->getArcStartPoint(arc2);
            DL_ArcData arc3 = DL_ArcData(start_point.x,start_point.y,0,radius_,0,359);
            std::vector<DL_PointData> cross_points;
            DL_PointData cross_point1;
            DL_PointData cross_point2;
            DL_PointData cross_point;
            cross_points = getCrossPoint(arc2,arc3);
            cross_point1 = cross_points[0];
            cross_point2 = cross_points[1];
            double cross_angle;
            double cross_angle1 = acos((cross_point1.x - arc2.cx)/arc2.radius)*180/M_PI;
            double cross_angle2 = acos((cross_point2.x - arc2.cx)/arc2.radius)*180/M_PI;
            cross_angle1 = getCloseAngle(cross_angle1,arc2.angle1);
            cross_angle2 = getCloseAngle(cross_angle2,arc2.angle1);

            if (((cross_angle1 - arc2.angle1) / (arc2.angle1 - arc2.angle2)) > 0){
                cross_angle = cross_angle1;
                cross_point = cross_point1;
            }
            else {
                cross_angle = cross_angle2;
                cross_point = cross_point2;
            }
            fix_path1.line = line1;
            fix_path2.line = DL_LineData(line1.x2,line1.y2,0,cross_point.x,cross_point.y,0);
            fix_path3.arc = DL_ArcData(arc2.cx,arc2.cy,arc2.cz,arc2.radius,cross_angle,arc2.angle2);
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
            fixed_offset.push_back(fix_path3);
        }
    }
    //弧与线的转接形式
    else if (offset1.arc.cx != -1 && offset2.line.x1 != -1){
        DL_ArcData arc1 = offset1.arc;
        DL_LineData line2 = offset2.line;
        //缩短型
        if (angle > 180){
            contour fix_path1,fix_path2;
            std::vector<DL_PointData> cross_points = getCrossPoint(arc1,line2);
            DL_PointData cross_point;
            DL_PointData cross_point1 = cross_points[0];
            DL_PointData cross_point2 = cross_points[1];
            double cross_angle;
            double cross_angle1 = acos((cross_point1.x - arc1.cx)/arc1.radius)*180/M_PI;
            double cross_angle2 = acos((cross_point2.x - arc1.cx)/arc1.radius)*180/M_PI;
            cross_angle1 = getCloseAngle(cross_angle1,arc1.angle2);
            cross_angle2 = getCloseAngle(cross_angle2,arc1.angle2);
            if (std::abs(cross_angle1 - arc1.angle2) < std::abs(cross_angle2 - arc1.angle2)){
                cross_angle = cross_angle1;
                cross_point = cross_point1;
            }
            else {
                cross_angle = cross_angle2;
                cross_point = cross_point2;
            } 
            fix_path1.arc = DL_ArcData(arc1.cx,arc1.cy,arc1.cz,arc1.radius,arc1.angle1,cross_angle);
            fix_path2.line = DL_LineData(cross_point.x,cross_point.y,0,line2.x2,line2.y2,0);
            fixed_offset.clear();
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
        }
        //伸长型
        else if (angle >= 90 && angle <180){
            contour fix_path1,fix_path2;
            std::vector<DL_PointData> cross_points = getCrossPoint(arc1,line2);
            DL_PointData cross_point;
            DL_PointData cross_point1 = cross_points[0];
            DL_PointData cross_point2 = cross_points[1];
            double cross_angle;
            double cross_angle1 = acos((cross_point1.x - arc1.cx)/arc1.radius)*180/M_PI;
            double cross_angle2 = acos((cross_point2.x - arc1.cx)/arc1.radius)*180/M_PI;
            cross_angle1 = getCloseAngle(cross_angle1,arc1.angle2);
            cross_angle2 = getCloseAngle(cross_angle2,arc1.angle2);
            if (std::abs(cross_angle1 - arc1.angle2) < std::abs(cross_angle2 - arc1.angle2)){
                cross_angle = cross_angle1;
                cross_point = cross_point1;
            }
            else {
                cross_angle = cross_angle2;
                cross_point = cross_point2;
            }
            fix_path1.arc = DL_ArcData(arc1.cx,arc1.cy,arc1.cz,arc1.radius,arc1.angle1,cross_angle);
            fix_path2.line = DL_LineData(cross_point.x,cross_point.y,0,line2.x2,line2.y2,0);
            fixed_offset.clear();
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
        }
        //插入型
        else if (angle < 90){   //may have some bug ; didn' test
            contour fix_path1,fix_path2,fix_path3;
            //修复圆弧段
            DL_PointData end_point = reader_->getArcEndPoint(arc1);
            DL_ArcData arc3 = DL_ArcData(end_point.x,end_point.y,0,radius_,0,359);
            std::vector<DL_PointData> cross_points;
            DL_PointData cross_point1;
            DL_PointData cross_point2;
            DL_PointData cross_point;
            cross_points = getCrossPoint(arc1,arc3);
            cross_point1 = cross_points[0];
            cross_point2 = cross_points[1];
            double cross_angle;
            double cross_angle1 = acos((cross_point1.x - arc1.cx)/arc1.radius)*180/M_PI;
            double cross_angle2 = acos((cross_point2.x - arc1.cx)/arc1.radius)*180/M_PI;
            cross_angle1 = getCloseAngle(cross_angle1,arc1.angle2);
            cross_angle2 = getCloseAngle(cross_angle2,arc1.angle2);

            if (((cross_angle1 - arc1.angle2) / (arc1.angle2 - arc1.angle1)) > 0){
                cross_angle = cross_angle1;
                cross_point = cross_point1;
            }
            else {
                cross_angle = cross_angle2;
                cross_point = cross_point2;
            }
            //修复直线段
            DL_PointData extend_point;
            double k = (line2.y1-line2.y2)/(line2.x1 - line2.x2);
            double b = line2.y1 - k * line2.x1;
            double extendx1 = sqrt(pow(radius_,2)/(pow(k,2)+1)) + line2.x1;
            double extendx2 = -1*sqrt(pow(radius_,2)/(pow(k,2)+1)) + line2.x1;
            if ((extendx1 - line2.x1)/(line2.x1 - line2.x2) >= 0){
                extend_point.x = extendx1;
            }
            else {
                extend_point.x = extendx2;
            }
            extend_point.y = k * extend_point.x + b;
            line2.x1 = extend_point.x;
            line2.y1 = extend_point.y;

            fix_path1.arc = DL_ArcData(arc1.cx,arc1.cy,arc1.cz,arc1.radius,arc1.angle1,cross_angle);
            fix_path2.line = DL_LineData(cross_point.x,cross_point.y,0,line2.x1,line2.y1,0);
            fix_path3.line = line2;
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
            fixed_offset.push_back(fix_path3);
        }
    }
    //弧与弧的转接形式
    else if (offset1.arc.cx != -1 && offset2.arc.cx != -1){
        DL_ArcData arc1 = offset1.arc;
        DL_ArcData arc2 = offset2.arc;
        //缩短型
        if (angle > 180){
            contour fix_path1,fix_path2;
            std::vector<DL_PointData> cross_points = getCrossPoint(arc1,arc2);
            DL_PointData cross_point1 = cross_points[0];
            DL_PointData cross_point2 = cross_points[1];
            double cross_angle1;
            double cross_angle2;

            double cross_angle11 = acos((cross_point1.x - arc1.cx)/arc1.radius)*180/M_PI;
            double cross_angle12 = acos((cross_point2.x - arc1.cx)/arc1.radius)*180/M_PI;
            cross_angle11 = getCloseAngle(cross_angle11,arc1.angle2);
            cross_angle12 = getCloseAngle(cross_angle12,arc1.angle2);
            double dis_p11 = std::abs(cross_angle11 - arc1.angle2) + std::abs(cross_angle11 - arc2.angle1);
            double dis_p12 = std::abs(cross_angle12 - arc1.angle2) + std::abs(cross_angle12 - arc2.angle1);
            if (dis_p11 < dis_p12){
                cross_angle1 = cross_angle11;
            }
            else {
                cross_angle1 = cross_angle12;
            }

            double cross_angle21 = acos((cross_point1.x - arc2.cx)/arc2.radius)*180/M_PI;
            double cross_angle22 = acos((cross_point2.x - arc2.cx)/arc2.radius)*180/M_PI;
            cross_angle21 = getCloseAngle(cross_angle21,arc2.angle1);
            cross_angle22 = getCloseAngle(cross_angle22,arc2.angle1);
            double dis_p21 = std::abs(cross_angle21 - arc1.angle2) + std::abs(cross_angle21 - arc2.angle1);
            double dis_p22 = std::abs(cross_angle22 - arc1.angle2) + std::abs(cross_angle22 - arc2.angle1);
            if (dis_p21 < dis_p22){
                cross_angle2 = cross_angle21;
            }
            else {
                cross_angle2 = cross_angle22;
            }

            fix_path1.arc = DL_ArcData(arc1.cx,arc1.cy,arc1.cz,arc1.radius,arc1.angle1,cross_angle1);
            fix_path2.arc = DL_ArcData(arc2.cx,arc2.cy,arc2.cz,arc2.radius,cross_angle2,arc2.angle2);
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
        }
        //伸长型
        else if (angle >= 90 && angle < 180){
            contour fix_path1,fix_path2;
            std::vector<DL_PointData> cross_points = getCrossPoint(arc1,arc2);
            DL_PointData cross_point1 = cross_points[0];
            DL_PointData cross_point2 = cross_points[1];
            double cross_angle1;
            double cross_angle2;

            double cross_angle11 = acos((cross_point1.x - arc1.cx)/arc1.radius)*180/M_PI;
            double cross_angle12 = acos((cross_point2.x - arc1.cx)/arc1.radius)*180/M_PI;
            cross_angle11 = getCloseAngle(cross_angle11,arc1.angle2);
            cross_angle12 = getCloseAngle(cross_angle12,arc1.angle2);
            double dis_p11 = std::abs(cross_angle11 - arc1.angle2) + std::abs(cross_angle11 - arc2.angle1);
            double dis_p12 = std::abs(cross_angle12 - arc1.angle2) + std::abs(cross_angle12 - arc2.angle1);
            if (dis_p11 < dis_p12){
                cross_angle1 = cross_angle11;
            }
            else {
                cross_angle1 = cross_angle12;
            }

            double cross_angle21 = acos((cross_point1.x - arc2.cx)/arc2.radius)*180/M_PI;
            double cross_angle22 = acos((cross_point2.x - arc2.cx)/arc2.radius)*180/M_PI;
            cross_angle21 = getCloseAngle(cross_angle21,arc2.angle1);
            cross_angle22 = getCloseAngle(cross_angle22,arc2.angle1);
            double dis_p21 = std::abs(cross_angle21 - arc1.angle2) + std::abs(cross_angle21 - arc2.angle1);
            double dis_p22 = std::abs(cross_angle22 - arc1.angle2) + std::abs(cross_angle22 - arc2.angle1);
            if (dis_p21 < dis_p22){
                cross_angle2 = cross_angle21;
            }
            else {
                cross_angle2 = cross_angle22;
            }

            fix_path1.arc = DL_ArcData(arc1.cx,arc1.cy,arc1.cz,arc1.radius,arc1.angle1,cross_angle1);
            fix_path2.arc = DL_ArcData(arc2.cx,arc2.cy,arc2.cz,arc2.radius,cross_angle2,arc2.angle2);
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
        }
        //插入型
        else if (angle < 90){
            contour fix_path1,fix_path2,fix_path3;
            //修复圆弧段1
            DL_PointData end_point = reader_->getArcEndPoint(arc1);
            DL_ArcData arc3 = DL_ArcData(end_point.x,end_point.y,0,radius_,0,359);
            std::vector<DL_PointData> cross_points1;
            DL_PointData cross_point11;
            DL_PointData cross_point12;
            DL_PointData cross_point1;
            cross_points1 = getCrossPoint(arc1,arc3);
            cross_point11 = cross_points1[0];
            cross_point12 = cross_points1[1];
            double cross_angle1;
            double cross_angle11 = acos((cross_point11.x - arc1.cx)/arc1.radius)*180/M_PI;
            double cross_angle12 = acos((cross_point12.x - arc1.cx)/arc1.radius)*180/M_PI;
            cross_angle11 = getCloseAngle(cross_angle11,arc1.angle2);
            cross_angle12 = getCloseAngle(cross_angle12,arc1.angle2);
            if (((cross_angle11 - arc1.angle2) / (arc1.angle2 - arc1.angle1)) > 0){
                cross_angle1 = cross_angle11;
                cross_point1 = cross_point11;
            }
            else {
                cross_angle1 = cross_angle12;
                cross_point1 = cross_point12;
            }
            //修复圆弧段2
            DL_PointData start_point = reader_->getArcStartPoint(arc2);
            DL_ArcData arc4 = DL_ArcData(start_point.x,start_point.y,0,radius_,0,359);
            std::vector<DL_PointData> cross_points2;
            DL_PointData cross_point21;
            DL_PointData cross_point22;
            DL_PointData cross_point2;
            cross_points2 = getCrossPoint(arc2,arc4);
            cross_point21 = cross_points2[0];
            cross_point22 = cross_points2[1];
            double cross_angle2;
            double cross_angle21 = acos((cross_point21.x - arc2.cx)/arc2.radius)*180/M_PI;
            double cross_angle22 = acos((cross_point22.x - arc2.cx)/arc2.radius)*180/M_PI;
            cross_angle21 = getCloseAngle(cross_angle21,arc2.angle1);
            cross_angle22 = getCloseAngle(cross_angle22,arc2.angle1);
            if (((cross_angle21 - arc2.angle1) / (arc2.angle1 - arc2.angle2)) > 0){
                cross_angle2 = cross_angle21;
                cross_point2 = cross_point21;
            }
            else {
                cross_angle2 = cross_angle22;
                cross_point2 = cross_point22;
            }

            fix_path1.arc = DL_ArcData(arc1.cx,arc1.cy,arc1.cz,arc1.radius,arc1.angle1,cross_angle1);
            fix_path2.line = DL_LineData(cross_point1.x,cross_point1.y,0,cross_point2.x,cross_point2.y,0);
            fix_path3.arc = DL_ArcData(arc2.cx,arc2.cy,arc2.cz,arc2.radius,cross_angle2,arc2.angle2);
            fixed_offset.push_back(fix_path1);
            fixed_offset.push_back(fix_path2);
            fixed_offset.push_back(fix_path3);
        }
    }
    return fixed_offset;
}
```

## 源码地址：

https://github.com/mai4567/CNC

## 给读者：

开源的动机是知道很多同学们做课设唯一的参考例子是用VB写的，这么多年了还是看了以前的东西，在我看来是没有进步的，最起码我不希望我的学弟学妹们一年后做课设的时候还是看的一样的东西。所以当时就决定尽快把这个课设搞出来然后开源给大家看。

开源的初衷是加强大家的交流学习，我希望大家看的时候更多的是抱着辩证的目光去看，取其精华去其糟粕。欢迎各位同学批评指正。