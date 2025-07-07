#include "2Ddisplay.h"

D2Display::D2Display()
{
    this->setGeometry(0,0,2048,1024);
    scene = new QGraphicsScene();
    this->setScene(scene);
    this->setSceneRect(0,0,this->width(),this->height());
    this->setD2max(8192);
    this->setD2min(0);
    this->setActionFlag(1);
}

void D2Display::setData2(const QVector<QVector<u_int32_t> > &value)
{
    data2 = value;
}

void D2Display::mousePressEvent(QMouseEvent *ev)
{
    double trueXValue,trueYValue;

    trueXValue=minx+(maxx-minx)*(ev->x())/(this->geometry().width());
    trueYValue=miny+(maxy-miny)*(ev->y())/(this->geometry().height());

    io = new QGraphicsTextItem;
    io->setDefaultTextColor(QColor(255,250,250));
    io2 = new QGraphicsTextItem;
    io2->setDefaultTextColor(QColor(255,250,250));

    if ((ev->y()<25)&&(ev->x()>this->geometry().width()-180))
        io->setPos(ev->x()-180,ev->y());
    else if (ev->x()>this->geometry().width()-180)
        io->setPos(ev->x()-180,ev->y()-25);
    else if(ev->y()<25)
        io->setPos(ev->x(),ev->y());
    else
        io->setPos(ev->x(),ev->y()-25);
    io->setPlainText(QString("x: ")+QString::number(trueXValue)+QString(" | ") +QString("y: ")+ QString::number(trueYValue));

    rect = new QGraphicsRectItem(0,0,0,0);
    scene->addItem(rect);
    scene->addItem(io);
    scene->addItem(io2);
    this->setX1(ev->x());
    this->setY1(ev->y());
}

void D2Display::mouseMoveEvent(QMouseEvent *ev)
{
    int leftCornerX, leftCornerY, rightCornerX, rightCornerY, setX, setY;
    double trueXValue,trueYValue;

    trueXValue=minx+(maxx-minx)*(ev->x())/(this->geometry().width());
    trueYValue=miny+(maxy-miny)*(ev->y())/(this->geometry().height());

    rect->setBrush(QBrush(QColor(255, 255, 255, 80)));

    //Find the x coordinate of the left-hand side of the zoom rectangle
    leftCornerX = std::min(ev->x(),this->getX1());
    if(leftCornerX<0) leftCornerX=0;
    this->setD3x(leftCornerX);

    //Find the y coordinate of the left-hand side of the zoom rectangle
    leftCornerY = std::min(ev->y(),this->getY1());
    if(leftCornerY<0) leftCornerY=0;
    this->setD3y(leftCornerY);

    //Find the width of the zoom rectangle
    rightCornerX = std::max(ev->x(),this->getX1());
    if(rightCornerX>this->width()) rightCornerX=this->width();
    this->setD3lx(rightCornerX-leftCornerX);

    //Find the height of the zoom rectangle
    rightCornerY = std::max(ev->y(),this->getY1());
    if(rightCornerY>this->height()-15) rightCornerY=this->height()-15;
    this->setD3ly(rightCornerY-leftCornerY);

    rect->setRect(this->getD3x(), this->getD3y(), this->getD3lx(), this->getD3ly());

    if(ev->x()>this->width()-60)
        setX=this->width()-200;
    else if (ev->x()<150)
        setX=10;
    else setX=ev->x()-140;

    if(ev->y()>this->height()-40)
        setY=this->height()-45;
    else if (ev->y()<10)
        setY=5;
    else setY=ev->y()-5;

    io2->setPos(setX,setY);
    io2->setPlainText(QString("x: ")+QString::number(trueXValue)+QString(" | ") +QString("y: ")+ QString::number(trueYValue));
}

void D2Display::mouseDoubleClickEvent()
{
    this->setActionFlag(1);
    this->paint();
}

