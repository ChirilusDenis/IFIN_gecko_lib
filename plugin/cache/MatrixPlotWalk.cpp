#include "MatrixPlotWalk.h"

static PluginRegistrar registrar ("2D Matrix for Walk adjustment", MatrixPlotWalk::create, AbstractPlugin::GroupCache, MatrixPlotWalk::getMCHPAttributeMap());


MatrixPlotWalk::MatrixPlotWalk(int _id, QString _name, const Attributes &_attrs)
    : BasePlugin(_id, _name),
      attribs_ (_attrs)
{
    createSettings(settingsLayout);

    //Creating the standard inputs
    addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Reference Energy")));
    addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Reference Time")));
    addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Detector Energy")));
    addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Detector Time")));

    //The plugin needs only one input to have data in order to run
    setNumberOfMandatoryInputs(1);

    std::cout << "Instantiated D2Display" << std::endl;

    //Starting a timer that updates all the visuals. It takes a ms value, thus the *1000
    secondTimer = new QTimer();
    secondTimer->start(20000);
    connect(secondTimer,SIGNAL(timeout()),this,SLOT(updateVisuals()));

    data.resize(8192);
    for(int i=0;i<8192;i++)
    {
        data[i].resize(8192);
        data[i].fill(0,8192);
    }
}

AbstractPlugin::AttributeMap MatrixPlotWalk::getMCHPAttributeMap() {
    AbstractPlugin::AttributeMap attrs;
    return attrs;
}

AbstractPlugin::Attributes MatrixPlotWalk::getAttributes () const { return attribs_;}

void MatrixPlotWalk::createSettings(QGridLayout * l)
{
    QWidget* container = new QWidget();
    {
        QGridLayout* cl = new QGridLayout;
        matrixButton = new QPushButton(tr("Show 2D Matrix"));

        QLabel* noCountsLabel = new QLabel(tr("Number of counts in the graph"));
        noCounter = new QLabel(tr("0"));

        QLabel* minRefLabel = new QLabel(tr("Minimum energy count in referrence"));
        minRef = new QSpinBox();
        minRef->setMinimum(0);
        minRef->setMaximum(8191);
        minRef->setSingleStep(1);

        QLabel* maxRefLabel = new QLabel(tr("Maximum energy count in referrence"));
        maxRef = new QSpinBox();
        maxRef->setMinimum(1);
        maxRef->setMaximum(8192);
        maxRef->setSingleStep(1);

        calibLabel = new QLabel();
        calibButton = new QPushButton(tr("Choose the calibration file"));

        clearButton = new QPushButton(tr("Clear Spectra"));

        connect(matrixButton,SIGNAL(clicked()),this,SLOT(on_display_2d_clicked()));
        connect(minRef,SIGNAL(valueChanged(int)),this,SLOT(refMinChanged()));
        connect(maxRef,SIGNAL(valueChanged(int)),this,SLOT(refMaxChanged()));
        connect(calibButton,SIGNAL(clicked()),this,SLOT(calibButtonClicked()));
        connect(clearButton,SIGNAL(clicked()),this,SLOT(clearButtonClicked()));

        cl->addWidget(clearButton,      0,0,1,3);
        cl->addWidget(matrixButton,     1,0,1,3);
        cl->addWidget(noCountsLabel,    2,0,1,2);
        cl->addWidget(noCounter,        2,2,1,1);
        cl->addWidget(minRefLabel,      3,0,1,2);
        cl->addWidget(minRef,           3,2,1,1);
        cl->addWidget(maxRefLabel,      4,0,1,2);
        cl->addWidget(maxRef,           4,2,1,1);
        cl->addWidget(new QLabel("Calibration file Name:"),5,0,1,1);
        cl->addWidget(calibLabel,       5,1,1,1);
        cl->addWidget(calibButton,      5,2,1,1);

        container->setLayout(cl);
    }

    // End

    l->addWidget(container,0,0,1,1);
}

void MatrixPlotWalk::calibButtonClicked()
{
    //Change calibration file
     calName=QFileDialog::getExistingDirectory(this,tr("Choose the calibration file"), "/home",QFileDialog::ShowDirsOnly);
     getCalib();
}

void MatrixPlotWalk::clearButtonClicked()
{
    do{
        if(!stopAq)
        {
            for(int i=0;i<8192;i++)
            {
                data[i].resize(8192);
                data[i].fill(0,8192);
            }
            noCounts=0;
        }
    }while(stopAq);
}

