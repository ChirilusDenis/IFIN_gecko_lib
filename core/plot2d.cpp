/*
Copyright 2011 Bastian Loeher, Roland Wirth

This file is part of GECKO.

GECKO is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GECKO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "plot2d.h"
#include "samqvector.h"

#include <limits>
#include <QTimer>
#include <QPixmap>

using namespace std;

Annotation::Annotation(QPoint _p, annoType _type, QString _text)
: p(_p)
, text (_text)
, type(_type)

{
}

Annotation::~Annotation()
{
}

Channel::Channel(QVector<double> _data)
: xmin (0)
, xmax (0)
, ymin (0)
, ymax (0)
, ylog (false)
, xlog (false)
, color (Qt::black)
, name ("unset")
, type (line)
, id (0)
, enabled (true)
, stepSize (1)
, data (_data)
{
}

Channel::~Channel()
{
    clearAnnotations();
}

void Channel::clearAnnotations(){
    QList<Annotation *> annos (annotations);

    annotations.clear ();
    foreach (Annotation *a, annos)
        delete a;
}
void Channel::setColor(QColor color){ this->color = color;}
void Channel::setData(QVector<double> _data)
{
    this->data = _data;
    emit changed ();
}

void Channel::setEnabled(bool enabled){ this->enabled = enabled;}
void Channel::setId(unsigned int id){ this->id = id;}
void Channel::setName(QString name){ this->name = name;}
void Channel::setType(plotType type){ this->type = type;}
void Channel::setStepSize(double stepSize){ this->stepSize = stepSize;}

void Channel::addAnnotation(QPoint p, Annotation::annoType type, QString text)
{
    Annotation* a = new Annotation(p,type,text);
    this->annotations.push_back(a);
}

// Plot2D
plot2d::plot2d(QWidget *parent, QSize size, unsigned int id)
        : QWidget(parent)
        , state(0)
        , curTickCh(0)
        , calibration (0)
        , scalemode (ScaleOff)
        , viewport (0, -0.15, 1, 1)
        , backbuffer (NULL)
        , backbuffervalid (false)

{
    this->id = id;
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(false);

    this->setGeometry(QRect(QPoint(0,0),size));
    this->channels = new QList<Channel*>;
//    xmin = 0; ymin = 0;
//    xmax = this->width(); ymax = this->height();

    useExternalBoundaries = false;
    zoomExtendsTrue = false;
//    ext_xmin = xmin;
//    ext_xmax = xmax;
//    ext_ymin = ymin;
//    ext_ymax = ymax;

    createActions();
    setMouseTracking(true);
}

plot2d::~plot2d()
{
    channels->clear();
    delete channels;
    delete backbuffer;
}

void plot2d::resizeEvent(QResizeEvent *event)
{
    delete backbuffer;
    backbuffer = NULL;
    update();
    QWidget::resizeEvent(event);
}

void plot2d::addChannel(unsigned int id, QString name, QVector<double> data,
                        QColor color, Channel::plotType type = Channel::line, double stepSize = 1)
{
    Channel *newChannel = new Channel(data);
    newChannel->setColor(color);
    newChannel->setName(name);
    newChannel->setId(id);
    newChannel->setStepSize(stepSize);
    newChannel->setType(type);
    this->channels->append(newChannel);
    connect (newChannel, SIGNAL(changed()), SLOT(channelUpdate()));
    viewport.setCoords (0, -0.15, 1, 1);
    QAction* a = new QAction(tr("Ch %1").arg(id,1,10),this);
    a->setData(QVariant::fromValue(id));
   // setCurTickChActions.push_back(a);
}

void plot2d::removeChannel(unsigned int id)
{
    for(unsigned int i = 0; i < this->getNofChannels(); i++)
    {
        if(this->channels->at(i)->getId() == id)
        {
            Channel * ch = channels->at(i);
            ch->disconnect(SIGNAL(changed()), this, SLOT(channelUpdate()));
            channels->removeAt(i);
            delete ch;
        }
    }
}

void plot2d::mousePressEvent (QMouseEvent *ev) {
    if (ev->y () <= height ()*0.15 || ev->y () >= 0.85*height ()) {
        scalemode = ScaleX;
        scalestart = viewport.x () + ev->x () / (1.0 * width ()) * viewport.width ();
        scaleend = scalestart;
    } else if (ev->x () <= width()*0.05 || ev->x () >= width ()*0.95) {
        scalemode = ScaleY;
        scalestart = viewport.y () + ev->y () / (1.0 * height ()) * viewport.height ();
        scaleend = scalestart;
    }
}

void plot2d::mouseReleaseEvent(QMouseEvent *) {
    switch (scalemode) {
    case ScaleX:
        if (fabs (scalestart - scaleend) / viewport.width () * width () >= 5) {
            if(scalestart<0) scalestart=0;
            if(scaleend<0) scaleend=0;
            if(scalestart>1) scalestart=1;
            if(scaleend>1) scaleend=1;
            viewport.setLeft (std::min (scalestart, scaleend));
            viewport.setRight (std::max (scalestart, scaleend));
            autoscaleZoom();
        }
        break;
    case ScaleY:
        if (fabs (scalestart - scaleend) / viewport.height () * height ()>= 5) {
            if(scalestart>1) scalestart=1;
            if(scaleend>1) scaleend=1;
            if(scalestart<-0.15) scalestart=-0.15;
            if(scaleend<-0.15) scaleend=-0.15;
            viewport.setTop (std::min (scalestart, scaleend));
            viewport.setBottom (std::max (scalestart, scaleend));
        }
        break;
    default:
        break;
    }

    if (scalemode != ScaleOff) {
        scalemode = ScaleOff;
        unsetCursor ();
        backbuffervalid = false;
        update ();
    }
}

void plot2d::mouseMoveEvent(QMouseEvent *ev)
{
    curxmin = channels->at(curTickCh)->xmin;
    curxmax = channels->at(curTickCh)->xmax;
    curymin = channels->at(curTickCh)->ymin;
    curymax = channels->at(curTickCh)->ymax;
    Channel *curChan = channels->at(0);
    QVector<double> data = curChan->getData();
    QPoint p = (ev->pos());
    double toolTipx2=0;
    int toolTipx,toolTipx3;
    toolTipx3=(viewport.x()+viewport.width()*p.x()/width())*(curxmax-curxmin);
    if (calibration)
    {
        toolTipx2=(viewport.x()+viewport.width()*p.x()/width())*(curxmax-curxmin)*calibCoef1;
        toolTipx=toolTipx2;

        QToolTip::showText(ev->globalPos(),
                           tr("E:%1\nCounts:%2")
                           .arg((int)(toolTipx),3,10)
                           .arg((int)(data[toolTipx3]),3,10),this);

    }
    else
        QToolTip::showText(ev->globalPos(),
                           tr("%1,%2")
                           .arg((int)((viewport.x()+viewport.width()*p.x()/width())*(curxmax-curxmin)),3,10)
                           .arg((int)(data[toolTipx3]),3,10),this);

    if (ev->y () <= height ()*0.15 || ev->y () >= height ()*0.85) {
        setCursor (Qt::SizeHorCursor);
    } else if (ev->x () <= width()*0.05 || ev->x () >= width()*0.95) {
        setCursor (Qt::SizeVerCursor);
    } else if (scalemode == ScaleOff) {
        // XXX: does this have to happen everytime the mouse moves? --rw
        unsetCursor ();
    }

    if (scalemode == ScaleX || scalemode == ScaleY) {
        switch (scalemode) {
        case ScaleX: scaleend = viewport.x () + ev->x () / (1.0 * width ()) * viewport.width (); break;
        case ScaleY: scaleend = viewport.y () + ev->y () / (1.0 * height ()) * viewport.height (); break;
        default: break;
        }
        update ();
    }
}

void plot2d::mouseDoubleClickEvent(QMouseEvent *) {
    viewport.setCoords (0, -0.15, 1, 1);
    backbuffervalid = false;
    update ();
}

void plot2d::autoscaleZoom()
{
    curxmin = channels->at(curTickCh)->xmin;
    curxmax = channels->at(curTickCh)->xmax;
    curymax = channels->at(curTickCh)->ymax;
    QVector<double> data = channels->at(0)->getData();
    int datamin=(int)(viewport.x()*(curxmax-curxmin));
    int datamax=(int)((viewport.x()+viewport.width())*(curxmax-curxmin));
    double localmax=0;
    for(int i=datamin;i<datamax;i++)
        {if (data[i]>localmax) localmax=data[i];}
    if(state==1)  {
        if(curymax>1)
            viewport.setTop (1-1.15*log10(localmax)/log10(curymax));
        else viewport.setTop (-0.15);
    }
    else if(state==2)  {
        viewport.setTop (1-1.15*sqrt(localmax)/sqrt(curymax));
    }
    else        {
        viewport.setTop (1-1.15*localmax/curymax);
    }
}

void plot2d::setZoom(double x, double width)
{
    viewport.setX(x);
    viewport.setWidth(width);
    autoscaleZoom();
}

void plot2d::channelUpdate () {
    backbuffervalid = false;
}

void plot2d::paintEvent(QPaintEvent *)
{
    //printf("paintEvent\n"); fflush(stdout);
    if (!backbuffer || !backbuffervalid) {
        if (!backbuffer) {
            backbuffer = new QPixmap (size ());
        }

        backbuffer->fill (isEnabled () ? QColor(252,252,252) : Qt::lightGray);
        QPainter pixmappainter (backbuffer);
        {
            QReadLocker rd (&lock);
            setBoundaries();
            drawChannels(pixmappainter);
        }
        if(state==1)
            drawLogTicks(pixmappainter);
        else if(state==2)
            drawSqrtTicks(pixmappainter);
        else
            drawTicks(pixmappainter);
        backbuffervalid = true;
    }

    QPainter painter (this);
    painter.drawPixmap(0, 0, *backbuffer);

    if (scalemode == ScaleX) {
        double start = (scalestart - viewport.x ()) / viewport.width();
        double end = (scaleend - viewport.x()) / viewport.width();
        QRectF rectangle1 (QPoint(start * width (), 0.005 * height ()), QSize((end-start) * width (), 0.995*height ()));

        painter.save ();
        painter.fillRect (rectangle1,QBrush(QColor(185,211,238,64),Qt::SolidPattern));

        painter.restore ();
    }

    if (scalemode == ScaleY) {
        double start = (scalestart - viewport.y ()) / viewport.height();
        double end = (scaleend - viewport.y()) / viewport.height();
        QRectF rectangle2 (QPoint(0.005 * width (), start * height ()), QSize(0.995 * width (),(end-start) *height ()));

        painter.save ();
        painter.fillRect (rectangle2,QBrush(QColor(185,211,238,64),Qt::SolidPattern));

        painter.restore ();
    }

}

void plot2d::redraw()
{
    this->update();
}

void plot2d::setBoundaries()
{
    // Get extents in data
    foreach(Channel* ch, (*channels))
    {
        if(useExternalBoundaries)
        {
            if(ch->ymax > ext_ymax) ch->ymax = ext_ymax;
            if(ch->ymin < ext_ymin) ch->ymin = ext_ymin;
            if(ch->xmax > ext_xmax) ch->xmax = ext_xmax;
            if(ch->xmin < ext_xmin) ch->xmin = ext_xmin;
        }
        else if(ch->isEnabled())
        {
            int newymin = std::numeric_limits<int>::max();
            int newymax = std::numeric_limits<int>::min();

            QVector<double> data = ch->getData();
            if(data.size() > 0)
            {
                ch->xmax = data.size();
                if(ch->getType() == Channel::steps)
                {
                    ch->ymin = 0;
                }
                else
                {
                    newymin = dsp->min(data)[AMP];
                    if(newymin < ch->ymin) ch->ymin = newymin;
                }
                newymax = dsp->max(data)[AMP];
                if(newymax > ch->ymax) ch->ymax = newymax;
                if(zoomExtendsTrue)
                {
                    ch->ymax = newymax;
                    ch->ymin = newymin;
                }
            }

            //cout << "Bounds: (" << ch->xmin << "," << ch->xmax << ") (" << ch->ymin << "," << ch->ymax << ") " << endl;
            //cout << std::flush;

            data.clear();
        }
    }
}

void plot2d::drawChannels(QPainter &painter)
{
    if(state==1)
    {
        for(unsigned int i = 0; i < this->getNofChannels(); i++)
            if(channels->at(i)->isEnabled())
                drawLogChannel(painter, i);
    }
    else if(state==2)
    {
        for(unsigned int i = 0; i < this->getNofChannels(); i++)
            if(channels->at(i)->isEnabled())
                drawSqrtChannel(painter, i);
    }
    else
        for(unsigned int i = 0; i < this->getNofChannels(); i++)
            if(channels->at(i)->isEnabled())
                drawChannel(painter, i);
}

void plot2d::drawChannel(QPainter &painter, unsigned int id)
{
    Channel *curChan = channels->at(id);
    QVector<double> data = curChan->getData();
    Channel::plotType curType = curChan->getType();
    double nofPoints = data.size();

    //cout << "Drawing ch " << id << " with size " << data.size() << endl;

    if(nofPoints > 0)
    {
        painter.save ();
        painter.setWindow(QRectF (viewport.x () * width (), viewport.y () * height (),
                                  viewport.width () * width (), viewport.height () * height ()).toRect ());
        double max = curChan->ymax;
        double min = curChan->ymin;

        // Move 0,0 to lower left corner
        painter.translate(0,this->height());
        if(curType == Channel::steps)
        {
            min = 0;
        }

        //cout << "Bounds: (" << curChan->xmin << "," << curChan->xmax << ") (" << curChan->ymin << "," << curChan->ymax << ") " << endl;

        QPolygon poly;
        double stepX = (curChan->xmax*1. - curChan->xmin)/(double)(width()/viewport.width());
        if (stepX > 1) { // there are multiple points per pixel
            long lastX = 0;
            int coord = curChan->xmin;
            double dataMin, dataMax, dataFirst, dataLast;
            dataMin = dataMax = dataFirst = dataLast = data [curChan->xmin];

            // draw at most 4 points per picture column: first, min, max, last. That way
            // the lines between pixels are correct and not too much detail gets lost
            for (unsigned int i = curChan->xmin + 1; (i < nofPoints && i < curChan->xmax); ++i) {
                long x = lrint ((i - curChan->xmin) / stepX);
                if (lastX != x) { // begin drawing a new pixel
                    poly.push_back(QPoint (coord, -dataFirst));
                    if (dataLast == dataMin) { // save a point by drawing the min last
                        if (dataMax != dataFirst)
                            poly.push_back (QPoint (coord, -dataMax));
                        if (dataMin != dataMax)
                            poly.push_back (QPoint (coord, -dataMin));
                    } else {
                        if (dataMin != dataFirst)
                            poly.push_back (QPoint (coord, -dataMin));
                        if (dataMax != dataMin)
                            poly.push_back (QPoint (coord, -dataMax));
                        if (dataLast != dataMin)
                            poly.push_back (QPoint (coord, -dataLast));
                    }
                    lastX = x;
                    coord = i;
                    dataMin = dataMax = dataFirst = dataLast = data [i];
                } else { // continue pixel
                    dataLast = data [i];
                    if (dataMin > dataLast)
                        dataMin = dataLast;
                    if (dataMax < dataLast)
                        dataMax = dataLast;
                }
            }

            //finish last pixel
            poly.push_back(QPoint (coord, -dataFirst));
            if (dataMin != dataFirst)
                poly.push_back (QPoint (coord, -dataMin));
            if (dataMax != dataMin)
                poly.push_back (QPoint (coord, -dataMax));
            if (dataLast != dataMin)
                poly.push_back (QPoint (coord, -dataLast));
        } else {
            int lastX = 0;
            int delta = 0;
            int deltaX = 0;
            int lastData = 0;
            for(unsigned int i = curChan->xmin; (i < nofPoints && i < curChan->xmax); i++)
            {
                // Only append point, if it would actually be displayed
                if(abs(data[i]-data[lastX]) > abs(delta))
                {
                    delta = data[i]-data[lastX];
                    deltaX = i;
                    //std::cout << "Delta: " << delta << " , i: " << i << std::endl;
                }
                if((int)(i) >= lastX+1 || i == (curChan->xmax-1))
                {
                     // y-values increase downwards
                     //poly.push_back(QPoint(i,-data[i]));
                     //poly.push_back(QPoint(i+1,-data[i]));
                     lastData += delta;
                     poly.push_back(QPoint(i,-data[deltaX]));
                     poly.push_back(QPoint(i+1,-data[deltaX]));
                     lastX = i;
                     delta=0;
                }
            }
        }

        QPen* pen;
        pen=new QPen(isEnabled () ? curChan->getColor() : Qt::darkGray);
        pen->setCosmetic(true);

        painter.setPen(*pen);
       // painter.drawText(QPoint(0,id*20),tr("%1").arg(id,1,10));

        // Scale and move to display complete signals
        if(max-min < 0.00000001) max++;
        if(curChan->xmax-curChan->xmin < 0.00000001) curChan->xmax++;
        //cout << "Scaling: " << width()/nofPoints << " " << height()/(max-min) << endl;
        painter.scale(width()/(curChan->xmax-curChan->xmin), height()/(max-min));
        painter.translate(0,min);

        painter.drawPolyline(poly);
        //std::cout << "Drew " << std::dec << poly.size() << " points" << std::endl;

        painter.restore ();
    }
}

void plot2d::drawLogChannel(QPainter &painter, unsigned int id)
{
    Channel *curChan = channels->at(id);
    QVector<double> data = curChan->getData();
    Channel::plotType curType = curChan->getType();
    double nofPoints = data.size();

    for(int i=0;i<nofPoints;i++)
        if(data[i]!=0) data[i]=1000*log10(data[i]);

    //cout << "Drawing ch " << id << " with size " << data.size() << endl;

    if(nofPoints > 0)
    {
        painter.save ();
        painter.setWindow(QRectF (viewport.x () * width (), viewport.y () * height (),
                                  viewport.width () * width (), viewport.height () * height ()).toRect ());
        double max;
        if(curChan->ymax!=0) max=1000*log10(curChan->ymax);
        else max=0;
        double min;
        if(curChan->ymin!=0) min=1000*log10(curChan->ymin);
        else min=0;

        // Move 0,0 to lower left corner
        painter.translate(0,this->height());
        if(curType == Channel::steps)
        {
            min = 0;
        }

        //cout << "Bounds: (" << curChan->xmin << "," << curChan->xmax << ") (" << curChan->ymin << "," << curChan->ymax << ") " << endl;

        QPolygon poly;
        double stepX = (curChan->xmax*1. - curChan->xmin)/(double)(width()/viewport.width());
        if (stepX > 1) { // there are multiple points per pixel
            long lastX = 0;
            int coord = curChan->xmin;
            double dataMin, dataMax, dataFirst, dataLast;
            dataMin = dataMax = dataFirst = dataLast = data [curChan->xmin];

            // draw at most 4 points per picture column: first, min, max, last. That way
            // the lines between pixels are correct and not too much detail gets lost
            for (unsigned int i = curChan->xmin + 1; (i < nofPoints && i < curChan->xmax); ++i) {
                long x = lrint ((i - curChan->xmin) / stepX);
                if (lastX != x) { // begin drawing a new pixel
                    poly.push_back(QPoint (coord, -dataFirst));
                    if (dataLast == dataMin) { // save a point by drawing the min last
                        if (dataMax != dataFirst)
                            poly.push_back (QPoint (coord, -dataMax));
                        if (dataMin != dataMax)
                            poly.push_back (QPoint (coord, -dataMin));
                    } else {
                        if (dataMin != dataFirst)
                            poly.push_back (QPoint (coord, -dataMin));
                        if (dataMax != dataMin)
                            poly.push_back (QPoint (coord, -dataMax));
                        if (dataLast != dataMin)
                            poly.push_back (QPoint (coord, -dataLast));
                    }
                    lastX = x;
                    coord = i;
                    dataMin = dataMax = dataFirst = dataLast = data [i];
                } else { // continue pixel
                    dataLast = data [i];
                    if (dataMin > dataLast)
                        dataMin = dataLast;
                    if (dataMax < dataLast)
                        dataMax = dataLast;
                }
            }

            //finish last pixel
            poly.push_back(QPoint (coord, -dataFirst));
            if (dataMin != dataFirst)
                poly.push_back (QPoint (coord, -dataMin));
            if (dataMax != dataMin)
                poly.push_back (QPoint (coord, -dataMax));
            if (dataLast != dataMin)
                poly.push_back (QPoint (coord, -dataLast));
        } else {
            int lastX = 0;
            int delta = 0;
            int deltaX = 0;
            int lastData = 0;
            for(unsigned int i = curChan->xmin; (i < nofPoints && i < curChan->xmax); i++)
            {
                // Only append point, if it would actually be displayed
                if(abs(data[i]-data[lastX]) > abs(delta))
                {
                    delta = data[i]-data[lastX];
                    deltaX = i;
                    //std::cout << "Delta: " << delta << " , i: " << i << std::endl;
                }
                if((int)(i) >= lastX+1 || i == (curChan->xmax-1))
                {
                     // y-values increase downwards
                     //poly.push_back(QPoint(i,-data[i]));
                     //poly.push_back(QPoint(i+1,-data[i]));
                     lastData += delta;
                     poly.push_back(QPoint(i,-data[deltaX]));
                     poly.push_back(QPoint(i+1,-data[deltaX]));
                     lastX = i;
                     delta=0;
                }
            }
        }

        QPen* pen;
        pen=new QPen(isEnabled () ? curChan->getColor() : Qt::darkGray);
        pen->setCosmetic(true);

        painter.setPen(*pen);
       // painter.drawText(QPoint(0,id*20),tr("%1").arg(id,1,10));

        // Scale and move to display complete signals
        if(max-min < 0.00000001) max++;
        if(curChan->xmax-curChan->xmin < 0.00000001) curChan->xmax++;
        //cout << "Scaling: " << width()/nofPoints << " " << height()/(max-min) << endl;
        painter.scale(width()/(curChan->xmax-curChan->xmin),height()/(max-min));
        painter.translate(0,min);

        painter.drawPolyline(poly);
        //std::cout << "Drew " << std::dec << poly.size() << " points" << std::endl;

        painter.restore ();
    }
}

void plot2d::drawSqrtChannel(QPainter &painter, unsigned int id)
{
    Channel *curChan = channels->at(id);
    QVector<double> data = curChan->getData();
    Channel::plotType curType = curChan->getType();
    double nofPoints = data.size();

    for(int i=0;i<nofPoints;i++)
        if(data[i]!=0) data[i]=1000*sqrt(data[i]);

    //cout << "Drawing ch " << id << " with size " << data.size() << endl;

    if(nofPoints > 0)
    {
        painter.save ();
        painter.setWindow(QRectF (viewport.x () * width (), viewport.y () * height (),
                                  viewport.width () * width (), viewport.height () * height ()).toRect ());
        double max;
        if(curChan->ymax!=0) max=1000*sqrt(curChan->ymax);
        else max=0;
        double min;
        if(curChan->ymin!=0) min=1000*sqrt(curChan->ymin);
        else min=0;

        // Move 0,0 to lower left corner
        painter.translate(0,this->height());
        if(curType == Channel::steps)
        {
            min = 0;
        }

        //cout << "Bounds: (" << curChan->xmin << "," << curChan->xmax << ") (" << curChan->ymin << "," << curChan->ymax << ") " << endl;

        QPolygon poly;
        double stepX = (curChan->xmax*1. - curChan->xmin)/(double)(width()/viewport.width());
        if (stepX > 1) { // there are multiple points per pixel
            long lastX = 0;
            int coord = curChan->xmin;
            double dataMin, dataMax, dataFirst, dataLast;
            dataMin = dataMax = dataFirst = dataLast = data [curChan->xmin];

            // draw at most 4 points per picture column: first, min, max, last. That way
            // the lines between pixels are correct and not too much detail gets lost
            for (unsigned int i = curChan->xmin + 1; (i < nofPoints && i < curChan->xmax); ++i) {
                long x = lrint ((i - curChan->xmin) / stepX);
                if (lastX != x) { // begin drawing a new pixel
                    poly.push_back(QPoint (coord, -dataFirst));
                    if (dataLast == dataMin) { // save a point by drawing the min last
                        if (dataMax != dataFirst)
                            poly.push_back (QPoint (coord, -dataMax));
                        if (dataMin != dataMax)
                            poly.push_back (QPoint (coord, -dataMin));
                    } else {
                        if (dataMin != dataFirst)
                            poly.push_back (QPoint (coord, -dataMin));
                        if (dataMax != dataMin)
                            poly.push_back (QPoint (coord, -dataMax));
                        if (dataLast != dataMin)
                            poly.push_back (QPoint (coord, -dataLast));
                    }
                    lastX = x;
                    coord = i;
                    dataMin = dataMax = dataFirst = dataLast = data [i];
                } else { // continue pixel
                    dataLast = data [i];
                    if (dataMin > dataLast)
                        dataMin = dataLast;
                    if (dataMax < dataLast)
                        dataMax = dataLast;
                }
            }

            //finish last pixel
            poly.push_back(QPoint (coord, -dataFirst));
            if (dataMin != dataFirst)
                poly.push_back (QPoint (coord, -dataMin));
            if (dataMax != dataMin)
                poly.push_back (QPoint (coord, -dataMax));
            if (dataLast != dataMin)
                poly.push_back (QPoint (coord, -dataLast));
        } else {
            int lastX = 0;
            int delta = 0;
            int deltaX = 0;
            int lastData = 0;
            for(unsigned int i = curChan->xmin; (i < nofPoints && i < curChan->xmax); i++)
            {
                // Only append point, if it would actually be displayed
                if(abs(data[i]-data[lastX]) > abs(delta))
                {
                    delta = data[i]-data[lastX];
                    deltaX = i;
                    //std::cout << "Delta: " << delta << " , i: " << i << std::endl;
                }
                if((int)(i) >= lastX+1 || i == (curChan->xmax-1))
                {
                     // y-values increase downwards
                     //poly.push_back(QPoint(i,-data[i]));
                     //poly.push_back(QPoint(i+1,-data[i]));
                     lastData += delta;
                     poly.push_back(QPoint(i,-data[deltaX]));
                     poly.push_back(QPoint(i+1,-data[deltaX]));
                     lastX = i;
                     delta=0;
                }
            }
        }

        QPen* pen;
        pen=new QPen(isEnabled () ? curChan->getColor() : Qt::darkGray);
        pen->setCosmetic(true);

        painter.setPen(*pen);
       // painter.drawText(QPoint(0,id*20),tr("%1").arg(id,1,10));

        // Scale and move to display complete signals
        if(max-min < 0.00000001) max++;
        if(curChan->xmax-curChan->xmin < 0.00000001) curChan->xmax++;
        //cout << "Scaling: " << width()/nofPoints << " " << height()/(max-min) << endl;
        painter.scale(width()/(curChan->xmax-curChan->xmin),height()/(max-min));
        painter.translate(0,min);

        painter.drawPolyline(poly);
        //std::cout << "Drew " << std::dec << poly.size() << " points" << std::endl;

        painter.restore ();
    }
}

void plot2d::setCalibration(double value2)
{
    calibration=1;
    calibCoef1=value2;
}

void plot2d::setPlotState(int receivedState)
{
    state=receivedState;
}

void plot2d::drawTicks(QPainter &painter)
{
    int ch = curTickCh;
    long i=0, value=0;
    long incx=0, incy=0;

    double chxmin = channels->at(ch)->xmin;
    double chxmax = channels->at(ch)->xmax;
    double chymin = channels->at(ch)->ymin;
    double chymax = channels->at(ch)->ymax;

    double xmin = (chxmax - chxmin) * viewport.left ();
    double xmax = (chxmax - chxmin) * viewport.right ();
    double ymin = (chymax - chymin) * (1 - viewport.bottom ());
    double ymax = (chymax - chymin) * (1 - viewport.top ());

    // Draw tickmarks around the border

    // Range chooser
    while(incx<50 && incx<(xmax-xmin)/10)
    {
        incx+=10;
    }
    while(incx<10000 && incx<(xmax-xmin)/10)
    {
        incx+=50;
    }
    // for large ranges the increment is the smallest multiple of 10000 larger than
    // or equal to one tenth of the x-range
    if (incx < (xmax-xmin)/10)
        incx = 10000 * (floor ((xmax-xmin)/(10*10000)) + 1);

    while(incy<10 && incy<(ymax-ymin)/5)
    {
        incy+=1;
    }
    while(incy<50 && incy<(ymax-ymin)/5)
    {
        incy+=10;
    }
    while(incy<10000 && incy<(ymax-ymin)/5)
    {
        incy+=50;
    }

    // for large ranges the increment is the smallest multiple of 10000 larger than
    // or equal to one fifth of the y-range
    if (incy < (ymax-ymin)/5)
        incy = 10000 * (floor ((ymax-ymin)/(5*10000)) + 1);

    if(incx == 0) incx = 1;
    if(incy == 0) incy = 1;

    painter.save ();
    painter.setPen(QPen(isEnabled () ? channels->at(ch)->getColor() : Qt::darkGray));

    // x Ticks
    value=xmin;

    int xtickInc = 0;
    if(xmax-xmin <= 1)
    {
        xtickInc = width();
    }
    else
    {
        xtickInc = (incx*width()/(xmax-xmin));
    }

    for(i=0; i<width(); i+=xtickInc)
    {
        QLine line1(i,0,i,height()*0.01);
        painter.drawLine(line1);
    }

    // y Ticks
    value=ymin;

    int ytickInc = 0;
    if(ymax-ymin <= 1)
    {
        ytickInc = height();
    }
    else
    {
        ytickInc = (incy*height()/(ymax-ymin));
    }

    if(ymax-ymin < 0.000001) ymax += 1;
    for(i=height(); i>0; i-=ytickInc)
    {
        //std::cout << "Drawing y tick " << i << std::endl;
        QLine line1(0,i,width()*0.01,i);
        QLine line2(width(),i,width()*0.99,i);
        painter.drawLine(line1);
        painter.drawLine(line2);
        painter.drawText(width()-40,i-6,tr("%1").arg(value,5,10));
        painter.drawText(0,i-6,tr("%1").arg(value,5,10));
        value+=incy;
    }

    painter.setPen(QColor(159,182,205,255));

    for(uint i=0;i<maximaList.size();i++)
    {
        int position = width()*((maximaList[i]/(curxmax-curxmin))-viewport.x())/viewport.width();
        QLine line1(position,height()*(1-(dataVector[maximaList[i]]/(ymax-ymin)-0.005)),position,height()*(1-(dataVector[maximaList[i]]/(ymax-ymin)+0.02)));
        painter.drawLine(line1);
        painter.drawText(position-15,height()*(1-(dataVector[maximaList[i]]/(ymax-ymin)+0.05)),tr("%1").arg(maximaList[i],5,10));
    }

    painter.restore ();
}

void plot2d::drawLogTicks(QPainter &painter)
{
    int ch = curTickCh;
    long i=0;
    long incx=0;

    double value=0, incy=0;

    double chxmin = channels->at(ch)->xmin;
    double chxmax = channels->at(ch)->xmax;
    double chymin = channels->at(ch)->ymin;
    double chymax = channels->at(ch)->ymax;

    double xmin = (chxmax - chxmin) * viewport.left ();
    double xmax = (chxmax - chxmin) * viewport.right ();
    double ymin = pow(10,(1 - viewport.bottom ())*log10(chymax - chymin));
    if(ymin>=1&&ymin<2) ymin=0;
    double ymax = pow(10,(1 - viewport.top ())*log10(chymax - chymin));
    // Draw tickmarks around the border

    // Range chooser
    while(incx<50 && incx<(xmax-xmin)/10)
    {
        incx+=10;
    }
    while(incx<10000 && incx<(xmax-xmin)/10)
    {
        incx+=50;
    }
    // for large ranges the increment is the smallest multiple of 10000 larger than
    // or equal to one tenth of the x-range
    if (incx < (xmax-xmin)/10)
        incx = 10000 * (floor ((xmax-xmin)/(10*10000)) + 1);

    while(incy<10 && incy<pow(ymax-ymin,0.2))
    {
        incy+=1;
    }
    while(incy<50 && incy<pow(ymax-ymin,0.2))
    {
        incy+=10;
    }
    while(incy<10000 && incy<pow(ymax-ymin,0.2))
    {
        incy+=50;
    }

    if(incx == 0) incx = 1;
    if(incy == 0) incy = 1;

    painter.save ();
    painter.setPen(QPen(isEnabled () ? channels->at(ch)->getColor() : Qt::darkGray));

    // x Ticks
    value=xmin;

    int xtickInc = 0;
    if(xmax-xmin <= 1)
    {
        xtickInc = width();
    }
    else
    {
        xtickInc = (incx*width()/(xmax-xmin));
    }

    for(i=0; i<width(); i+=xtickInc)
    {
        QLine line1(i,0,i,height()*0.01);
        painter.drawLine(line1);
    }

    // y Ticks
    value=0;

    int ytickInc = 0;
    if(ymax-ymin <= 1)
    {
        ytickInc = height();
    }
    else
    {
        ytickInc = (log10(incy)*height()/log10(ymax-ymin));
    }

    if(ymax-ymin < 0.000001) ymax += 1;
    for(i=height(); i>0; i-=ytickInc)
    {
        //std::cout << "Drawing y tick " << i << std::endl;
        QLine line1(0,i,width()*0.01,i);
        QLine line2(width(),i,width()*0.99,i);
        painter.drawLine(line1);
        painter.drawLine(line2);
        if((i==height())&&viewport.bottom()>0.99)
        {
            painter.drawText(width()-40,i-6,tr("%1").arg(0,5,10));
            painter.drawText(0,i-6,tr("%1").arg(0,5,10));
        }
        else
        {
            painter.drawText(width()-40,i-6,tr("%1").arg((int)(ymin+pow(incy,value)),5,10));
            painter.drawText(0,i-6,tr("%1").arg((int)(ymin+pow(incy,value)),5,10));
        }
        value+=1;
    }

    painter.restore ();
}

void plot2d::drawSqrtTicks(QPainter &painter)
{
    int ch = curTickCh;
    long i=0;
    long incx=0;

    double value=0, incy=0;

    double chxmin = channels->at(ch)->xmin;
    double chxmax = channels->at(ch)->xmax;
    double chymin = channels->at(ch)->ymin;
    double chymax = channels->at(ch)->ymax;

    double xmin = (chxmax - chxmin) * viewport.left ();
    double xmax = (chxmax - chxmin) * viewport.right ();
    double ymin = pow((1 - viewport.bottom ())*sqrt((chymax - chymin)),2);
    double ymax = pow((1 - viewport.top ())*sqrt((chymax - chymin)),2);
    // Draw tickmarks around the border

    // Range chooser
    while(incx<50 && incx<(xmax-xmin)/10)
    {
        incx+=10;
    }
    while(incx<10000 && incx<(xmax-xmin)/10)
    {
        incx+=50;
    }
    // for large ranges the increment is the smallest multiple of 10000 larger than
    // or equal to one tenth of the x-range
    if (incx < (xmax-xmin)/10)
        incx = 10000 * (floor ((xmax-xmin)/(10*10000)) + 1);

    while(incy<10 && incy<(ymax-ymin)/25)
    {
        incy+=1;
    }
    while(incy<50 && incy<(ymax-ymin)/25)
    {
        incy+=10;
    }
    while(incy<10000 && incy<(ymax-ymin)/25)
    {
        incy+=50;
    }

    // for large ranges the increment is the smallest multiple of 10000 larger than
    // or equal to one fifth of the y-range
    //if (incy < pow(1/5,ymax-ymin))
    //    incy = 10000 * (floor ((ymax-ymin)/(5*10000)) + 1);

    if(incx == 0) incx = 1;
    if(incy == 0) incy = 1;

    painter.save ();
    painter.setPen(QPen(isEnabled () ? channels->at(ch)->getColor() : Qt::darkGray));

    // x Ticks
    value=xmin;

    int xtickInc = 0;
    if(xmax-xmin <= 1)
    {
        xtickInc = width();
    }
    else
    {
        xtickInc = (incx*width()/(xmax-xmin));
    }

    for(i=0; i<width(); i+=xtickInc)
    {
        QLine line1(i,0,i,height()*0.01);
        painter.drawLine(line1);
    }

    // y Ticks
    value=0;

    int ytickInc = 0;
    if(ymax-ymin <= 1)
    {
        ytickInc = height();
    }
    else
    {
        ytickInc = (sqrt(incy)*height()/sqrt(ymax-ymin));
    }

    if(ymax-ymin < 0.000001) ymax += 1;
    for(i=height(); i>0; i-=ytickInc)
    {
        //std::cout << "Drawing y tick " << i << std::endl;
        QLine line1(0,i,width()*0.01,i);
        QLine line2(width(),i,width()*0.99,i);
        painter.drawLine(line1);
        painter.drawLine(line2);
        if((i==height())&&viewport.bottom()>0.99)
        {
            painter.drawText(width()-40,i-6,tr("%1").arg(0,5,10));
            painter.drawText(0,i-6,tr("%1").arg(0,5,10));
        }
        else
        {
            painter.drawText(width()-40,i-6,tr("%1").arg((int)(ymin+pow(value*sqrt(incy),2)),5,10));
            painter.drawText(0,i-6,tr("%1").arg((int)(ymin+pow(value*sqrt(incy),2)),5,10));
        }
        value+=1;
    }

    painter.restore ();
}

void plot2d::createActions()
{
    clearHistogramAction = new QAction(tr("Clear Histogram"), this);
    connect(clearHistogramAction, SIGNAL(triggered()), this, SLOT(clearHistogram()));
    saveChannelAction = new QAction(tr("Save histogram"), this);
    connect(saveChannelAction, SIGNAL(triggered()), this, SLOT(saveChannel()));
    sameZoomAll = new QAction(tr("Same zoom for all"),this);
    connect(sameZoomAll, SIGNAL(triggered()), this, SLOT(emitZoomExtends()));
    unzoomAll = new QAction(tr("Unzoom all"),this);
    connect(unzoomAll, SIGNAL(triggered()), this, SLOT(emitUnzoomAll()));
}

void plot2d::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(this->clearHistogramAction);
    menu.addAction(this->saveChannelAction);
    menu.addAction(this->sameZoomAll);
    menu.addAction(this->unzoomAll);
    menu.exec(event->globalPos());

    /*QMenu* sub = menu.addMenu("Axes for");
    sub->addActions(setCurTickChActions);
    QAction* curAct =
    foreach(QAction* act, setCurTickChActions)
    {
        if(curAct == act)
        {
            selectCurTickCh(act->data().value<int>());
        }
    }*/
}

