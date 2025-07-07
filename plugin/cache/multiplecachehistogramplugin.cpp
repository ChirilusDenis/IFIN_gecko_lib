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
#include "multiplecachehistogramplugin.h"

static PluginRegistrar registrar ("Multiplecachehistogramplugin", MultipleCacheHistogramPlugin::create, AbstractPlugin::GroupCache, MultipleCacheHistogramPlugin::getMCHPAttributeMap());

MultipleCacheHistogramPlugin::MultipleCacheHistogramPlugin(int _id, QString _name, const Attributes &_attrs)
    : BasePlugin(_id, _name),
    attribs_ (_attrs),
    binWidth(1),
    scheduleReset(true),
    calibrated(0)
{
    createSettings(settingsLayout);

    //Checking the number of inputs is valid.
    if (ninputs <= 0 || ninputs > 256) {
        ninputs = 256;
        std::cout << _name.toStdString () << ": nofInputs invalid. Setting to 256." << std::endl;
    }

    //Creating the standard inputs
    for(int n=0; n<ninputs; n++)
    {
        addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("in %1").arg(n)));
    }

    //Creating more sets of inputs if the user requests to have a more input panels
    if(extraPanels)
    {
        for(int n=0; n<extraPanels*ninputs/2; n++)
        {
            addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Extra inputs %1").arg(n)));
        }
    }

    //Creating another set of inputs if the user requests to veto the data with the BGOs
    if(BGOVeto)
    {
        for(int n=0; n<ninputs/2; n++)
        {
            addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Veto %1").arg(n)));
        }
    }

    //The plugin needs only one input to have data in order to run
    setNumberOfMandatoryInputs(1);

    //Creating the standard set of plots
    for(int i=0;i<(extraPanels+4)*ninputs/2+2;i++)
    {
        mplot[i]->addChannel(0,tr("histogram"),QVector<double>(1,0),
                     QColor(153,153,153),Channel::steps,1);
    }

    //Starting a timer that updates all the visuals. It takes a ms value, thus the *1000
    updateTimer = new QTimer();
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateVisuals()));

    connect(this,SIGNAL(stopUpdateTimer()),this,SLOT(stoppingUpdateTimer()));
    connect(this,SIGNAL(startUpdateTimer(int)),this,SLOT(startingUpdateTimer(int)));

    std::cout << "Instantiated MultipleCacheHistogram" << std::endl;
}

MultipleCacheHistogramPlugin::~MultipleCacheHistogramPlugin()
{
   //Destructor of the class. Deallocates the plots and the plot window.
   for(unsigned int i=0;i<mplot.size();i++)
   {
       mplot[i]->close();
       delete mplot[i];
       mplot[i]=NULL;
   }
   Multiplewindow->close();
   delete Multiplewindow;
   Multiplewindow=NULL;
}

AbstractPlugin::AttributeMap MultipleCacheHistogramPlugin::getMCHPAttributeMap() {
    //These are the initial settings with which the plugin is created
    AbstractPlugin::AttributeMap attrs;
    attrs.insert ("nofInputs", QVariant::Int);
    attrs.insert ("BGO Veto",  QVariant::Bool);
    attrs.insert ("Extra input sets", QVariant::Int);
    attrs.insert ("Veto extra inputs", QVariant::Bool);
    return attrs;
}

AbstractPlugin::Attributes MultipleCacheHistogramPlugin::getAttributes () const {
    return attribs_;
}

void MultipleCacheHistogramPlugin::startingUpdateTimer(int value)
{
    updateTimer->start(value);
}

void MultipleCacheHistogramPlugin::stoppingUpdateTimer()
{
    updateTimer->stop();
}

void MultipleCacheHistogramPlugin::applySettings(QSettings* settings)
{
    //Reading the settings from the save file and applying them to the interface elements, which also prompts their registering in the variables.

    QString set;
    settings->beginGroup(getName());
        set = "nofBins"; if(settings->contains(set)) conf.nofBins = settings->value(set).toInt();
        set = "nofTBins"; if(settings->contains(set)) conf.nofTBins = settings->value(set).toInt();
        if(extraPanels)
        {
            set = "nofSBins"; if(settings->contains(set)) conf.nofSBins = settings->value(set).toInt();
        }
        set = "calName"; if(settings->contains(set)) conf.calName = settings->value(set).toString();
        set = "graphName";  if(settings->contains(set)) conf.graphName = settings->value(set).toString();
        set = "updateInterval"; if(settings->contains(set)) conf.updateInterval = settings->value(set).toInt();
        settings->endGroup();

    calibName->setText(conf.calName);
    graphanaName->setText(conf.graphName);
    updateSpeedSpinner->setValue(conf.updateInterval);
    nofBinsBox->setCurrentIndex(nofBinsBox->findData(conf.nofBins,Qt::UserRole));
    nofTBinsBox->setCurrentIndex(nofTBinsBox->findData(conf.nofTBins,Qt::UserRole));
    if(extraPanels)
    {
        nofSBinsBox->setCurrentIndex(nofSBinsBox->findData(conf.nofSBins,Qt::UserRole));
    }
}