void D2Display::mouseReleaseEvent(QMouseEvent *ev)
{
    if(ev->x()>this->width()-11)
        this->setX2(this->width()-11);
    else this->setX2(ev->x());

    if(ev->y()>this->height()-11)
        this->setX2(this->height()-11);
    else this->setY2(ev->y());

    if(rect->isActive())
        scene->removeItem(rect);
    if(io->isActive())
        scene->removeItem(io);
    if(io2->isActive())
        scene->removeItem(io2);

    if(abs(this->getX2()-this->getX1()) > 20)
        this->paint();
}

void D2Display::paint()
{
    scene->clear();

    float w = this->width()-10;
    float h = this->height()-10;
    qi = new QImage(w,h,QImage::Format_RGB32);
    qi->fill(Qt::black);
//    std::cout<<w<<" "<<h<<" "<<qi->size().rwidth()<<" "<<qi->size().rheight()<<std::endl;

    if (this->actionFlag==1)
    {
        //Unzoom la dublu click
        minx = 0;
        miny = 0;
        maxx = 8192;
        maxy = 8192;
        this->setActionFlag(3);
    }
    else if (this->actionFlag==2)
    {
        //Pentru window resize
        this->setActionFlag(3);
    }
    else if (this->actionFlag==3)
    {
        minx = this->getD3x()*this->getCoef1()+this->getCoef2();
        miny = this->getD3y()*this->getCoef1y()+this->getCoef2y();
        maxx = (this->getD3lx()+this->getD3x())*this->getCoef1()+this->getCoef2();
        maxy = (this->getD3ly()+this->getD3y())*this->getCoef1y()+this->getCoef2y();
    }

    float rx=w/(maxx-minx);
    float ry=h/(maxy-miny);

    int ax=0;
    int ay=0;
    int bx=0;
    int by=0;

    this->setCoef1(((float)(maxx-minx))/this->width());
    this->setCoef2(minx);
    this->setCoef1y(((float)(maxy-miny))/this->height());
    this->setCoef2y(miny);

    QVector<int> vx;
    QVector<int> vy;
    vx=calculateVX(w,rx);
    vy=calculateVY(h,ry);

    for(int i = 0; i < vy.size(); i++)
    {
        by = by + vy.at(i);

        if(by > maxy||by<0)
            break;

        ax = 0;
        bx = 0;

        for (int j = 0; j < vx.size(); j++)
        {
            double buffer = 0;
            bx = bx + vx.at(j);
            if(bx > maxx||bx<0)
                break;

            if (rx <= 1 && ry <= 1)
            {
                for(int k = ay; k < by; k++)
                {
                    for(int l = ax; l < bx; l++)
                    {
                        buffer = buffer + data2[miny+k][minx+l];
                    }
                }
                int val = buffer;
                setPixelColor(val,j,i);
            }
            else if (rx > 1 && ry > 1)
            {
                for(int k = ay; k < by; k++)
                {
                    for(int l = ax; l < bx; l++)
                    {
                        int val = this->data2[miny+i][minx+j];
                        setPixelColor(val,l,k);
                    }
                }
            }
            else if (rx <= 1 && ry > 1 )
            {
                for(int l = ax; l < bx; l++)
                {
                    buffer = buffer + data2[miny+i][minx+l];
                }
                int val = buffer;
                for(int k = ay; k < by; k++)
                {
                    setPixelColor(val,j,k);
                }
            }
            else if (rx > 1 && ry <= 1)
            {
                    for(int k = ay; k < by; k++)
                    {
                       buffer = buffer + data2[miny+k][minx+j];
                    }
                    int val = buffer;
                    for(int l = ax; l < bx; l++)
                    {
                        setPixelColor(val,l,i);
                    }
            }
        ax = ax + vx.at(j);
        }
    ay = ay + vy.at(i);
    }

    scene->addPixmap(QPixmap::fromImage(*qi));
}