void plot2d::emitZoomExtends()
{
    emit this->changeZoomForAll(this->id,viewport.x(),viewport.width());
}

void plot2d::emitUnzoomAll()
{
    emit this->changeZoomForAll(this->id,0,1);
}

void plot2d::clearHistogram()
{
    for(unsigned int i = 0; i < this->getNofChannels(); i++)
    {
        emit this->histogramCleared(channels->at(i)->getId(),this->id);
    }
}

void plot2d::setMaximumExtends(int _xmin, int _xmax, int _ymin, int _ymax)
{
    ext_xmin = _xmin;
    ext_xmax = _xmax;
    ext_ymin = _ymin;
    ext_ymax = _ymax;
}

void plot2d::toggleExternalBoundaries(bool newValue)
{
    useExternalBoundaries = newValue;
}

void plot2d::maximumFinder()
{
    smoothed.resize(8192);
    derivative.resize(8192);
    dataVector = channels->at(0)->getData();
    smoothData();
    findMaxima();
    channelUpdate();
}

void plot2d::smoothData()
{
    smoothed[0]=dataVector[0];
    smoothed[1]=(dataVector[0]+2*dataVector[1]+dataVector[2])/4;
    for(int i=2;i<8190;i++)
    {
        smoothed[i]=dataVector[i-2]+2*dataVector[i-1]+3*dataVector[i]+2*dataVector[i+1]+dataVector[i+2];
        smoothed[i]=smoothed[i]/9;
    }
    smoothed[8190]=(dataVector[8189]+2*dataVector[8190]+dataVector[8191])/4;
    smoothed[8191]=dataVector[8191];
}

