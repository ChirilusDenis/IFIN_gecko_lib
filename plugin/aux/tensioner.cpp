#include "tensioner.h"
#include "pluginmanager.h"
#include "pluginconnectorqueued.h"


static PluginRegistrar registrar ("tensioner", Tensioner::create, AbstractPlugin::GroupPack, Tensioner::getTensionerAttributeMap());

Tensioner::Tensioner(int _id, QString _name, const Attributes &_attrs)
            : BasePlugin(_id, _name)
            , attribs_ (_attrs)
{
    createSettings(settingsLayout);

    tensionerPlot->addChannel(0,tr("histogram"),QVector<double>(1,0),
                 QColor(153,153,153),Channel::steps,1);

    std::cout << "Instantiated Tensioner" << std::endl;
}

void Tensioner::createSettings(QGridLayout * l)
{
    tensionerPlot=new plot2d(0,QSize(640,480),0);
    QWidget* container = new QWidget();
    {
    QGridLayout* cl = new QGridLayout;

    tensionerPlot->setMinimumSize(640,360);

    QLabel* dataLabel = new QLabel(tr("Data file name:"));
    dataNameEdit = new QLineEdit();
    dataNameButton = new QPushButton(tr("..."));

    maximaFinderButton = new QPushButton(tr("Find maxima"));

    connect(dataNameButton,SIGNAL(clicked()),this,SLOT(dataNameButtonClicked()));
    connect(dataNameEdit, SIGNAL(textChanged(QString)), this, SLOT(readData(QString)));
    connect(maximaFinderButton,SIGNAL(clicked()),this,SLOT(findMaxima()));

    cl->addWidget(tensionerPlot,0,0,2,4);
    cl->addWidget(dataLabel,     2,0,1,1);
    cl->addWidget(dataNameEdit,      2,1,1,2);
    cl->addWidget(dataNameButton,    2,3,1,1);
    cl->addWidget(maximaFinderButton,    3,0,1,1);
    container->setLayout(cl);
    }
    l->addWidget(container,0,0,1,1);
    std::cout << " done" << std::endl;
}

void Tensioner::findMaxima()
{
 tensionerPlot->maximumFinder();
}

void Tensioner::dataNameButtonClicked()
{
     setDataName(QFileDialog::getOpenFileName(this,tr("Choose data file"), "/home",tr("Text (*.txt)")));
}

void Tensioner::setDataName(QString _runName)
{
    dataNameEdit->setText(_runName);
    conf.dataName=_runName;
}

void Tensioner::readData(QString newValue)
{
    QByteArray ba = newValue.toLatin1();
    const char *c_str2 = ba.data();
    int ignore;
    data.resize(8192);
    std::ifstream input;
    input.open (c_str2);

    if(input.is_open())
        for(int i=0;i<8192;i++)
        {
            input>>ignore;
            input>>data[i];
        }

    input.close();

    tensionerPlot->getChannelById(0)->setData(data);
}

AbstractPlugin::AttributeMap Tensioner::getTensionerAttributeMap() {
    AbstractPlugin::AttributeMap attrs;
    return attrs;
}

void Tensioner::applySettings(QSettings* settings)
{
    std::cout << " done" << std::endl;
}

void Tensioner::saveSettings(QSettings* settings)
{
    std::cout << " done" << std::endl;
}

void Tensioner::userProcess()
{
    std::cout << " done" << std::endl;
}

