#ifndef MATRIXPLOTWALK_H
#define MATRIXPLOTWALK_H
#include "2Ddisplay.h"
#include "baseplugin.h"
#include "pluginmanager.h"
#include "runmanager.h"
#include "pluginconnectorqueued.h"
#include <QPushButton>
#include <QMainWindow>
#include "fstream"

namespace Ui {
class MatrixPlotWalk;
}

struct MatrixPlotWalkPluginConfig
{
    int nofBins, nofTBins, nofSBins;
    QString calName;

    MatrixPlotWalkPluginConfig()
        : nofBins(8192),nofTBins(8192),nofSBins(8192)
    {}
};

class MatrixPlotWalk: public BasePlugin
{
    Q_OBJECT

protected:
    Attributes attribs_;

    QPushButton* matrixButton;

    virtual void createSettings(QGridLayout*);

public:
    MatrixPlotWalk(int _id, QString _name, const Attributes &attrs);
    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new MatrixPlotWalk (id, name, attrs);
    }

    Attributes getAttributes () const;
    static AttributeMap getMCHPAttributeMap ();

    virtual void applySettings(QSettings*);
    virtual void saveSettings(QSettings*);
    virtual void userProcess();
    virtual void runStartingEvent();

    ~MatrixPlotWalk();

    QVector<QVector<uint32_t> > data;
    QVector<qint32> readPointer;

private slots:
    void on_display_2d_clicked();
    void updateVisuals();
    void refMinChanged();
    void refMaxChanged();
    void calibButtonClicked();
    void clearButtonClicked();

private:
    QVector<uint32_t> refEnergy;
    QVector<uint32_t> refTime;
    QVector<uint32_t> energy;
    QVector<uint32_t> time;

    u_int32_t noCounts;
    QLabel* noCounter;
    QTimer* secondTimer;

    QSpinBox* minRef;
    QSpinBox* maxRef;
    uint32_t minRefValue;
    uint32_t maxRefValue;
    D2Display *myDisplay;
    bool stopAq;

    QLabel* calibLabel;
    QPushButton* calibButton;
    QPushButton* clearButton;
    QString calName;
    void getCalib();
    std::vector <std::vector <double> > calibcoef;
//    plot2d* plot1DE;
//    plot2d* plot1DT;

    bool checkPointers();
    bool checkStamps(int32_t,int32_t,int32_t,int32_t);
    QVector<bool> stampsChecked;
};

#endif // MAINWINDOW_H
