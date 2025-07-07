#ifndef TENSIONER_H
#define TENSIONER_H

#include <vector>
#include <iostream>
#include <fstream>
#include "baseplugin.h"
#include <algorithm>
#include "plot2d.h"
#include <QGridLayout>
#include <QVector>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

class BasePlugin;

struct TensionerConfig
{
    QString dataName;
    int nrChannels;

    TensionerConfig()
        :nrChannels(8192)
    {}
};

class Tensioner : public BasePlugin
{
    Q_OBJECT

protected:
virtual void createSettings(QGridLayout*);
plot2d* tensionerPlot;
QPushButton* dataNameButton;
QPushButton* maximaFinderButton;
QLineEdit* dataNameEdit;
void setDataName(QString);
TensionerConfig conf;

public slots:
void dataNameButtonClicked();
void readData(QString);
void findMaxima();

public:
Attributes attribs_;
QVector<double> data;

    virtual void userProcess();
    virtual void applySettings(QSettings*);
    virtual void saveSettings(QSettings*);
Tensioner (int _id, QString _name, const Attributes &_attrs);
    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new Tensioner (id, name, attrs);
    }

static AttributeMap getTensionerAttributeMap ();
};

#endif // TENSIONER_H
