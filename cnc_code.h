#ifndef CNC_CODE_H
#define CNC_CODE_H

#include "src/dl_creationadapter.h"
#include "src/dl_codes.h"
#include "src/dl_global.h"
#include "src/dl_dxf.h"
#include <QPoint>

struct PointInsert{
    int x1;
    int y1;
    int x2;
    int y2;
};

class dxfReader : public DL_CreationAdapter {
public:
    std::vector<DL_LineData> lines_{};
    std::vector<DL_ArcData> arcs_{};
    std::vector<DL_ArcData> draw_arcs_{};
    std::vector<DL_PointData> points_{};
    std::vector<DL_CircleData> circles_{};
    virtual void addLine(const DL_LineData& d);
    virtual void addPoint(const DL_PointData & data);
    virtual void addArc(const DL_ArcData& data);
    virtual void addCircle(const DL_CircleData & data);
    void messageOutput(); 
    DL_PointData getArcStartPoint(DL_ArcData arc);
    DL_PointData getArcEndPoint(DL_ArcData arc);
    DL_LineData swapLinePoints(DL_LineData line);
    DL_ArcData swapArcPoints(DL_ArcData arc);
    std::vector<DL_LineData> deleteIndexLine(std::vector<DL_LineData> data,int index);
    std::vector<DL_ArcData> deleteIndexArc(std::vector<DL_ArcData> data,int index);
    int judgeStartPoint(DL_PointData point,std::vector<DL_LineData> lines,std::vector<DL_ArcData> arcs,int line_index,int arc_index);
};


class lineInserter{
public:
    lineInserter(DL_LineData line);
    void lineInsert();
    std::vector<QPoint> getPoints();
    std::vector<int> getFm();
private:
    int Fm_,Xm_,Ym_,step_,Xe_,Ye_;
    int X0,Y0;
    std::vector<QPoint> points_;
    std::vector<int> Fms_;
};

class circleInserter{
public:
    circleInserter(DL_ArcData circles);
    void circleInsert();
    std::vector<QPoint> getPoints();
    std::vector<int> getFm();
private:
    int Fm_,Xe_,Ye_,Xm_,Ym_,step_;
    int X0,Y0;
    int clock_;
    std::vector<QPoint> points_;
    std::vector<int> Fms_;
};


class Insertion{
public:
    std::vector<DL_LineData> lines_{};
    std::vector<DL_ArcData> arcs_{};
    std::vector<DL_PointData> points_{};
    std::vector<DL_CircleData> circles_{};
    std::vector<std::vector<QPoint>> all_points_;
    std::vector<std::vector<QPoint>> all_offset_points_;
    Insertion(dxfReader *reader);
    std::vector<std::vector<QPoint>> lineInsert(std::vector<DL_LineData> lines,std::vector<std::vector<QPoint>> points);
    std::vector<std::vector<QPoint>> circleInsert(std::vector<DL_ArcData> arcs,std::vector<std::vector<QPoint>> points);
    std::vector<std::vector<QPoint>> getPoints();
    std::vector<std::vector<int>> getFm(); 
    std::vector<std::vector<QPoint>> getOffsetPoints();
private:
    std::vector<std::vector<int>> all_fms_;
};


struct contour{
    DL_LineData line = DL_LineData(-1,-1,-1,-1,-1,-1);
    DL_ArcData arc = DL_ArcData(-1,-1,-1,-1,-1,-1);
};

class cutterOffset{
public:
    cutterOffset(int radius,int status,dxfReader *reader);
    std::vector<std::vector<contour>> reorderPattern();
    int getVectorAngle(contour tour1,contour tour2);
    int countVectorAngle(DL_LineData line1,DL_LineData line2);
    DL_LineData pulseOffset(DL_LineData line);
    DL_LineData lineOffset(DL_LineData line);
    DL_ArcData arcOffset(DL_ArcData arc);
    contour contourOffset(contour path);
    DL_PointData getCrossPoint(DL_LineData line1,DL_LineData line2);
    std::vector<DL_PointData> getCrossPoint(DL_ArcData arc1,double k1, double b1);
    std::vector<DL_PointData> getCrossPoint(DL_ArcData arc1,DL_ArcData arc2);
    std::vector<DL_PointData> getCrossPoint(DL_ArcData arc1,DL_LineData line2);
    double getCloseAngle(double angle,double compare);
    std::vector<contour> fixOffsetPath(contour offset1,contour offset2,int angle);
    void fixOffset(std::vector<std::vector<contour>> path);
    std::vector<DL_ArcData> getOffsetArcs(){
        return offset_arcs_;
    }
    std::vector<DL_LineData> getOffsetLines(){
        return offset_lines_;
    }
private:
    dxfReader *reader_;
    int radius_;
    int left_status_;
    std::vector<DL_LineData> offset_lines_;
    std::vector<DL_ArcData> offset_arcs_;
    DL_LineData getArcLine1(DL_ArcData arc);
    DL_LineData getArcLine2(DL_ArcData arc);
};

#endif // CNC_CODE_H