void MultipleCacheHistogramPlugin::saveSettings(QSettings* settings)
{
    //Saving the settings to file
    if(settings == NULL)
    {
        std::cout << getName().toStdString() << ": no settings file" << std::endl;
        return;
    }
    else
    {
        std::cout << getName().toStdString() << " saving settings...";
        settings->beginGroup(getName());
            settings->setValue("nofBins",conf.nofBins);
            settings->setValue("nofTBins",conf.nofTBins);
            if(extraPanels)
            {
                settings->setValue("nofSBins",conf.nofSBins);
            }
            settings->setValue("calName",conf.calName);
            settings->setValue("graphName",conf.graphName);
            settings->setValue("updateInterval",conf.updateInterval);
            settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void MultipleCacheHistogramPlugin::createSettings(QGridLayout * l)
{
    //read the attributes set on creating the plugin
    Attributes _attrib=getAttributes();
    ninputs=_attrib.value ("nofInputs", QVariant (4)).toInt ();
    BGOVeto=_attrib.value ("BGO Veto", QVariant (4)).toBool ();
    extraPanels=_attrib.value ("Extra input sets",QVariant (4)).toInt ();
    vetoExtra=_attrib.value ("Veto extra inputs",QVariant (4)).toBool ();

    //Setting the number of columns on each row. If there are less than 5 detectors, that is the number set. If there are more than 5, there will be 5 columns
    int rowcol=(ninputs/2-1)/5+1;

    //Creating the two total sum histograms
    totalCache.resize(2);

    //Resizing the plots, histograms and counters vectors
    mplot.resize((4+2*extraPanels)*ninputs/2+2);
    cache.resize((2+extraPanels)*ninputs/2);
    rawcache.resize((2+extraPanels)*ninputs/2);
    plotCounts.resize((4+2*extraPanels)*ninputs/2);
    prevCount.resize((4+2*extraPanels)*ninputs/2);
    evRate.resize((4+extraPanels)*ninputs/2);
    plotCountsLabel.resize((4+2*extraPanels)*ninputs/2);

    //Resizing the vectors for the calibration coefficients
    calibcoef.resize(ninputs/2);

    //Ionitializing the calibration coefficients
    for(int i=0;i<ninputs/2;i++)
    {
        calibcoef[i].resize(3);
        calibcoef[i][0]=2;
        calibcoef[i][1]=0;
        calibcoef[i][2]=1;
    }

    // Plugin specific code here
    QWidget* container = new QWidget();
    {
        QGridLayout* cl = new QGridLayout;

        //Creating preview and reset buttons and setting fonts
        previewButton = new QPushButton(tr("Show"));
        previewButton->setFont(QFont("Helvetica", 10, QFont::Bold));

        resetButton = new QPushButton("Clear Spectra");
        resetButton->setFont(QFont("Helvetica", 10, QFont::Bold));

        //initializing the Plot Counts Labels
        for(int i=0;i<(4+2*extraPanels)*ninputs/2;i++)
        {
            plotCountsLabel[i] = new QLabel (tr ("0"));
            plotCountsLabel[i]->setAlignment(Qt::AlignCenter);
        }

        //Initializing the total count labels
        totalCountsLabel = new QLabel (tr ("0"));
        totalCountsLabel->setAlignment(Qt::AlignCenter);

        //Initializing all the plots
        for(int i=0;i<(4+2*extraPanels)*ninputs/2+2;i++)
            mplot[i]=new plot2d(0,QSize(640,480),i);

        //Setting up the main histogram window
        Multiplewindow= new QWidget;
        Multiplewindow->setWindowTitle(getName());
        Multiplewindow->setMinimumSize(QSize(1280,720));

        QGridLayout *mainLayout = new QGridLayout(Multiplewindow);
        QStackedWidget *stack = new QStackedWidget();

        //Setting up the top part of the main window (select tab, clear, select plot style)
        QWidget *topWidget = new QWidget();
        QGridLayout *topLayout = new QGridLayout(topWidget);
        topWidget->setMaximumWidth(480);

        //Creating the tab select box
        QComboBox *pageSelect = new QComboBox();
            pageSelect->addItem("Totals");
            pageSelect->addItem("Energy");
            pageSelect->addItem("Time");
            for(int i=0;i<extraPanels;i++)
                pageSelect->addItem(QString("Extra Inputs %1").arg(i));
            pageSelect->addItem("Raw Energy");
            pageSelect->addItem("Raw Time");
            for(int i=0;i<extraPanels;i++)
                pageSelect->addItem(QString("Raw Extra Inputs %1").arg(i));
            connect(pageSelect, SIGNAL(activated(int)), stack, SLOT(setCurrentIndex(int)));
        topLayout->addWidget(pageSelect,0,0,1,1);
        topLayout->addWidget(resetButton,0,1,1,1);
        connect(resetButton,SIGNAL(clicked()),this,SLOT(resetButtonClicked()));

        //Creating the plot style select box
        QComboBox *setPlotStyle=new QComboBox();
            setPlotStyle->addItem("Normal");
            setPlotStyle->addItem("Logarithmic");
            setPlotStyle->addItem("Square root");
        topLayout->addWidget(setPlotStyle,0,2,1,1);
        connect(setPlotStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(modifyPlotState(int)));

        mainLayout->addWidget(topWidget,0,0,1,1);

        //creating the window and layout vectors
        windowVector.resize(5+extraPanels*2);
        multipleLayoutVector.resize(5+extraPanels*2);
        for(int i=0;i<5+extraPanels*2;i++)
        {
            windowVector[i]=new QWidget();
            multipleLayoutVector[i]=new QGridLayout(windowVector[i]);
        }

        //Placing the total plots
        multipleLayoutVector[0]->addWidget(mplot[2*ninputs],0,0,1,1);
        multipleLayoutVector[0]->addWidget(mplot[2*ninputs+1],1,0,1,1);
        stack->addWidget(windowVector[0]);

        //Placing the energy plots and count labels
        for(int i=0;i<rowcol;i++)
            for(int j=0;j<5;j++)
            {
                multipleLayoutVector[1]->addWidget(mplot[ninputs+i*5+j],12*i+2,j+1,11,1);
                multipleLayoutVector[1]->addWidget(plotCountsLabel[i*5+j],12*i+13,j+1,1,1);
                if(i*5+j+1>=ninputs/2) {i=rowcol; j=5;}
            }
        multipleLayoutVector[1]->setHorizontalSpacing(15);
        stack->addWidget(windowVector[1]);

        //Placing the time plots and count labels
        for(int i=0;i<rowcol;i++)
            for(int j=0;j<5;j++)
            {
                multipleLayoutVector[2]->addWidget(mplot[3*ninputs/2+i*5+j],12*i+2,j+1,11,1);
                multipleLayoutVector[2]->addWidget(plotCountsLabel[ninputs/2+i*5+j],12*i+13,j+1,1,1);
                if(i*5+j+1>=ninputs/2) {i=rowcol; j=5;}
            }
        multipleLayoutVector[2]->setHorizontalSpacing(15);
        stack->addWidget(windowVector[2]);

        //Initializing all the extraPanel plots and count labels
        for(int k=0;k<extraPanels;k++)
        {
            for(int i=0;i<rowcol;i++)
                for(int j=0;j<5;j++)
                {
                    multipleLayoutVector[3+k]->addWidget(mplot[2*ninputs+2+i*5+j],12*i+2,j+1,11,1);
                    multipleLayoutVector[3+k]->addWidget(plotCountsLabel[2*ninputs+i*5+j],12*i+13,j+1,1,1);
                    if(i*5+j+1>=ninputs/2) {i=rowcol; j=5;}
                }

            multipleLayoutVector[3+k]->setHorizontalSpacing(15);
            stack->addWidget(windowVector[3+k]);
        }

        //Initializing the Raw Energy plots and count labels
        for(int i=0;i<rowcol;i++)
            for(int j=0;j<5;j++)
            {
                multipleLayoutVector[3+extraPanels]->addWidget(mplot[i*5+j],12*i+2,j+1,11,1);
                multipleLayoutVector[3+extraPanels]->addWidget(plotCountsLabel[ninputs+i*5+j],12*i+13,j+1,1,1);
                if(i*5+j+1>=ninputs/2) {i=rowcol; j=5;}
            }
        multipleLayoutVector[3+extraPanels]->setHorizontalSpacing(15);
        stack->addWidget(windowVector[3+extraPanels]);

        //Initializing the Raw Time plots and count labels
        for(int i=0;i<rowcol;i++)
            for(int j=0;j<5;j++)
            {
                multipleLayoutVector[4+extraPanels]->addWidget(mplot[ninputs/2+i*5+j],12*i+2,j+1,11,1);
                multipleLayoutVector[4+extraPanels]->addWidget(plotCountsLabel[3*ninputs/2+i*5+j],12*i+13,j+1,1,1);
                if(i*5+j+1>=ninputs/2) {i=rowcol; j=5;}
            }
        multipleLayoutVector[4+extraPanels]->setHorizontalSpacing(15);
        stack->addWidget(windowVector[4+extraPanels]);

        //Initializing the extraPanel raw plots and count labels
        for(int k=0;k<extraPanels;k++)
        {
            for(int i=0;i<rowcol;i++)
                for(int j=0;j<5;j++)
                {
                    multipleLayoutVector[5+extraPanels+k]->addWidget(mplot[2*ninputs+2+i*5+j],12*i+2,j+1,11,1);
                    multipleLayoutVector[5+extraPanels+k]->addWidget(plotCountsLabel[2*ninputs+i*5+j],12*i+13,j+1,1,1);
                    if(i*5+j+1>=ninputs/2) {i=rowcol; j=5;}
                }

            multipleLayoutVector[5+extraPanels+k]->setHorizontalSpacing(15);
            stack->addWidget(windowVector[5+extraPanels+k]);
        }

        //Adding the tab stack and the total counts label to the Multiple histogram window
        mainLayout->addWidget(stack,1,0,1,1);
        mainLayout->addWidget(totalCountsLabel,2,0,1,1);

        //Adding the show button
        cl->addWidget(previewButton,0,0,1,4);
        connect(previewButton,SIGNAL(clicked()),this,SLOT(previewButtonClicked()));


        //Adding the total counter and the update speed spinner
        numCountsLabel = new QLabel (tr ("0"));
        updateSpeedSpinner = new QSpinBox();
        updateSpeedSpinner->setMinimum(1);
        updateSpeedSpinner->setMaximum(60);
        updateSpeedSpinner->setSingleStep(1);
        updateSpeedSpinner->setValue(conf.updateInterval);

        cl->addWidget(new QLabel ("Counts in histogram:"), 1, 0, 1, 1);
        cl->addWidget(numCountsLabel, 1, 1, 1, 3);
        cl->addWidget(new QLabel ("Update Speed (s)"),  1,2,1,1);
        cl->addWidget(updateSpeedSpinner,1,3,1,1);
        connect(updateSpeedSpinner,SIGNAL(valueChanged(int)),this,SLOT(setTimerTimeout(int)));

        //Adding the calibration row: A fixed label, a changeable label and the button
        calibName = new QLabel("No current calibration");
        calibNameButton = new QPushButton(tr("Chose a calibration file"));

        cl->addWidget(new QLabel ("Calibration file name:"),     2,0,1,1);
        cl->addWidget(calibName,      2,1,1,2);
        cl->addWidget(calibNameButton,    2,3,1,1);

        connect(calibNameButton,SIGNAL(clicked()),this,SLOT(calibNameButtonClicked()));

        //Adding the Graphana row: A fixed label, a changeable label and the button
        graphanaName = new QLabel("No current Graphana folder chosen");
        graphanaNameButton = new QPushButton(tr("Chose a folder for the Graphana files"));

        cl->addWidget(new QLabel ("Graphana folder:"),     3,0,1,1);
        cl->addWidget(graphanaName,      3,1,1,2);
        cl->addWidget(graphanaNameButton,    3,3,1,1);
        connect(graphanaNameButton,SIGNAL(clicked()),this,SLOT(graphanaNameButtonClicked()));

        //Adding the fixed bin row, with the energy and time size.

        nofBinsBox = new QComboBox();
        nofBinsBox->addItem("1024",1024);
        nofBinsBox->addItem("2048",2048);
        nofBinsBox->addItem("4096",4096);
        nofBinsBox->addItem("8192",8192);
        nofBinsBox->addItem("16384",16384);
        nofBinsBox->addItem("32768",32768);
        nofBinsBox->addItem("65536",65536);

        nofTBinsBox = new QComboBox();
        nofTBinsBox->addItem("1024",1024);
        nofTBinsBox->addItem("2048",2048);
        nofTBinsBox->addItem("4096",4096);
        nofTBinsBox->addItem("8192",8192);
        nofTBinsBox->addItem("16384",16384);
        nofTBinsBox->addItem("32768",32768);
        nofTBinsBox->addItem("65536",65536);

        cl->addWidget(new QLabel ("Number of Bins"),4,0,1,1);
        cl->addWidget(nofBinsBox,  4,1,1,1);
        cl->addWidget(new QLabel ("Number of Time Bins"),4,2,1,1);
        cl->addWidget(nofTBinsBox,  4,3,1,1);
        connect(nofBinsBox,SIGNAL(currentIndexChanged(int)),this,SLOT(nofBinsChanged(int)));
        connect(nofTBinsBox,SIGNAL(currentIndexChanged(int)),this,SLOT(nofTBinsChanged(int)));

        if(extraPanels)
        {
            nofSBinsBox = new QComboBox();
            nofSBinsBox->addItem("1024",1024);
            nofSBinsBox->addItem("2048",2048);
            nofSBinsBox->addItem("4096",4096);
            nofSBinsBox->addItem("8192",8192);
            nofSBinsBox->addItem("16384",16384);
            nofSBinsBox->addItem("32768",32768);
            nofSBinsBox->addItem("65536",65536);

            cl->addWidget(new QLabel ("Number of extra input Bins"),5,0,1,1);
            cl->addWidget(nofSBinsBox,  5,1,1,1);
            connect(nofSBinsBox,SIGNAL(currentIndexChanged(int)),this,SLOT(nofSBinsChanged(int)));
        }

        //Connecting the signals from right clicking to their functions
        for(int i=0;i<(4+2*extraPanels)*ninputs/2+2;i++)
            connect(mplot[i], SIGNAL(histogramCleared(unsigned int,unsigned int)), this, SLOT(resetSingleHistogram(unsigned int,unsigned int)));
        for(int i=0;i<(4+2*extraPanels)*ninputs/2+2;i++)
            connect(mplot[i], SIGNAL(changeZoomForAll(unsigned int, double, double)), this, SLOT(changeBlockZoom(unsigned int, double, double)));

        container->setLayout(cl);
    }

    l->addWidget(container,0,0,1,1);
}

void MultipleCacheHistogramPlugin::recalculateBinWidth()
{
    double calibxmax, xmax=0;

    if(calibrated)
    {
        for(int i=0;i<ninputs/2;i++)
        {
            calibxmax=0;
            for(int j=0;j<calibcoef[0][0];j++)
            {
                calibxmax=calibxmax+pow(conf.nofBins,j)*calibcoef[0][j+1];
            }
            if(xmax<calibxmax)
            {
                xmax=calibxmax;
                binWidth = xmax/((double)conf.nofBins);
            }
        }
    }
    else
        binWidth = 1;
}

void MultipleCacheHistogramPlugin::changeBlockZoom(unsigned int id0, double x, double width)
{
    int id = (int) id0;
    if(id<ninputs/2)
    {
        for(int i=0;i<ninputs/2;i++)
            mplot[i]->setZoom(x,width);
    }
    else if (id<ninputs)
    {
        for(int i=ninputs/2;i<ninputs;i++)
            mplot[i]->setZoom(x,width);
    }
    else if((id<3*ninputs/2)||(id==2*ninputs))
    {
        for(int i=ninputs;i<3*ninputs/2;i++)
            mplot[i]->setZoom(x,width);
        mplot[2*ninputs]->setZoom(x,width);
    }
    else if((id<2*ninputs)||(id==2*ninputs+1))
    {
        for(int i=3*ninputs/2;i<2*ninputs;i++)
            mplot[i]->setZoom(x,width);
        mplot[2*ninputs+1]->setZoom(x,width);
    }
    else if(extraPanels)
    {
        for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
            mplot[i]->setZoom(x,width);
    }
}

void MultipleCacheHistogramPlugin::resetSingleHistogram(unsigned int,unsigned int c)
{
    int b = (int) c;
    if(b<ninputs)
    {
        rawcache[b].clear();
        rawcache[b].fill(0, conf.nofBins);
    }
    else if(b<ninputs*2)
    {
        cache[b-ninputs].clear();
        cache[b-ninputs].fill(0, conf.nofBins);
    }
    else if(b==ninputs*2)
    {
        totalCache[0].clear();
        totalCache[0].fill(0, conf.nofBins);
    }
    else if(b==ninputs*2+1)
    {
        totalCache[1].clear();
        totalCache[1].fill(0, conf.nofBins);
    }
    else if((extraPanels)&&(b<5*ninputs/2+2))
    {
        cache[b-ninputs-2].clear();
        cache[b-ninputs-2].fill(0, conf.nofSBins);
    }

    mplot[b]->resetBoundaries(0);
}

void MultipleCacheHistogramPlugin::previewButtonClicked()
{
    if(Multiplewindow->isHidden())
    {
         Multiplewindow->show();
    }
    else
    {
         Multiplewindow->hide();
    }
}

void MultipleCacheHistogramPlugin::nofBinsChanged(int newValue)
{
     conf.nofBins = nofBinsBox->itemData(newValue,Qt::UserRole).toInt();
}

void MultipleCacheHistogramPlugin::nofTBinsChanged(int newValue)
{
     conf.nofTBins = nofTBinsBox->itemData(newValue,Qt::UserRole).toInt();
}

void MultipleCacheHistogramPlugin::nofSBinsChanged(int newValue)
{
     conf.nofSBins = nofSBinsBox->itemData(newValue,Qt::UserRole).toInt();
}

void MultipleCacheHistogramPlugin::calibNameButtonClicked()
{
     setCalibName(QFileDialog::getOpenFileName(this,tr("Choose calibration file name"), "/home",tr("Text (*.cal)")));
}

void MultipleCacheHistogramPlugin::setCalibName(QString _runName)
{
    calibName->setText(_runName);
    conf.calName=_runName;
}

void MultipleCacheHistogramPlugin::graphanaNameButtonClicked()
{
     setGraphanaName(QFileDialog::getExistingDirectory(this,tr("Choose Graphana Folder"), "/home"));
}

void MultipleCacheHistogramPlugin::setGraphanaName(QString _runName)
{
    graphanaName->setText(_runName);
    conf.graphName=_runName;
}

void MultipleCacheHistogramPlugin::modifyPlotState(int state)
{
    for(int i=0;i<2*ninputs+2;i++)
        mplot[i]->setPlotState(state);
    if(extraPanels)
        for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
            mplot[i]->setPlotState(state);
}

void MultipleCacheHistogramPlugin::findCalibName(QString newValue)
{
    QByteArray ba = newValue.toLatin1();
    const char *c_str2 = ba.data();
    calibration.open (c_str2);
    int dump, det;
    if(calibration.is_open())
    {
        calibrated=1;

        while(!calibration.eof())
        {
            calibration>>dump;
            calibration>>det;
            calibration>>calibcoef[det][0];
            calibcoef[det].resize(1+calibcoef[det][0]);
            for(int j=0;j<calibcoef[det][0];j++)
            {
                calibration>>calibcoef[det][j+1];
            }
        }
    }
    else calibrated=0;

    recalculateBinWidth();
    for(int i=ninputs;i<3*ninputs/2;i++)
        mplot[i]->setCalibration(binWidth);
    mplot[2*ninputs]->setCalibration(binWidth);

    calibration.close();
}

void MultipleCacheHistogramPlugin::updateVisuals()
{
    int totalCounts=0, totalRate=0;

    for(int i=0;i<ninputs;i++)
    {

        if(!rawcache[i].empty())
        {
            if(!rawCheck)
                        mplot[i]->getChannelById(0)->setData(rawcache[i]);
        }

        if(!cache[i].empty())
            if(!cleanCheck)
                        mplot[ninputs+i]->getChannelById(0)->setData(cache[i]);
    }

    if(extraPanels)
        for(int i=ninputs;i<3*ninputs/2;i++)
        {
            if(!cache[i].empty())
                mplot[ninputs+2+i]->getChannelById(0)->setData(cache[i]);
        }

    if(!totalCache[0].empty())
	if(!cleanCheck)
                mplot[2*ninputs]->getChannelById(0)->setData(totalCache[0]);
    if(!totalCache[1].empty())
	if(!cleanCheck)
                mplot[2*ninputs+1]->getChannelById(0)->setData(totalCache[1]);

    Multiplewindow->update();
    numCountsLabel->setText(tr("%1").arg(nofCounts));

    for(int i=0;i<ninputs*2;i++)
    {
        evRate[i] = (plotCounts[i]-prevCount[i])/conf.updateInterval;
        if(i<ninputs/2)
        {
            totalCounts+=plotCounts[i];
            totalRate+=evRate[i];
        }
        plotCountsLabel[i]->setText(tr("%1 events     %2 ev/s").arg(plotCounts[i]).arg(evRate[i]));
        prevCount[i]=plotCounts[i];
    }

    if(extraPanels)
    {
        for(int i=ninputs*2;i<5*ninputs/2;i++)
        {
            evRate[i] = (plotCounts[i]-prevCount[i])/conf.updateInterval;
            plotCountsLabel[i]->setText(tr("%1 events     %2 ev/s").arg(plotCounts[i]).arg(evRate[i]));
            prevCount[i]=plotCounts[i];
        }
    }

    uint64_t nofTriggers = RunManager::ptr()->sendTriggers();
    uint64_t trigspersec = RunManager::ptr()->sendTriggerRate();

    rates.open ((tr("%1%2%3%4").arg(conf.graphName).arg("/").arg(getName()).arg(".txt")).toLatin1().data(),std::ofstream::out);
    rates<<trigspersec<<" "<<nofTriggers<<std::endl;
    rates<<totalCounts<<" "<<totalRate<<std::endl;
    for(int i=0;i<ninputs/2;i++)
    {
        rates<<plotCounts[i]<<" ";
    }

    rates<<std::endl;

    for(int i=0;i<ninputs/2;i++)
    {
        rates<<evRate[i]<<" ";
    }

    rates<<std::endl;

    for(int i=ninputs;i<3*ninputs/2;i++)
    {
        rates<<plotCounts[i]<<" ";
    }

    rates<<std::endl;

    for(int i=ninputs;i<3*ninputs/2;i++)
    {
        rates<<evRate[i]<<" ";
    }

    rates<<std::endl;

    for(int i=3*ninputs/2;i<2*ninputs;i++)
    {
        rates<<plotCounts[i]<<" ";
    }

    rates<<std::endl;

    for(int i=3*ninputs/2;i<2*ninputs;i++)
    {
        rates<<evRate[i]<<" ";
    }

    rates.close();

    spectra.open ((tr("%1%2%3%4").arg(conf.graphName).arg("/").arg(getName()).arg("spectrum.txt")).toLatin1().data(),std::ofstream::out);

    spectra<<binWidth<<std::endl;

    for(int i=0;i<8192;i++)
    {
        if(!cleanCheck)
            spectra<<totalCache[0][i]<<" ";
        else
            spectra<<0<<" ";
    }

    spectra.close();

    totalCountsLabel->setText(tr("TOTALS: %1 events     %2 ev/s     TRIGGERS: %3 triggers      %4 trig/s").arg(totalCounts).arg(totalRate).arg(nofTriggers).arg(trigspersec));
}

void MultipleCacheHistogramPlugin::resetButtonClicked()
{
  scheduleReset = true;
}

void MultipleCacheHistogramPlugin::setTimerTimeout(int msecs)
{
    conf.updateInterval = msecs;
    updateTimer->setInterval(1000*msecs);
}

bool MultipleCacheHistogramPlugin::notVeto(uint32_t time, int det)
{
    if(BGOVeto)
    {
        if(vetoData[det].size()>readPointer[ninputs+det])
        {
            while(time+10>vetoData[det][readPointer[ninputs+det]+1])
            {
                if(time-vetoData[det][readPointer[ninputs+det]+1]<10)
                {
                    readPointer[ninputs+det]+=2;
                    return 0;
                }
                else readPointer[ninputs+det]+=2;
                if(vetoData[det].size()<readPointer[ninputs+det])
                    break;
            }
        }
    }

    return 1;
}

void MultipleCacheHistogramPlugin::writeEnergyOnly(int32_t energy, int det)
{
    if(energy > 0 && energy < conf.nofBins)
    {
        rawcache[det][energy]++;
        plotCounts[det+ninputs]++;
    }
    readPointer[det]+=2;
}

void MultipleCacheHistogramPlugin::writeTimeOnly(int32_t time, int det)
{
    if(time > 0 && time < conf.nofTBins)
    {
        rawcache[det+ninputs/2][time]++;
        plotCounts[det+3*ninputs/2] ++;
    }
    readPointer[det+ninputs/2]+=2;
}

/*!
* @fn void CacheHistogramPlugin::userProcess()
* @brief For plugin specific processing code
*
* @warning This function must ONLY be called from the plugin thread;
* @variable pdata wants to be a vector<double> of new values to be inserted into the histogram
*/
void MultipleCacheHistogramPlugin::userProcess()
{
    QVector< QVector<uint32_t> > idata;
    if(BGOVeto)
    {
        vetoData.resize(ninputs/2);
    }

    if(extraPanels)
    {
        secondTimeData.resize(ninputs/2);
    }

    idata.resize(ninputs);
    readPointer.fill(0);
    uint32_t stampEnergy;
    uint32_t stampTime;
    int binEnergy,bin3=0, binTime;
    double bin2=0, randomized, bin4;


    for(int i=0;i<ninputs;i++)
    {
        idata[i] = inputs->at(i)->getData().value< QVector<uint32_t> > ();
    }

    if(BGOVeto)
        for(int i=0;i<ninputs/2;i++)
        {
            vetoData[i] = inputs->at(i+ninputs)->getData().value< QVector<uint32_t> > ();
        }

    if(extraPanels)
    {
        if(BGOVeto) secondTimePosition=3*ninputs/2;
        else secondTimePosition=ninputs;
        for(int i=0;i<ninputs/2;i++)
        {
            secondTimeData[i] = inputs->at(i+secondTimePosition)->getData().value< QVector<uint32_t> > ();
        }
    }

    if(scheduleReset)
    {
        for(int i=0;i<ninputs/2;i++)
        {
            cache[i].clear();
            cache[i].fill(0, conf.nofBins);
            rawcache[i].clear();
            rawcache[i].fill(0, conf.nofBins);
        }
        for(int i=ninputs/2;i<ninputs;i++)
        {
            cache[i].clear();
            cache[i].fill(0, conf.nofTBins);
            rawcache[i].clear();
            rawcache[i].fill(0, conf.nofTBins);
        }
        if(extraPanels)
        {
            for(int i=ninputs;i<3*ninputs/2;i++)
            {
                cache[i].clear();
                cache[i].fill(0, conf.nofSBins);
            }
            for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
                mplot[i]->resetBoundaries(0);
        }
        totalCache[0].clear();
        totalCache[0].fill(0, conf.nofBins);
        totalCache[1].clear();
        totalCache[1].fill(0, conf.nofTBins);
        recalculateBinWidth();
        scheduleReset = false;
        for(int i=0;i<2*ninputs+2;i++)
            mplot[i]->resetBoundaries(0);
    }

    for(int i=0;i<ninputs/2;i++)
    {
        if((int)(cache[i].size()) != conf.nofBins) cache[i].resize(conf.nofBins);
        if((int)(rawcache[i].size()) != conf.nofBins) rawcache[i].resize(conf.nofBins);
    }
    for(int i=ninputs/2;i<ninputs;i++)
    {
        if((int)(cache[i].size()) != conf.nofTBins) cache[i].resize(conf.nofTBins);
        if((int)(rawcache[i].size()) != conf.nofTBins) rawcache[i].resize(conf.nofTBins);
    }
    if(extraPanels)
        for(int i=ninputs;i<3*ninputs/2;i++)
        {
            if((int)(cache[i].size()) != conf.nofSBins) cache[i].resize(conf.nofSBins);
        }

    if((int)(totalCache[0].size()) != conf.nofBins) totalCache[0].resize(conf.nofBins);
    if((int)(totalCache[1].size()) != conf.nofTBins) totalCache[1].resize(conf.nofTBins);


    // Add data to histogram
    for(int i=0;i<ninputs/2;i++)
    {
        while((readPointer[i]<idata[i].size())&&(readPointer[i+ninputs/2]<idata[i+ninputs/2].size()))
        {
            stampEnergy = idata[i][readPointer[i]+1];
            stampTime   = idata[i+ninputs/2][readPointer[i+ninputs/2]+1];

            binEnergy = idata[i][readPointer[i]];
            binTime= idata[i+ninputs/2][readPointer[i+ninputs/2]];

            if(std::min(stampEnergy,stampTime)+100>std::max(stampEnergy,stampTime))
            {
                if(notVeto(stampTime,i))
                {
                        if(calibrated)
                        {
                            bin2=0;
                            randomized=((double) rand() / (RAND_MAX));
                            bin4=binEnergy+randomized;
                            for(int j=0;j<calibcoef[i][0];j++)
                                bin2=bin2+pow(bin4,j)*calibcoef[i][j+1];
                            bin3=bin2/binWidth;
                        }
                        else bin3=binEnergy;

                        cleanCheck=1;
                        rawCheck=1;
                        if(binEnergy > 0 && binEnergy < conf.nofBins)
                        {
                            rawcache[i][binEnergy]++;
                            plotCounts[i+ninputs]++;
                        }

                        if(bin3 > 0 && bin3 < conf.nofBins)
                        {
                            cache[i][bin3] ++;
                            totalCache[0][bin3]++;
                            ++nofCounts;
                            plotCounts[i] ++;
                        }

                        if(binTime > 0 && binTime < conf.nofTBins)
                        {
                            cache[i+ninputs/2][binTime] ++;
                            rawcache[i+ninputs/2][binTime]++;
                            totalCache[1][binTime]++;
                            ++nofCounts;
                            plotCounts[i+ninputs/2] ++;
                            plotCounts[i+3*ninputs/2] ++;
                        }

                        cleanCheck=0;
                        rawCheck=0;
                        readPointer[i]+=2;
                        readPointer[i+ninputs/2]+=2;
                }
                else
                {
                    rawCheck=1;
                    writeEnergyOnly(binEnergy,i);
                    writeTimeOnly(binTime,i);
                    rawCheck=0;
                }

            }
            else if (stampEnergy<stampTime)
            {
                rawCheck=1;
                writeEnergyOnly(binEnergy,i);
                rawCheck=0;
            }
            else if (stampEnergy>stampTime)
            {
                rawCheck=1;
                writeTimeOnly(binTime,i);
                rawCheck=0;
            }
        }

        while(readPointer[i]<idata[i].size())
        {
            binEnergy = idata[i][readPointer[i]];
            writeEnergyOnly(binEnergy,i);
        }

        while(readPointer[i+ninputs/2]<idata[i+ninputs/2].size())
        {
            binTime= idata[i+ninputs/2][readPointer[i+ninputs/2]];
            writeTimeOnly(binTime,i);
        }

        if(extraPanels)
        {
            int kk=0;
            foreach(double datum, secondTimeData[i])
            {
                if(kk%2==0)
                {
                    if(datum < conf.nofSBins && datum >= 0)
                    {
                            cache[ninputs+i][datum] ++;
                            plotCounts[2*ninputs+i]++;
                    }
                }
                kk++;
            }
        }
    }
}

void MultipleCacheHistogramPlugin::runStartingEvent () {
    // reset all timers and the histogram before starting anew
    emit stopUpdateTimer();
    scheduleReset = true;
    nofCounts = 0;
    plotCounts.fill(0);
    prevCount.fill(0);
    evRate.fill(0);
    rawCheck=0;
    cleanCheck=0;
    if(BGOVeto)
        readPointer.resize(3*ninputs/2);
    else readPointer.resize(ninputs);
    recalculateBinWidth();
    for(int i=0;i<ninputs/2;i++)
    {
        cache[i].resize(conf.nofBins);
        cache[i].fill(0, conf.nofBins);
        rawcache[i].resize(conf.nofBins);
        rawcache[i].fill(0, conf.nofBins);
    }
    for(int i=ninputs/2;i<ninputs;i++)
    {
        cache[i].resize(conf.nofTBins);
        cache[i].fill(0, conf.nofTBins);
        rawcache[i].resize(conf.nofTBins);
        rawcache[i].fill(0, conf.nofTBins);
    }
    if(extraPanels)
    {
        for(int i=ninputs;i<3*ninputs/2;i++)
        {
            cache[i].resize(conf.nofSBins);
            cache[i].fill(0, conf.nofSBins);
        }
    }
    totalCache[0].resize(conf.nofBins);
    totalCache[0].fill(0, conf.nofBins);
    totalCache[1].resize(conf.nofTBins);
    totalCache[1].fill(0, conf.nofTBins);

    emit startUpdateTimer(conf.updateInterval*1000);
}

/*!
\page cachehistogramplg Histogram Cache Plugin
\li <b>Plugin names:</b> \c cachehistogramplugin
\li <b>Group:</b> Cache

\section pdesc Plugin Description
The histogram cache plugin creates histograms from values fed to its input connector.
The computed histogram may be stored to disk periodically.
It is also possible to define a timespan after which the histogram is reset (increasing the index of the file the histogram is stored to).
That way, one can get a separate histogram for every timeslice.

The plugin can also display a plot of the current histogram.

\section attrs Attributes
None

\section conf Configuration
\li <b>Auto reset</b>: Enable or disable automatic reset after given interval
\li <b>Auto reset interval</b>: Interval after which the histogram is reset (in minutes)
\li <b>Auto save</b>: Enable or disable automatic saving after given interval
\li <b>Auto save interval</b>: Interval after which the current histogram is saved (in seconds)
\li <b>From</b>: lower bound of the lowest histogram bin
\li <b>Max Height</b>: Not yet implemented
\li <b>Normalize</b>: Normalizes the histogram to its maximum value (\b Attention: This is still buggy)
\li <b>Number of Bins</b>: Number of bins the range [From..To] is divided into
\li <b>Preview</b>: Shows a live plot of the histogram
\li <b>Reset</b>: Manually reset the histogram. \b Attention: This will NOT create a new file. The histogram will be saved to the current file.
\li <b>To</b>: upper bound of the highest histogram bin
\li <b>Update Speed</b>: Interval between updates of the histogram plot and counter

\section inputs Input Connectors
\li \c in \c &lt;double>: Input for the data to be histogrammed

\section outputs Output Connectors
\li \c fileOut, out \c &lt;double>: Contains the current histogram
*/
