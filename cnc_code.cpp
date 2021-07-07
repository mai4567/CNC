#include <cnc_code.h>
#include <Widget.h>
#include <math.h>

void dxfReader::addLine(const DL_LineData& d) {
     lines_.push_back(d);
}


void dxfReader::addPoint(const DL_PointData & data){
    points_.push_back(data);
}


void dxfReader::addArc(const DL_ArcData& data){
    arcs_.push_back(data);
    draw_arcs_.push_back(data);
}


void dxfReader::addCircle(const DL_CircleData & data)
{
    circles_.push_back(data);
}


void dxfReader::messageOutput(){
    std::cout<<std::endl;
    if (!lines_.empty()){
        std::cout<<"Line:"<<std::endl;
        for (auto &line:lines_){
           std::cout<<"("<<line.x1<<","<< line.y1<<") ("<< line.x2 <<","<< line.y2<<")"<< std::endl;
        }
    }
    if (!circles_.empty()){
        std::cout<<"Circle:"<<std::endl;
        for (auto &circle:circles_){
            std::cout<<"("<<circle.cx<<","<<circle.cy<<") radius:"<<circle.radius<<std::endl;
        }
    }
    if (!arcs_.empty()){
        std::cout<<"ARC:"<<std::endl;
        for (auto &arc:arcs_){
            if (arc.angle1 > arc.angle2){
                arc.angle1 = arc.angle1 - 360;
            }
            std::cout<<"("<<arc.cx<<","<<arc.cy<<") begin:"<<arc.angle1<<" end:"<<arc.angle2<<" radius:"<<arc.radius<<std::endl;
        }
    }
    if (!draw_arcs_.empty()){
        std::cout<<"draw_ARC:"<<std::endl;
        for (auto &arc:draw_arcs_){
            std::cout<<arc.cx<<" , "<<arc.cy<<" begin:"<<arc.angle1<<" end:"<<arc.angle2<<" radius:"<<arc.radius<<std::endl;
        }
    }
    if (!points_.empty()){
        std::cout<<"Point:"<<std::endl;
        for (auto &point:points_){
            std::cout<<point.x<<","<<point.y<<std::endl;
        }
    }
}


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


std::vector<DL_LineData> dxfReader::deleteIndexLine(std::vector<DL_LineData> data,int index){  //从线的集合中删除对应索引的线
    std::vector<DL_LineData> lines;
    for (int i=0;i<int(data.size());i++){
        if (i != index){
            lines.push_back(data[i]);
        }
    }
    return lines;
}


std::vector<DL_ArcData> dxfReader::deleteIndexArc(std::vector<DL_ArcData> data, int index){  //从弧的集合中删除对应索引的弧
    std::vector<DL_ArcData> arcs;
    for (int i=0;i<int(data.size());i++){
        if (i != index){
            arcs.push_back(data[i]);
        }
    }
    return arcs;
}


DL_ArcData dxfReader::swapArcPoints(DL_ArcData arc){  //交换弧的起点和终点
    DL_ArcData swap_arc = DL_ArcData(0,0,0,0,0,0);
    swap_arc.cx = arc.cx;
    swap_arc.cy = arc.cy;
    swap_arc.cz = arc.cz;
    swap_arc.radius = arc.radius;
    swap_arc.angle1 = arc.angle2;
    swap_arc.angle2 = arc.angle1;
    return swap_arc;
}

DL_LineData dxfReader::swapLinePoints(DL_LineData line){  //交换线的起点和终点
    DL_LineData swap_line = DL_LineData(0,0,0,0,0,0);
    swap_line.x1 = line.x2;
    swap_line.y1 = line.y2;
    swap_line.x2 = line.x1;
    swap_line.y2 = line.y1;
    return swap_line;
}