void plot2d::calculateDerivative()
{
    for(int i=1;i<8192;i++)
    {
        derivative[i]=smoothed[i+1]-smoothed[i-1];
        derivative[i]=derivative[i]/2;
    }
}

void plot2d::findMaxima()
{
    double ratio;
    std::vector <int> extremaList;
    calculateDerivative();
    for(int i=2;i<8192;i++)
        if(derivative[i-2]*derivative[i-1]*derivative[i]*derivative[i+1]<0)
            if(dataVector[i]>100)
                    extremaList.push_back(i);

    for(unsigned int i=0;i<extremaList.size()-1;i++)
        if(extremaList[i+1]-extremaList[i]<10)
        {
            if(extremaList[i]>extremaList[i+1])
                extremaList.erase(extremaList.begin()+i+1);
            else {
                extremaList.erase(extremaList.begin()+i);
                i--;
            }
        }

    for(unsigned int i=1;i<extremaList.size()-1;i++)
    {
        int minmax;
        if(dataVector[extremaList[i-1]]>dataVector[extremaList[i+1]])
            minmax=dataVector[extremaList[i-1]];
        else minmax=dataVector[extremaList[i+1]];
        ratio=(double)(dataVector[extremaList[i]]/minmax);
        if(ratio>1.7)
            maximaList.push_back(extremaList[i]);
    }

    for(unsigned int i=0;i<maximaList.size();i++)
        std::cout<<maximaList[i]<<" "<<smoothed[maximaList[i]]<<std::endl;
}

void plot2d::zoomExtends(bool newValue)
{
    zoomExtendsTrue = newValue;
}

void plot2d::selectCurTickCh(int _curTickCh)
{
    curTickCh = _curTickCh;
}

void plot2d::resetBoundaries(int ch)
{
    channels->at(ch)->xmin = 0;
    channels->at(ch)->xmax = 1;
    channels->at(ch)->ymin = 0;
    channels->at(ch)->ymax = 1;
}

void plot2d::saveChannel()
{
    QString fileName = QFileDialog::getSaveFileName(this,"Save Histogram as...","","Data files (*.dat)");
    if (fileName.isEmpty())
        return;
    QVector<double> data = channels->first()->getData();
    dsp->vectorToFile(data,fileName.toStdString());
}