void D2Display::setPixelColor(int val, int j, int i)
{
    if(val > 0)
    {

        if (val > 120 )
            qi->setPixel(j,i,qRgb(192,57,43));

        else if(val > 1 && val < 45)
            qi->setPixel(j,i,qRgb(36,113,163));

        else if (val > 45 && val <  75)
            qi->setPixel(j,i,qRgb(34,153,84));

        else
            qi->setPixel(j,i,qRgb(244,208,63));

    }
}

QVector<int> D2Display::calculateVX(float width, float rx)
{
    QVector<int> vx;

    //Calculate vx for less pixels than point or for more
    if(rx<=1)
    {
        int m=0;
        float x=0;

        for (int i = 0; i < width; i++)
        {
            m = 0;
            while(1)
            {
                x+=rx;
                m++;
                if(x > 1)
                {
                    x = x - 1;
                    break;
                }
            }
            vx.append(m);
        }
    }
    else
    {
        float n=0;
        float x=0;
        int sum=0;

        while(1)
        {
            x = n + rx;
            vx.append(floor(x));
            n = x - floor(x);
            sum = sum + floor(x);
            if(sum > this->width()-12)
                break;
        }
    }

    return vx;
}

QVector<int> D2Display::calculateVY(float height, float ry)
{
    QVector<int> vy;

    //Calculate vy for less pixels than point or for more
    if(ry<=1)
    {
        int m=0;
        float y=0;

        for (int i = 0; i < height; i++)
        {
            m = 0;
            while(1)
            {
                y+=ry;
                m++;
                if(y > 1)
                {
                    y = y - 1;
                    break;
                }
            }
            vy.append(m);
        }
    }
    else
    {
        float n=0;
        float y=0;
        int sum=0;

        while(1)
        {
            y =  n + ry;
            vy.append(floor(y));
            n = y - floor(y);
            sum = sum + floor(y);
            if(sum > this->height()-12)
                break;
        }
    }

    return vy;
}

void D2Display::resizeEvent()
{
    if(maxx==0||maxy==0)
    {
    minx = 0;
    miny = 0;
    maxx = 8192;
    maxy = 8192;
    }

    this->setActionFlag(2);
    this->paint();
}

int D2Display::getX1() const
{
    return x1;
}

void D2Display::setX1(int value)
{
    x1 = value;
}
int D2Display::getX2() const
{
    return x2;
}

void D2Display::setX2(int value)
{
    x2 = value;
}
int D2Display::getY1() const
{
    return y1;
}

void D2Display::setY1(int value)
{
    y1 = value;
}
int D2Display::getY2() const
{
    return y2;
}

void D2Display::setY2(int value)
{
    y2 = value;
}

int D2Display::getActionFlag() const
{
    return actionFlag;
}

void D2Display::setActionFlag(int value)
{
    actionFlag = value;
}

int D2Display::getD2min() const
{
    return d2min;
}

void D2Display::setD2min(int value)
{
    d2min = value;
}

int D2Display::getD2max() const
{
    return d2max;
}

void D2Display::setD2max(int value)
{
    d2max = value;
}

float D2Display::getCoef1() const
{
    return coef1;
}

void D2Display::setCoef1(float value)
{
    coef1 = value;
}

float D2Display::getCoef2() const
{
    return coef2;
}

void D2Display::setCoef2(float value)
{
    coef2 = value;
}

int D2Display::getD3x() const
{
    return d3x;
}

void D2Display::setD3x(int value)
{
    d3x = value;
}

int D2Display::getD3y() const
{
    return d3y;
}

void D2Display::setD3y(int value)
{
    d3y = value;
}

int D2Display::getD3lx() const
{
    return d3lx;
}

void D2Display::setD3lx(int value)
{
    d3lx = value;
}

int D2Display::getD3ly() const
{
    return d3ly;
}

void D2Display::setD3ly(int value)
{
    d3ly = value;
}

float D2Display::getCoef1y() const
{
    return coef1y;
}

void D2Display::setCoef1y(float value)
{
    coef1y = value;
}

float D2Display::getCoef2y() const
{
    return coef2y;
}

void D2Display::setCoef2y(float value)
{
    coef2y = value;
}