int dxfReader::judgeStartPoint(DL_PointData point,std::vector<DL_LineData> lines, std::vector<DL_ArcData> arcs, int line_index, int arc_index){
    int status = 1;
    for (int i=0;i<int(lines.size());i++){
        if (i != line_index){
            DL_PointData now_point1 = DL_PointData(lines[i].x1,lines[i].y1,0);
            DL_PointData now_point2 = DL_PointData(lines[i].x2,lines[i].y2,0);
            double dis1 = std::sqrt(std::pow(point.y-now_point1.y,2)+std::pow(point.x-now_point1.x,2));
            double dis2 = std::sqrt(std::pow(point.y-now_point2.y,2)+std::pow(point.x-now_point2.x,2));
            if (dis1 <= 1 || dis2 <= 1){
                status = 0;
            }
        }
    }
    for (int j=0;j<int(arcs.size());j++){
        if (j != arc_index){
            DL_PointData now_point3 = getArcStartPoint(arcs[j]);
            DL_PointData now_point4 = getArcEndPoint(arcs[j]);
            double dis3 = std::sqrt(std::pow(point.y-now_point3.y,2)+std::pow(point.x-now_point3.x,2));
            double dis4 = std::sqrt(std::pow(point.y-now_point4.y,2)+std::pow(point.x-now_point4.x,2));
            if (dis3 <= 1 || dis4 <= 1){
                status = 0;
            }
        }
    }
    return status;
}


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


std::vector<DL_PointData> cutterOffset::getCrossPoint(DL_ArcData arc1,double k1, double b1){
    std::vector<DL_PointData> cross_points;
    DL_PointData cross_point1,cross_point2;
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
    //存放已修补刀补路径
    std::vector<contour> fixed_offset(reader_->lines_.size()+reader_->arcs_.size());
    for (int i=0;i<int(path.size());i++){
        for (int j=0;j<int(path[i].size()-1);j++){
            if (path[i][j].line.x1 != -1 || path[i][j].arc.cx != -1){
                int angle = getVectorAngle(path[i][j],path[i][j+1]);
                std::cout<<"angle:"<<angle<<std::endl;
                struct contour offset_contour1 = offsets[i][j];
                struct contour offset_contour2 = offsets[i][j+1];
                std::vector<contour> fixed_path = fixOffsetPath(offset_contour1,offset_contour2,angle);
                if (fixed_path.size() == 2){
                    offsets[i][j] = fixed_path[0];
                    offsets[i][j+1] = fixed_path[1];
                }
                //插入型有3个返回值
                else if (fixed_path.size() == 3){
                    //将从j+1开始的路径全部往后移一位
                    std::vector<contour> swap = offsets[i];
                    for (int now = j;now<int(offsets[i].size()-1);now++){
                        offsets[i][now+1] = swap[now];
                    }
                    offsets[i].push_back(swap[swap.size()-1]);
                    offsets[i][j] = fixed_path[0];
                    offsets[i][j+1] = fixed_path[1];
                    offsets[i][j+2] = fixed_path[2];
                }
            }
        }
    }

    //将刀补路径转换为可以显示的层级结构---------------------------------------------
    std::vector<DL_LineData> offsetline;
    std::vector<DL_ArcData> offsetarc;
    for (auto &offsetpath : offsets){
        for (auto &offset:offsetpath){
            if (offset.line.x1 != -1){
                offsetline.push_back(offset.line);
            }
            else if(offset.arc.cx != -1){
                offsetarc.push_back(offset.arc);
            }
        }
    }
    offset_lines_ = offsetline;
    offset_arcs_ = offsetarc;
    std::cout<<std::endl;
    std::cout<<"offset_path"<<std::endl;
    for (auto &contour:offsets)
    {
        for (auto &tour:contour){
            if (tour.line.x1 != -1){
                std::cout<<"line: p1:("<<tour.line.x1<<","<<tour.line.y1<<") p2:("<<
                           tour.line.x2<<","<<tour.line.y2<<")"<<std::endl;
            }
            else if (tour.arc.cx != -1){
                std::cout<<"arcs: p1:("<<reader_->getArcStartPoint(tour.arc).x<<","<<reader_->getArcStartPoint(tour.arc).y<<") p2:("<<
                           reader_->getArcEndPoint(tour.arc).x<<","<<reader_->getArcEndPoint(tour.arc).y<<")"<<std::endl;
            }
        }
    }
    //--------------------------------------------------------------------------
}

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
    clock_ = (circle.angle2 - circle.angle1)/std::abs(circle.angle2 - circle.angle1);
//    std::cout<<"start_point:("<<std::to_string(Xm_+X0)<<","<<std::to_string(Ym_+Y0)<<")"<<"   end_point:("<<
//               std::to_string(Xe_+X0)<<","<<std::to_string(Ye_+Y0)<<")"<<std::endl;
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


