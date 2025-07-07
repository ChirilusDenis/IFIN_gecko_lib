#ifndef D2DISPLAY_H
#define D2DISPLAY_H
#include <QtGui>
#include <QtCore>
#include <QDebug>
#include <iostream>
#include <algorithm>
#include <stdint.h>
#include <QGraphicsScene>
#include <QtWidgets>

class D2Display: public QGraphicsView{

public:
    D2Display();
    QGraphicsScene *scene ;

    void setData2(const QVector<QVector<uint32_t> > &value);

    void resizeEvent();
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void mouseDoubleClickEvent();
    void paint();
    void mouseReleaseEvent(QMouseEvent *ev);

    QGraphicsRectItem *rect;
    QGraphicsTextItem *io;
    QGraphicsTextItem *io2;


    int getX1() const;
    void setX1(int value);

    int getX2() const;
    void setX2(int value);

    int getY1() const;
    void setY1(int value);

    int getY2() const;
    void setY2(int value);

    int getStart() const;
    void setStart(int value);

    int getStop() const;
    void setStop(int value);

    int getActionFlag() const;
    void setActionFlag(int value);

    int getD2min() const;
    void setD2min(int value);

    int getD2max() const;
    void setD2max(int value);

    float getCoef1() const;
    void setCoef1(float value);

    float getCoef2() const;
    void setCoef2(float value);

    int getD3x() const;
    void setD3x(int value);

    int getD3y() const;
    void setD3y(int value);

    int getD3lx() const;
    void setD3lx(int value);

    int getD3ly() const;
    void setD3ly(int value);

    float getCoef1y() const;
    void setCoef1y(float value);

    float getCoef2y() const;
    void setCoef2y(float value);

private:

    QVector<int> calculateVX(float, float);
    QVector<int> calculateVY(float, float);
    void setPixelColor(int, int, int);

    QVector<QVector<uint32_t> > data2;
    int maxx,maxy;
    int minx,miny;

    QImage *qi;

    QLabel *lbl;
    QGridLayout *lay;
    float coef1;
    float coef2;
    float coef1y;
    float coef2y;

    int x1;
    int x2;
    int y1;
    int y2;
    int actionFlag;
    int d2min;
    int d2max;
    int d3x;
    int d3y;
    int d3lx;
    int d3ly;
};
#endif // D2DISPLAY_H