void MatrixPlotWalk::getCalib()
{
    QByteArray ba = calName.toLatin1();
    const char *c_str2 = ba.data();
    std::ifstream calibration;
    calibration.open (c_str2);
    int dump, det;
    if(calibration.is_open())
    {
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

    calibration.close();
}

void MatrixPlotWalk::refMinChanged()
{
    minRefValue=minRef->value();
}

void MatrixPlotWalk::refMaxChanged()
{
    maxRefValue=maxRef->value();
}

void MatrixPlotWalk::applySettings(QSettings* settings)
{
    QString set;
    settings->beginGroup(getName());
    set = "minRefValue"; if(settings->contains(set)) minRefValue = settings->value(set).toInt();
    set = "maxRefValue"; if(settings->contains(set)) maxRefValue = settings->value(set).toInt();
    set = "calName"; if(settings->contains(set))     calName = settings->value(set).toString();
        settings->endGroup();

    maxRef->setValue(maxRefValue);
    minRef->setValue(minRefValue);
    calibLabel->setText(calName);
}

void MatrixPlotWalk::saveSettings(QSettings* settings)
{
    if(settings == NULL)
    {
        std::cout << getName().toStdString() << ": no settings file" << std::endl;
        return;
    }
    else
    {
        std::cout << getName().toStdString() << " saving settings...";
        settings->beginGroup(getName());
        settings->setValue("minRefValue",minRefValue);
        settings->setValue("maxRefValue",maxRefValue);
        settings->setValue("calName",calName);
            settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void MatrixPlotWalk::runStartingEvent()
{
    data.resize(8192);
    for(int i=0;i<8192;i++)
    {
        data[i].resize(8192);
        data[i].fill(0,8192);
    }
    readPointer.resize(4);
    stampsChecked.resize(4);
    noCounts=0;
    stopAq=0;

    if(minRefValue>maxRefValue)
    {
        int temp;
        temp=minRefValue;
        minRefValue=maxRefValue;
        maxRefValue=temp;
    }
}

MatrixPlotWalk::~MatrixPlotWalk()
{
}

void MatrixPlotWalk::on_display_2d_clicked()
{
    myDisplay = new D2Display;
    myDisplay->setData2(data);
    myDisplay->show();
}

bool MatrixPlotWalk::checkPointers()
{
    bool check=1;
    if(readPointer[0]>refEnergy.size())
        check=0;
    else if (readPointer[1]>refTime.size())
        check=0;
    else if (readPointer[2]>energy.size())
        check=0;
    else if (readPointer[3]>time.size())
        check=0;

    return check;
}

bool MatrixPlotWalk::checkStamps(int32_t a, int32_t b, int32_t c, int32_t d)
{
    int32_t min;
    min=a;
    if(b<min)
        min=b;
    if(c<min)
        min=c;
    if(d<min)
        min=d;

    bool check=1;
    stampsChecked.fill(1,4);
    if(abs(a-min)>100)
    {
        check=0;
        stampsChecked[0]=0;
    }
    if(abs(b-min)>100)
    {
        check=0;
        stampsChecked[1]=0;
    }
    if(abs(c-min)>100)
    {
        check=0;
        stampsChecked[2]=0;
    }
    if(abs(d-min)>100)
    {
        check=0;
        stampsChecked[3]=0;
    }
    return check;
}

void MatrixPlotWalk::updateVisuals()
{
    stopAq=1;
    noCounter->setText(QString::number(noCounts));
    if(myDisplay)
        myDisplay->setData2(data);
    stopAq=0;
}

void MatrixPlotWalk::userProcess()
{
    refEnergy   =   inputs->at(0)->getData().value< QVector<uint32_t> > ();
    refTime     =   inputs->at(1)->getData().value< QVector<uint32_t> > ();
    energy      =   inputs->at(2)->getData().value< QVector<uint32_t> > ();
    time        =   inputs->at(3)->getData().value< QVector<uint32_t> > ();

    readPointer.fill(0,4);

    uint32_t stampRefEnergy;
    uint32_t stampRefTime;
    uint32_t binRefEnergy;
    int32_t binRefTime;

    uint32_t stampEnergy;
    uint32_t stampTime;
    uint32_t binEnergy;
    int32_t binTime;

    while(checkPointers())
    {
        stampRefEnergy =   refEnergy[readPointer[0]+1];
        stampRefTime   =   refTime[readPointer[1]+1];
        binRefEnergy   =   refEnergy[readPointer[0]];
        binRefTime     =   refTime[readPointer[1]];

        stampEnergy =   energy[readPointer[2]+1];
        stampTime   =   time[readPointer[3]+1];
        binEnergy   =   energy[readPointer[2]];
        binTime     =   time[readPointer[3]];

        if(!stopAq)
        {
            if(checkStamps(stampRefEnergy,stampRefTime,stampEnergy,stampTime))
            {
                if((binRefEnergy>=minRefValue)&&(binRefEnergy<=maxRefValue))
                {
                    if(binTime-binRefTime+2000>0)
                        if(binTime-binRefTime+2000<8192)
                            if(!stopAq)
                                data[binEnergy][binTime-binRefTime+2000]++;
                    noCounts++;
                }

                for(int i=0;i<4;i++)
                     readPointer[i]+=2;
            }
            else
                for(int i=0;i<4;i++)
                    if(stampsChecked[i])
                        readPointer[i]+=2;
        }
    }
}