std::vector<int> circleInserter::getFm(){
    return Fms_;
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


std::vector<QPoint> lineInserter::getPoints(){
    return points_;
}


std::vector<int> lineInserter::getFm(){
    return Fms_;
}


std::vector<QPoint> circleInserter::getPoints(){
    return points_;
}

Insertion::Insertion(dxfReader *reader){
    lines_ = reader->lines_;
    circles_ = reader->circles_;
    arcs_ = reader->arcs_;
    points_ = reader->points_;
}


std::vector<std::vector<QPoint>> Insertion::lineInsert(std::vector<DL_LineData> lines,std::vector<std::vector<QPoint>> points){
    for (auto &line:lines){
       lineInserter *line_inserter = new lineInserter(line);
       line_inserter->lineInsert();
       std::vector<QPoint> line_points = line_inserter->getPoints();
       std::vector<int> fms = line_inserter->getFm();
       points.push_back(line_points);
       all_fms_.push_back(fms);
       delete line_inserter;
    }
    return points;
}


std::vector<std::vector<QPoint>> Insertion::circleInsert(std::vector<DL_ArcData> arcs,std::vector<std::vector<QPoint>> points){
    for (auto &arc:arcs){
        circleInserter *circle_inserter = new circleInserter(arc);
        circle_inserter->circleInsert();
        std::vector<QPoint> circle_points = circle_inserter->getPoints();
        std::vector<int> fms = circle_inserter->getFm();
        points.push_back(circle_points);
        all_fms_.push_back(fms);
        delete circle_inserter;
    }
    return points;
}


std::vector<std::vector<QPoint>> Insertion::getPoints(){
    return all_points_;
}

std::vector<std::vector<QPoint>> Insertion::getOffsetPoints(){
    return all_offset_points_;
}


std::vector<std::vector<int>> Insertion::getFm(){
    return all_fms_;
}

cutterOffset::cutterOffset(int radius,int status,dxfReader *reader){
    radius_ = radius;
    if (status == 1){
        left_status_ = 1;
    }
    else if (status == -1) {
        left_status_ = 0;
    }
    reader_ = reader;
}

DL_LineData cutterOffset::getArcLine1(DL_ArcData arc){
    DL_PointData endpoint = reader_->getArcEndPoint(arc);
    DL_PointData zeropoint(arc.cx,arc.cy,0);
    double slope = (zeropoint.y-endpoint.y)/(zeropoint.x-endpoint.x);
    double k_line1 = -1/slope;
    double b_line1 = endpoint.y - k_line1 * endpoint.x;
    double angle_point1 = arc.angle1 + (arc.angle2 - arc.angle1)/4;
    double x_point1 = arc.cx + arc.radius * cos(angle_point1 * M_PI/180);
    double y_point1 = k_line1 * x_point1 + b_line1;
    DL_LineData line1(x_point1,y_point1,0,endpoint.x,endpoint.y,0);
    return line1;
}


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

DL_LineData cutterOffset::pulseOffset(DL_LineData line){
    DL_LineData pulse_line(0,0,0,0,0,0);
    if (left_status_ == 1){
        if (line.x2 > line.x1 && line.y2 == line.y1){
            pulse_line = DL_LineData(line.x1,line.y1-radius_,0,line.x2,line.y2-radius_,0);
        }
        else if (line.x2 <line.x1 && line.y2 == line.y1){
           pulse_line = DL_LineData(line.x1,line.y1+radius_,0,line.x2,line.y2+radius_,0);
        }
        else if (line.y2 > line.y1 && line.x2 == line.x1){
            pulse_line = DL_LineData(line.x1+radius_,line.y1,0,line.x2+radius_,line.y2,0);
        }
        else if (line.y2 < line.y1 && line.x2 == line.x1){
            pulse_line = DL_LineData(line.x1-radius_,line.y1,0,line.x2-radius_,line.y2,0);
        }
    }
    else if (left_status_ == 0){
        if (line.x2 > line.x1 && line.y2 == line.y1){
            pulse_line = DL_LineData(line.x1,line.y1+radius_,0,line.x2,line.y2+radius_,0);
        }
        else if (line.x2 < line.x1 && line.y2 == line.y1){
           pulse_line = DL_LineData(line.x1,line.y1-radius_,0,line.x2,line.y2-radius_,0);
        }
        else if (line.y2 > line.y1 && line.x2 == line.x1){
            pulse_line = DL_LineData(line.x1-radius_,line.y1,0,line.x2-radius_,line.y2,0);
        }
        else if (line.y2 < line.y1 && line.x2 == line.x1){
            pulse_line = DL_LineData(line.x1+radius_,line.y1,0,line.x2+radius_,line.y2,0);
        }
    }
    return pulse_line;
}

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
