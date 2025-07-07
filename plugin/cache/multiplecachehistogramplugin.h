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

#ifndef MULTIPLECACHEHISTOGRAMPLUGIN_H
#define MULTIPLECACHEHISTOGRAMPLUGIN_H

#include "baseplugin.h"
#include "pluginconnectorqueued.h"
#include <QTimer>
#include <QGridLayout>
#include <QWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <vector>
#include <QVector>



#include "pluginmanager.h"
#include "runmanager.h"
#include "math.h"
#include <QFont>
#include <QStackedWidget>
#include <QLabel>
#include <QSettings>


#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFile>
#include <QDateTime>
#include <string>
#include <algorithm>

#include "baseplugin.h"
#include "plot2d.h"

class QComboBox;
class BasePlugin;
class QLabel;

struct MultipleCacheHistogramPluginConfig
{
    int nofBins, nofTBins, nofSBins, updateInterval;
    QString calName, graphName;

    MultipleCacheHistogramPluginConfig()
        : nofBins(8192),nofTBins(8192),nofSBins(8192), updateInterval(1)
    {}
};

class MultipleCacheHistogramPlugin : public BasePlugin
{
    Q_OBJECT

protected:
    Attributes attribs_;
    double binWidth;
    bool scheduleReset;
    bool calibrated;
    int ninputs;
    bool BGOVeto;
    int extraPanels;
    bool vetoExtra;
    std::vector <plot2d*> mplot;
    QTimer* updateTimer;
    QWidget* Multiplewindow;
    MultipleCacheHistogramPluginConfig conf;
    std::vector <std::vector <double> > calibcoef;
    QLabel* calibName;
    QLabel* graphanaName;
    QSpinBox* updateSpeedSpinner;
    QComboBox* nofBinsBox;
    QComboBox* nofTBinsBox;
    QComboBox* nofSBinsBox;
    std::vector < QVector<double> > totalCache;
    QPushButton* resetButton;
    QPushButton* previewButton;
    std::vector <QLabel*> plotCountsLabel;
    std::vector <QWidget*> windowVector;
    std::vector <QGridLayout*> multipleLayoutVector;

    virtual void createSettings(QGridLayout*);



    QLabel* nofInputsLabel;


    std::vector < QVector<double> > cache;
    std::vector < QVector<double> > scache;
    std::vector < QVector<double> > rawcache;
    QLabel* totalCountsLabel;


    QLabel* numCountsLabel;
    QLabel* calibLabel;
    std::ifstream calibration;

    QVector< QVector<uint32_t> > vetoData;
    QVector< QVector<uint32_t> > secondTimeData;

    QPushButton* calibNameButton;
    int secondTimePosition;

    QPushButton* graphanaNameButton;

    void setCalibName(QString);
    void setGraphanaName(QString);
    uint64_t nofCounts;
    QVector <uint64_t> plotCounts;
    QVector <uint64_t> prevCount;
    QVector <uint64_t> evRate;


public:
    MultipleCacheHistogramPlugin(int _id, QString _name, const Attributes &attrs);
    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new MultipleCacheHistogramPlugin (id, name, attrs);
    }

    Attributes getAttributes () const;
    static AttributeMap getMCHPAttributeMap ();
    virtual ~MultipleCacheHistogramPlugin();
    void recalculateBinWidth();
    virtual void applySettings(QSettings*);
    virtual void saveSettings(QSettings*);




    virtual AbstractPlugin::Group getPluginGroup () { return AbstractPlugin::GroupCache; }
    virtual void userProcess();
    virtual void runStartingEvent();
    uint32_t nofInputs;
    QVector <int> readPointer;
    bool rawCheck;
    bool cleanCheck;

private:



    std::ofstream rates;
    std::ofstream spectra;

    std::ofstream checker;


public slots:
    void updateVisuals();


    void nofBinsChanged(int);
    void nofTBinsChanged(int);
    void nofSBinsChanged(int);
    void calibNameButtonClicked();
    void graphanaNameButtonClicked();
    void previewButtonClicked();
    void findCalibName(QString);
    bool notVeto(uint32_t, int);
    void writeTimeOnly(int32_t,int);
    void writeEnergyOnly(int32_t,int);
    void modifyPlotState(int);
    virtual void resetButtonClicked();
    void setTimerTimeout(int msecs);
    void resetSingleHistogram(unsigned int,unsigned int);
    void changeBlockZoom(unsigned int, double, double);

signals:
    void stopUpdateTimer();
    void startUpdateTimer(int);

private slots:
    void stoppingUpdateTimer();
    void startingUpdateTimer(int);
};

#endif // MultipleCACHEHISTOGRAMPLUGIN_H
