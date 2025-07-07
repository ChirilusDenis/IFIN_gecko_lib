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

#include "mesytecMtdc32ui.h"
#include "mesytecMtdc32module.h"
#include <iostream>

MesytecMtdc32UI::MesytecMtdc32UI(MesytecMtdc32Module* _module)
    : module(_module), uif(this,&tabs), applyingSettings(false),
      previewRunning(false)
{
    createUI();
    createPreviewUI();

    previewTimer = new QTimer();
    previewTimer->setInterval(50);
    previewTimer->setSingleShot(true);

    std::cout << "Instantiated" << _module->getName().toStdString() << "UI" << std::endl;
}

MesytecMtdc32UI::~MesytecMtdc32UI(){}

void MesytecMtdc32UI::createUI()
{
    QGridLayout* l = new QGridLayout;
    l->setMargin(0);
    l->setVerticalSpacing(0);

    int nt = 0; // current tab number
    int ng = 0; // current group number

    // TAB ACQUISITION
    tn.append("Acq"); uif.addTab(tn[nt]);

    gn.append("Event Setup"); uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addPopupToGroup(tn[nt],gn[ng],"Data Format","data_length_format",
                        (QStringList()
                         << "8 bit"
                         << "16 bit"
                         << "32 bit"
                         << "64 bit"));
    uif.addPopupToGroup(tn[nt],gn[ng],"Mode","multi_event_mode",
                        (QStringList()
                         << "Single Event"
                         << "Multi Event 1"
                         << "Not used"
                         << "Multi Event 3"));
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"Different EOB marker", "enable_different_eob_marker");
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"Compare with Max Transfer Data", "enable_compare_with_max");
    uif.addPopupToGroup(tn[nt],gn[ng],"Marking Type","marking_type",
                        (QStringList()
                         << "Event Counter"
                         << "Timestamp"
                         << "Extended Timestamp"));

    uif.addPopupToGroup(tn[nt],gn[ng],"Bank Operation","bank_operation",
                        (QStringList()
                         << "Connected"
                         << "Independent"));
    uif.addPopupToGroup(tn[nt],gn[ng],"TDC resolution","tdc_resolution",
                        (QStringList()
                         << "3.9 ps"
                         << "7.8 ps"
                         << "15.6 ps"
                         << "31.3 ps"
                         << "62.5 ps"
                         << "125 ps"
                         << "250 ps"
                         << "500 ps"));
    uif.addPopupToGroup(tn[nt],gn[ng],"Output format","output_format",
                        (QStringList()
                         << "Standard"
                         << "Full Time Stamp"));

    gn.append("Pulser"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addPopupToGroup(tn[nt],gn[ng],"Pulser Mode","test_pulser_mode",
                        (QStringList()
                         << "Off"
                         << "On"));
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Pulser Pattern","pulser_pattern",0,33);
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank0 Input Threshold","bank0_input_thr",0,256); // 8 bits
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank1 Input Threshold","bank1_input_thr",0,256); // 8 bits

    // TAB Addressing
    tn.append("Addr"); nt++; uif.addTab(tn[nt]);

    gn.append("ID"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Base Address","base_addr","");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Firmware","firmware","unknown");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Firmware expected","firmware_expected","01.02");
    uif.addButtonToGroup(tn[nt],gn[ng],"Update","update_firmware_button");
    uif.addHexSpinnerToGroup(tn[nt],gn[ng],"Module ID","module_id",0,0xff); //8 bits

    gn.append("Basic"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addPopupToGroup(tn[nt],gn[ng],"Address Source","addr_source",
                        (QStringList()
                         << "Board"
                         << "Register"));
    uif.addHexSpinnerToGroup(tn[nt],gn[ng],"Address Register","base_addr_register",0,0xffff); // 16 bits

    gn.append("MCST/CBLT"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"CBLT Active","cblt_active");
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"MCST Active","mcst_active");
    uif.addRadioGroupToGroup(tn[nt],gn[ng],"CBLT Placement",
                             (QStringList() << "First" << "Middle" << "Last"),
                             (QStringList() << "enable_cblt_first"
                                            << "enable_cblt_last"
                                            << "enable_cblt_middle"));
    uif.addHexSpinnerToGroup(tn[nt],gn[ng],"CBLT address","cblt_addr",0,0xff); // 8 bits
    uif.addHexSpinnerToGroup(tn[nt],gn[ng],"MCST address","mcst_addr",0,0xff); // 8 bits

    gn.append("Readout"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addPopupToGroup(tn[nt],gn[ng],"Vme Access Mode","vme_mode",
                        (QStringList()
                         << "Single Reads"
                         << "DMA 32bit"
                         << "FIFO Reads"
                         << "Block Transfer 32bit"
                         << "Block Transfer 64bit"
                         << "VME2E accelerated mode"));

    // TAB Control
    tn.append("Ctrl"); nt++; uif.addTab(tn[nt]);
    gn.append("Control"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addUnnamedGroupToGroup(tn[nt],gn[ng],"b0_");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b0_","Start","start_button");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b0_","Stop","stop_button");
    uif.addUnnamedGroupToGroup(tn[nt],gn[ng],"b1_");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b1_","Readout Reset","readout_reset_button");
    uif.addUnnamedGroupToGroup(tn[nt],gn[ng],"b2_");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b2_","Reset","reset_button");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b2_","FIFO Reset","fifo_reset_button");
    uif.addUnnamedGroupToGroup(tn[nt],gn[ng],"b3_");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b3_","Configure","configure_button");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b3_","Single Shot","singleshot_button");

    // TAB INPUT/OUTPUT Config
    tn.append("I/O"); nt++; uif.addTab(tn[nt]);
    gn.append("Input"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Negative edge","negative_edge",0,3); // 2 bits

    gn.append("ECL Outputs"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"Terminate Trig 0 In","enable_termination_input_trig0");
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"Terminate Trig 1 In","enable_termination_input_trig1");
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"Terminate Reset In","enable_termination_input_res");
    uif.addPopupToGroup(tn[nt],gn[ng],"Trig1 Mode","ecl_trig1_mode",(QStringList()
                               << "Trigger input"
                               << "Oscillator"));
    uif.addPopupToGroup(tn[nt],gn[ng],"ECL Out Mode","ecl_out_mode",(QStringList()
                               << "Busy"
                               << "Data above threshold"
                               << "Events above threshold"));
    uif.addPopupToGroup(tn[nt],gn[ng],"Trig Select Mode","trig_select_mode",(QStringList()
                               << "NIM"
                               << "ECL"));
    gn.append("NIM Outputs"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addPopupToGroup(tn[nt],gn[ng],"Trig1 Mode","nim_trig1_mode",(QStringList()
                               << "Trigger"
                               << "Oscillator"));
    uif.addPopupToGroup(tn[nt],gn[ng],"Busy Mode","nim_busy_mode",(QStringList()
                               << "Busy"
                               << "CBus Output"
                               << "Data over threshold"));
    gn.append("Trigger"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank 1 window start","bank0_win_start",0,0x7FFF); // 15 bits
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank 2 window start","bank1_win_start",0,0x7FFF); // 15 bits
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank 1 window width","bank0_win_width",0,0x3FFF); // 14 bits
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank 2 window width","bank1_win_width",0,0x3FFF); // 14 bits
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank 1 trigger source","bank0_trig_source",0,0x3FF); // 10 bits
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank 2 trigger source","bank1_trig_source",0,0x3FF); // 10 bits
    uif.addSpinnerToGroup(tn[nt],gn[ng],"First Hit","only_first_hit",0,3); // 2 bits

    // TAB Channels
    int ch = 0;
    int nofChPerTab = 4;
    int nofRows = 8;

        tn.append("Channels"); nt++; uif.addTab(tn[nt]);

        for(int i=0; i<nofRows; i++)
        {
            QString un = tr("noname_%1").arg(i);
            gn.append(un); ng++; uif.addUnnamedGroupToTab(tn[nt],gn[ng]);
            for(int j=0; j<nofChPerTab; j++)
            {
                gn.append(tr("Channel %1").arg(ch)); ng++; uif.addGroupToGroup(tn[nt],un,gn[ng],tr("enable_channel%1").arg(ch));
                ch++;
            }
        }

    // TAB Clock and IRQ
    tn.append("Clock/IRQ"); nt++; uif.addTab(tn[nt]);

    gn.append("Clock"); ng++; uif.addGroupToTab(tn[nt],gn[ng]);
    uif.addPopupToGroup(tn[nt],gn[ng],"Timestamp Source","time_stamp_source",(QStringList()
             << "From VME"
             << "External LEMO"));
    uif.addCheckBoxToGroup(tn[nt],gn[ng],"External Timestamp Reset Enable","enable_ext_ts_reset");
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Timestamp Divisor","time_stamp_divisor",0,65535); // 16 bits

    gn.append("Interrupt Setup"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addSpinnerToGroup(tn[nt],gn[ng],"IRQ level","irq_level",0,7);
    uif.addHexSpinnerToGroup(tn[nt],gn[ng],"IRQ vector","irq_vector",0,0xff);
    uif.addUnnamedGroupToGroup(tn[nt],gn[ng],"b0_");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b0_","IRQ Test","irq_test_button");
    uif.addButtonToGroup(tn[nt],gn[ng]+"b0_","IRQ Reset","irq_reset_button");
    uif.addSpinnerToGroup(tn[nt],gn[ng],"IRQ threshold","irq_threshold",0,0x7fff); // 15 bit
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Maximum amount of transfer data","max_transfer_data",0,0x7fff); // 15 bit

    // TAB Counters
    tn.append("Counters"); nt++; uif.addTab(tn[nt]);

    gn.append("Counters"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Event counter","event_counter","0");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Timestamp Counter","timestamp_counter","0");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Time","time","0");
    uif.addButtonToGroup(tn[nt],gn[ng],"Update","counter_update_button");
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank0 multiplicity upper limit","high_limit0",0,255);
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank0 multiplicity lower limit","low_limit0",0,255);
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank1 multiplicity upper limit","high_limit1",0,255);
    uif.addSpinnerToGroup(tn[nt],gn[ng],"Bank1 multiplicity lower limit","low_limit1",0,255);

    // TAB RCBus
    tn.append("RCBus"); nt++; uif.addTab(tn[nt]);
    gn.append("Read"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addLineEditToGroup(tn[nt],gn[ng],"Address","rc_addr_read","");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Data","rc_data_read","");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Return Status","rc_status_read","");
    uif.addButtonToGroup(tn[nt],gn[ng],"Read","rc_read_button");
    gn.append("Write"); ng++; uif.addGroupToTab(tn[nt],gn[ng],"","v");
    uif.addLineEditToGroup(tn[nt],gn[ng],"Address","rc_addr_write","");
    uif.addLineEditToGroup(tn[nt],gn[ng],"Data","rc_data_write","");
    uif.addLineEditReadOnlyToGroup(tn[nt],gn[ng],"Return Status","rc_status_write","");
    uif.addButtonToGroup(tn[nt],gn[ng],"Write","rc_write_button");

    //###

    l->addWidget(dynamic_cast<QWidget*>(&tabs));

    QWidget* bottomButtons = new QWidget(this);
    {
    QHBoxLayout* l = new QHBoxLayout();
    QPushButton* previewButton = new QPushButton("Preview");
    connect(previewButton,SIGNAL(clicked()),this,SLOT(clicked_previewButton()));
    l->addWidget(previewButton);
    singleShotPreviewButton = new QPushButton("Singleshot");
    startStopPreviewButton = new QPushButton("Start");
    startStopPreviewButton->setCheckable(true);
    connect(singleShotPreviewButton,SIGNAL(clicked()),this,SLOT(clicked_singleshot_button()));
    connect(startStopPreviewButton,SIGNAL(clicked()),this,SLOT(clicked_startStopPreviewButton()));
    l->addWidget(singleShotPreviewButton);
    l->addWidget(startStopPreviewButton);
    bottomButtons->setLayout(l);
    }
    l->addWidget(bottomButtons);

    this->setLayout(l);
    connect(uif.getSignalMapper(),SIGNAL(mapped(QString)),this,SLOT(uiInput(QString)));

//    QList<QWidget*> li = this->findChildren<QWidget*>();
//    foreach(QWidget* w, li)
//    {
//        printf("%s\n",w->objectName().toStdString().c_str());
//    }
}

void MesytecMtdc32UI::createPreviewUI()
{
    int nofCh = MTDC32V2_NUM_CHANNELS;
    int nofCols = nofCh/sqrt(nofCh);
    int nofRows = nofCh/nofCols;

    if(nofCols*nofRows < nofCh) ++nofCols;


    QTabWidget* t = new QTabWidget();
    QGridLayout* l = new QGridLayout();

    // Histograms
    int ch = 0;
    QWidget* rw = new QWidget();
    {
        QGridLayout* li = new QGridLayout();
        for(int r = 0; r < nofRows; ++r) {
            for(int c = 0; c < nofCols; ++c) {
                if(ch < nofCh) {
                    previewCh[ch] = new plot2d(this,QSize(160,120),ch);
                    previewCh[ch]->addChannel(0,"raw",previewData[ch],QColor(Qt::blue),Channel::line,1);
                    li->addWidget(previewCh[ch],r,c,1,1);
                }
                ++ch;
            }
        }
        rw->setLayout(li);
    }
    // Value displays
    ch = 0;
    QWidget* vw = new QWidget();
    {
        QGridLayout* li = new QGridLayout();
        for(int r = 0; r < nofRows; ++r) {
            for(int c = 0; c < nofCols; ++c) {
                if(ch < nofCh) {
                    QGroupBox* b = new QGroupBox(tr("Ch: %1").arg(ch));
                    {
                        QGridLayout* lb = new QGridLayout();
                        lb->setSpacing(0);
                        lb->setMargin(0);
                        energyValueDisplay[ch] = new QLabel();
                        timestampDisplay[ch] = new QLabel();
                        resolutionDisplay[ch] = new QLabel();
                        flagDisplay[ch] = new QLabel();
                        moduleIdDisplay[ch] = new QLabel();

                        lb->addWidget(new QLabel("Module ID:"),0,0,1,1);
                        lb->addWidget(moduleIdDisplay[ch],0,1,1,1);
                        lb->addWidget(new QLabel("Flags:"),1,0,1,1);
                        lb->addWidget(flagDisplay[ch],1,1,1,1);
                        lb->addWidget(new QLabel("TDC Resolution:"),2,0,1,1);
                        lb->addWidget(resolutionDisplay[ch],2,1,1,1);
                        lb->addWidget(new QLabel("Timestamp:"),3,0,1,1);
                        lb->addWidget(timestampDisplay[ch],3,1,1,1);
                        lb->addWidget(new QLabel("Energy:"),4,0,1,1);
                        lb->addWidget(energyValueDisplay[ch],4,1,1,1);

                        b->setLayout(lb);
                    }
                    li->addWidget(b,r,c,1,1);
                }
                ++ch;
            }
        }
        vw->setLayout(li);
    }

    t->addTab(rw,"Histograms");
    t->addTab(vw,"Values");
    l->addWidget(t);

    previewWindow.setLayout(l);
    previewWindow.setWindowTitle("MTDC Preview");
    previewWindow.resize(640,480);
}

// Slot handling

void MesytecMtdc32UI::uiInput(QString _name)
{
    if(applyingSettings == true) return;

    QGroupBox* gb = findChild<QGroupBox*>(_name);
    if(gb != 0)
    {
        if(_name.startsWith("enable_channel")) {
            QRegExp reg("[0-9]{1,2}");
            reg.indexIn(_name);
            int ch = reg.cap().toInt();
            if(gb->isChecked()) module->conf_.enable_channel[ch] = true;
            else module->conf_.enable_channel[ch] = false;
            printf("Changed enable_channel %d\n",ch); fflush(stdout);
        }
    }

    QCheckBox* cb = findChild<QCheckBox*>(_name);
    if(cb != 0)
    {
        if(_name == "cblt_active")                         {
                                                           module->conf_.cblt_active = cb->isChecked();
                                                           if (cb->isChecked())
                                                               module->conf_.cblt_mcst_ctrl |=
                                                               (1 << MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_CBLT);
                                                           else module->conf_.cblt_mcst_ctrl |=
                                                               (1 << MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_CBLT);
                                                           }
        if(_name == "mcst_active")                         {
                                                           module->conf_.mcst_active = cb->isChecked();
                                                           if (cb->isChecked())
                                                               module->conf_.cblt_mcst_ctrl |=
                                                               (1 << MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_MCST);
                                                           else module->conf_.cblt_mcst_ctrl |=
                                                               (1 << MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_MCST);
                                                           }
        if(_name == "enable_different_eob_marker")         module->conf_.enable_different_eob_marker = cb->isChecked();
        if(_name == "enable_compare_with_max")             module->conf_.enable_compare_with_max = cb->isChecked();
        if(_name == "enable_termination_input_trig0")      module->conf_.enable_termination_input_trig0 = cb->isChecked();
        if(_name == "enable_termination_input_trig1")      module->conf_.enable_termination_input_trig1 = cb->isChecked();
        if(_name == "enable_termination_input_res")        module->conf_.enable_termination_input_res = cb->isChecked();
        if(_name == "enable_ext_ts_reset")                 module->conf_.enable_ext_ts_reset = cb->isChecked();
        //QMessageBox::information(this,"uiInput","You changed the checkbox "+_name);
    }

    QComboBox* cbb = findChild<QComboBox*>(_name);
    if(cbb != 0)
    {
        if(_name == "addr_source")                         module->conf_.addr_source = static_cast<MesytecMtdc32ModuleConfig::AddressSource>(cbb->currentIndex());
        if(_name == "data_length_format")                  module->conf_.data_length_format = static_cast<MesytecMtdc32ModuleConfig::DataLengthFormat>(cbb->currentIndex());
        if(_name == "multi_event_mode")                    module->conf_.multi_event_mode = static_cast<MesytecMtdc32ModuleConfig::MultiEventMode>(cbb->currentIndex());
        if(_name == "tdc_resolution")                      module->conf_.tdc_resolution = cbb->currentIndex();
        if(_name == "output_format")                       module->conf_.output_format = static_cast<MesytecMtdc32ModuleConfig::OutputFormat>(cbb->currentIndex());
        if(_name == "ecl_trig1_mode")                      module->conf_.ecl_trig1_mode = static_cast<MesytecMtdc32ModuleConfig::EclTrig1Mode>(cbb->currentIndex());
        if(_name == "ecl_out_mode")                        {
                                                           if(cbb->currentIndex()==0)
                                                               module->conf_.ecl_out_mode = 0;
                                                           else if(cbb->currentIndex()==1)
                                                               module->conf_.ecl_out_mode = 8;
                                                           else if(cbb->currentIndex()==2)
                                                               module->conf_.ecl_out_mode = 9;
                                                           }
        if(_name == "trig_select_mode")                    module->conf_.trig_select_mode = static_cast<MesytecMtdc32ModuleConfig::TrigSelectMode>(cbb->currentIndex());
        if(_name == "nim_trig1_mode")                      module->conf_.nim_trig1_mode = static_cast<MesytecMtdc32ModuleConfig::NimTrig1Mode>(cbb->currentIndex());
        if(_name == "nim_busy_mode")                       {
                                                           if(cbb->currentIndex()==0)
                                                               module->conf_.nim_busy_mode = 0;
                                                           else if(cbb->currentIndex()==1)
                                                               module->conf_.nim_busy_mode = 3;
                                                           else if(cbb->currentIndex()==2)
                                                               module->conf_.nim_busy_mode = 8;
                                                           }
        if(_name == "test_pulser_mode")                    module->conf_.test_pulser_mode = cbb->currentIndex();
        if(_name == "time_stamp_source")                   module->conf_.time_stamp_source = static_cast<MesytecMtdc32ModuleConfig::TimeStampSource>(cbb->currentIndex());



        if(_name == "vme_mode") {
            module->conf_.vme_mode = static_cast<MesytecMtdc32ModuleConfig::VmeMode>(cbb->currentIndex());
            std::cout << "Changed vme_mode to" << module->conf_.vme_mode << std::endl;
        }







        if(_name == "marking_type")                        {
                                                            switch(cbb->currentIndex()) {
                                                            case 0:
                                                                module->conf_.marking_type = MesytecMtdc32ModuleConfig::mtEventCounter;
                                                                break;
                                                            case 1:
                                                                module->conf_.marking_type = MesytecMtdc32ModuleConfig::mtTimestamp;
                                                                break;
                                                            case 2:
                                                                module->conf_.marking_type = MesytecMtdc32ModuleConfig::mtExtendedTs;
                                                                break;
                                                            default:
                                                                module->conf_.marking_type = MesytecMtdc32ModuleConfig::mtEventCounter;
                                                                break;
                                                            }
                                                           }
        if(_name == "bank_operation")                      {
                                                                switch(cbb->currentIndex()) {
                                                                case 0:
                                                                    module->conf_.bank_operation = MesytecMtdc32ModuleConfig::boConnected;
                                                                    break;
                                                                case 1:
                                                                    module->conf_.bank_operation = MesytecMtdc32ModuleConfig::boIndependent;
                                                                    break;
                                                                default:
                                                                    module->conf_.bank_operation = MesytecMtdc32ModuleConfig::boConnected;
                                                                    break;
                                                                }
                                                            }


          //QMessageBox::information(this,"uiInput","You changed the combobox "+_name);
    }
    QSpinBox* sb = findChild<QSpinBox*>(_name);
    if(sb != 0)
    {
        if(_name == "base_addr_register")                  module->conf_.base_addr_register = sb->value();
        if(_name == "module_id")                           module->conf_.module_id = sb->value();
        if(_name == "irq_level")                           module->conf_.irq_level = sb->value();
        if(_name == "irq_vector")                          module->conf_.irq_vector = sb->value();
        if(_name == "irq_threshold")                       module->conf_.irq_threshold = sb->value();
        if(_name == "max_transfer_data")                   module->conf_.max_transfer_data= sb->value();
        if(_name == "cblt_addr")                           module->conf_.cblt_addr=sb->value();
        if(_name == "mcst_addr")                           module->conf_.mcst_addr=sb->value();
        if(_name == "bank0_win_start")                     module->conf_.bank0_win_start = sb->value();
        if(_name == "bank1_win_start")                     module->conf_.bank1_win_start = sb->value();
        if(_name == "bank0_win_width")                     module->conf_.bank0_win_width = sb->value();
        if(_name == "bank1_win_width")                     module->conf_.bank1_win_width = sb->value();
        if(_name == "bank0_trig_source")                   module->conf_.bank0_trig_source = sb->value();
        if(_name == "bank1_trig_source")                   module->conf_.bank1_trig_source = sb->value();
        if(_name == "only_first_hit")                      module->conf_.only_first_hit = sb->value();
        if(_name == "negative_edge")                       module->conf_.negative_edge = sb->value();
        if(_name == "pulser_pattern")                      module->conf_.pulser_pattern = sb->value();
        if(_name == "bank0_input_thr")                     module->conf_.bank0_input_thr = sb->value();
        if(_name == "bank1_input_thr")                     module->conf_.bank1_input_thr = sb->value();

        if(_name == "time_stamp_divisor"){
            module->conf_.time_stamp_divisor = sb->value();
        }



        if(_name == "high_limit0")
                module->conf_.high_limit0 = sb->value();
        if(_name == "low_limit0")
                module->conf_.low_limit0 = sb->value();
        if(_name == "high_limit1")
                module->conf_.high_limit1 = sb->value();
        if(_name == "low_limit1")
                module->conf_.low_limit1 = sb->value();






    }
    QRadioButton* rb = findChild<QRadioButton*>(_name);
    if(rb != 0)
    {
        if(_name == "enable_cblt_first" && rb->isChecked()){
                                                           module->conf_.cblt_mcst_ctrl &=
                                                           ~(1 << MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_FIRST_MODULE)|
                                                           (1 << MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_LAST_MODULE);
                                                           module->conf_.enable_cblt_first =  true;
                                                           module->conf_.enable_cblt_last =   false;
                                                           module->conf_.enable_cblt_middle = false;
                                                           }
        if(_name == "enable_cblt_last" && rb->isChecked()) {
                                                           module->conf_.cblt_mcst_ctrl &=
                                                           ~(1 << MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_LAST_MODULE)|
                                                           (1 << MTDC32V2_OFF_CBLT_MCST_CTRL_DISABLE_FIRST_MODULE);
                                                           module->conf_.enable_cblt_first =  false;
                                                           module->conf_.enable_cblt_last =   true;
                                                           module->conf_.enable_cblt_middle = false;
                                                           }
        if(_name =="enable_cblt_middle" && rb->isChecked()){
                                                           module->conf_.cblt_mcst_ctrl &=
                                                           ~((1 << MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_FIRST_MODULE)
                                                           |(1 << MTDC32V2_OFF_CBLT_MCST_CTRL_ENABLE_LAST_MODULE));
                                                           module->conf_.enable_cblt_first =  false;
                                                           module->conf_.enable_cblt_last =   false;
                                                           module->conf_.enable_cblt_middle = true;
                                                           }
    }
    QPushButton* pb = findChild<QPushButton*>(_name);
    if(pb != 0)
    {
        if(_name == "irq_test_button")                     clicked_irqtestbutton();
        if(_name == "irq_reset_button")                    clicked_irqresetbutton();
        if(_name == "start_button")                        clicked_start_button();
        if(_name == "stop_button")                         clicked_stop_button();
        if(_name == "reset_button") clicked_reset_button();
        if(_name == "fifo_reset_button") clicked_fifo_reset_button();
        if(_name == "readout_reset_button") clicked_readout_reset_button();
        if(_name == "configure_button") clicked_configure_button();
        if(_name == "counter_update_button") clicked_counter_update_button();
        if(_name == "singleshot_button") clicked_singleshot_button();
        if(_name == "update_firmware_button")              clicked_update_firmware_button();
    }

}


void MesytecMtdc32UI::clicked_irqtestbutton()
{
    module->irqTest();
}

void MesytecMtdc32UI::clicked_irqresetbutton()
{
    module->irqReset();
}

void MesytecMtdc32UI::clicked_start_button()
{
    module->startAcquisition();
}

void MesytecMtdc32UI::clicked_stop_button()
{
    module->stopAcquisition();
}

void MesytecMtdc32UI::clicked_reset_button()
{
    module->reset();
}

void MesytecMtdc32UI::clicked_fifo_reset_button()
{
    module->fifoReset();
}

void MesytecMtdc32UI::clicked_readout_reset_button()
{
    module->readoutReset();
}

void MesytecMtdc32UI::clicked_configure_button()
{
    module->configure();
}

void MesytecMtdc32UI::clicked_counter_update_button()
{
    std::cout << "Clicked counter_update_button" << std::endl;
    uint32_t ev_cnt = module->getEventCounter();
    QLabel* ec = (QLabel*) uif.getWidgets()->find("event_counter").value();
    ec->setText(tr("%1").arg(ev_cnt,2,10));
    std::cout << "Event counter value: " << ev_cnt << std::endl;
}

void MesytecMtdc32UI::clicked_singleshot_button()
{
    if(!module->getInterface()) {
        return;
    }
    if(!module->getInterface()->isOpen()){
        if(0 != module->getInterface()->open()) {
            QMessageBox::warning (this, tr ("<%1> SIS3302 ADC").arg (module->getName()), tr ("Could not open interface"), QMessageBox::Ok);
            return;
        }
    }
    uint32_t data[MTDC32V2_LEN_EVENT_MAX];
    uint32_t rd = 0;
    module->singleShot(data,&rd);
    //printf("MesytecMtdc32UI: singleShot: Read %d words:\n",rd);

    updatePreview();

    if(previewRunning) {
        previewTimer->start();
    }
}

void MesytecMtdc32UI::clicked_update_firmware_button()
{
    module->updateModuleInfo();
    HexSpinBox* m = (HexSpinBox*) uif.getWidgets()->find("module_id").value();
    QLabel* f = (QLabel*) uif.getWidgets()->find("firmware").value();
    f->setText(tr("%1.%2").arg(module->conf_.firmware_revision_major,2,16,QChar('0')).arg(module->conf_.firmware_revision_minor,2,16,QChar('0')));
    m->setValue(module->conf_.module_id);
}

void MesytecMtdc32UI::clicked_previewButton()
{
    if(previewWindow.isHidden())
    {
        previewWindow.show();
    }
    else
    {
        previewWindow.hide();
    }
}

void MesytecMtdc32UI::clicked_startStopPreviewButton()
{
    if(previewRunning) {
        // Stopping
        startStopPreviewButton->setText("Start");
        previewTimer->stop();
        previewRunning = false;
    } else {
        // Starting
        // precondition check
        if(!module->getInterface()) {
            return;
        }
        if(!module->getInterface()->isOpen()){
            if(0 != module->getInterface()->open()) {
                QMessageBox::warning (this, tr ("<%1> SIS3302 ADC").arg (module->getName()), tr ("Could not open interface"), QMessageBox::Ok);
                return;
            }
        }

        startStopPreviewButton->setText("Stop");
        if(previewWindow.isHidden()) {
            previewWindow.show();
        }
        previewRunning = true;
        previewTimer->start();
        connect(previewTimer,SIGNAL(timeout()),this,SLOT(timeout_previewTimer()));
    }
}

void MesytecMtdc32UI::updatePreview()
{
    //std::cout << "MesytecMtdc32UI::updatePreview" << std::endl << std::flush;
    for(int ch = 0; ch < MTDC32V2_NUM_CHANNELS; ++ch) {
        if(module->conf_.enable_channel[ch]) {
            // Values
            moduleIdDisplay[ch]->setText(tr("0x%1").arg(module->getModuleIdConfigured(),4,16));
            QString flagString = "";
            flagDisplay[ch]->setText(flagString);
            timestampDisplay[ch]->setText(tr("0x%1").arg(module->current_time_stamp,16,16));
            energyValueDisplay[ch]->setText(tr("%1").arg(module->current_energy[ch]));
            resolutionDisplay[ch]->setText(tr("%1").arg(module->current_resolution));

            // HIST data
            int nof_bins = 1024;
            previewData[ch].resize(nof_bins);
            //printf("Channel size: %d\n",previewData[ch].size());
            int current_bin = ((double)(nof_bins)/8192.)*((double)(module->current_energy[ch]) );
            // printf("Current bin: %d \n",current_bin);
            if(current_bin > 0 && current_bin < nof_bins) {
                previewData[ch][current_bin]++;
            }

            {
                QWriteLocker lck(previewCh[ch]->getChanLock());
                previewCh[ch]->getChannelById(0)->setData(previewData[ch]);
            }
            previewCh[ch]->update();
        }
    }
}

void MesytecMtdc32UI::timeout_previewTimer() {
    if(previewRunning) {
        clicked_singleshot_button();
    }
}

// Settings handling

void MesytecMtdc32UI::applySettings()
{
    applyingSettings = true;

    QList<QGroupBox*> gbs = findChildren<QGroupBox*>();
    if(!gbs.empty())
    {
        QList<QGroupBox*>::const_iterator it = gbs.begin();
        while(it != gbs.end())
        {
            QGroupBox* w = (*it);
            for(int ch=0; ch < MTDC32V2_NUM_CHANNELS; ch++) {
                if(w->objectName() == tr("enable_channel%1").arg(ch)) w->setChecked(module->conf_.enable_channel[ch]);
            }
            it++;
        }
    }
    QList<QCheckBox*> cbs = findChildren<QCheckBox*>();
    if(!cbs.empty())
    {
        QList<QCheckBox*>::const_iterator it = cbs.begin();
        while(it != cbs.end())
        {
            QCheckBox* w = (*it);

            if(w->objectName() == "cblt_active")           w->setChecked(module->conf_.cblt_active);
            if(w->objectName() == "mcst_active")           w->setChecked(module->conf_.mcst_active);
            if(w->objectName() == "enable_different_eob_marker") w->setChecked(module->conf_.enable_different_eob_marker);
            if(w->objectName() == "enable_compare_with_max") w->setChecked(module->conf_.enable_compare_with_max);
            if(w->objectName() == "enable_termination_input_trig0") w->setChecked(module->conf_.enable_termination_input_trig0);
            if(w->objectName() == "enable_termination_input_trig1") w->setChecked(module->conf_.enable_termination_input_trig1);
            if(w->objectName() == "enable_termination_input_res") w->setChecked(module->conf_.enable_termination_input_res);
            if(w->objectName() == "enable_ext_ts_reset")   w->setChecked(module->conf_.enable_ext_ts_reset);

            it++;
        }
    }
    QList<QComboBox*> cbbs = findChildren<QComboBox*>();
    if(!cbbs.empty())
    {
        QList<QComboBox*>::const_iterator it = cbbs.begin();
        while(it != cbbs.end())
        {
            QComboBox* w = (*it);
            //printf("Found combobox with the name %s\n",w->objectName().toStdString().c_str());
            if(w->objectName() == "addr_source")           w->setCurrentIndex(module->conf_.addr_source);
            if(w->objectName() == "data_length_format")    w->setCurrentIndex(module->conf_.data_length_format);
            if(w->objectName() == "multi_event_mode")      w->setCurrentIndex(module->conf_.multi_event_mode);
            if(w->objectName() == "marking_type")          w->setCurrentIndex(module->conf_.marking_type);
            if(w->objectName() == "bank_operation")        w->setCurrentIndex(module->conf_.bank_operation);
            if(w->objectName() == "tdc_resolution")        w->setCurrentIndex(module->conf_.tdc_resolution);
            if(w->objectName() == "output_format")         w->setCurrentIndex(module->conf_.output_format);
            if(w->objectName() == "ecl_trig1_mode")        w->setCurrentIndex(module->conf_.ecl_trig1_mode);
            if(w->objectName() == "ecl_out_mode")          {
                                                           if(module->conf_.ecl_out_mode==0)
                                                               w->setCurrentIndex(0);
                                                           else if(module->conf_.ecl_out_mode==8)
                                                               w->setCurrentIndex(1);
                                                           else if(module->conf_.ecl_out_mode==9)
                                                               w->setCurrentIndex(2);
                                                           }
            if(w->objectName() == "trig_select_mode")      w->setCurrentIndex(module->conf_.trig_select_mode);
            if(w->objectName() == "nim_trig1_mode")        w->setCurrentIndex(module->conf_.nim_trig1_mode);
            if(w->objectName() == "nim_busy_mode")          {
                                                           if(module->conf_.nim_busy_mode==0)
                                                               w->setCurrentIndex(0);
                                                           else if(module->conf_.nim_busy_mode==3)
                                                               w->setCurrentIndex(1);
                                                           else if(module->conf_.nim_busy_mode==8)
                                                               w->setCurrentIndex(2);
                                                           }
            if(w->objectName() == "testpulser_mode")       w->setCurrentIndex(module->conf_.test_pulser_mode);
            if(w->objectName() == "pulser_pattern")        w->setCurrentIndex(module->conf_.pulser_pattern);
            if(w->objectName() == "bank0_input_thr")       w->setCurrentIndex(module->conf_.bank0_input_thr);
            if(w->objectName() == "bank1_input_thr")       w->setCurrentIndex(module->conf_.bank1_input_thr);
            if(w->objectName() == "time_stamp_source")     w->setCurrentIndex(module->conf_.time_stamp_source);



            if(w->objectName() == "vme_mode") w->setCurrentIndex(module->conf_.vme_mode);










            if(w->objectName() == "high_limit0") w->setCurrentIndex(module->conf_.high_limit0);
            if(w->objectName() == "low_limit0") w->setCurrentIndex(module->conf_.low_limit0);
            if(w->objectName() == "high_limit1") w->setCurrentIndex(module->conf_.high_limit1);
            if(w->objectName() == "low_limit1") w->setCurrentIndex(module->conf_.low_limit1);

            it++;
        }
    }
    QList<QSpinBox*> csb = findChildren<QSpinBox*>();
    if(!csb.empty())
    {
        QList<QSpinBox*>::const_iterator it = csb.begin();
        while(it != csb.end())
        {
            QSpinBox* w = (*it);
            //printf("Found spinbox with the name %s\n",w->objectName().toStdString().c_str());
            if(w->objectName() == "base_addr_register")    w->setValue(module->conf_.base_addr_register);
            if(w->objectName() == "module_id")             w->setValue(module->conf_.module_id);
            if(w->objectName() == "irq_level")             w->setValue(module->conf_.irq_level);
            if(w->objectName() == "irq_vector")            w->setValue(module->conf_.irq_vector);
            if(w->objectName() == "irq_threshold")         w->setValue(module->conf_.irq_threshold);
            if(w->objectName() == "max_transfer_data")     w->setValue(module->conf_.max_transfer_data);
            if(w->objectName() == "cblt_addr")             w->setValue(module->conf_.cblt_addr);
            if(w->objectName() == "mcst_addr")             w->setValue(module->conf_.mcst_addr);
            if(w->objectName() == "bank0_win_start")       w->setValue(module->conf_.bank0_win_start);
            if(w->objectName() == "bank1_win_start")       w->setValue(module->conf_.bank1_win_start);
            if(w->objectName() == "bank0_win_width")       w->setValue(module->conf_.bank0_win_width);
            if(w->objectName() == "bank1_win_width")       w->setValue(module->conf_.bank1_win_width);
            if(w->objectName() == "bank0_trig_source")     w->setValue(module->conf_.bank0_trig_source);
            if(w->objectName() == "bank1_trig_source")     w->setValue(module->conf_.bank1_trig_source);
            if(w->objectName() == "only_first_hit")        w->setValue(module->conf_.only_first_hit);
            if(w->objectName() == "negative_edge")         w->setValue(module->conf_.negative_edge);

if(w->objectName() == "time_stamp_divisor") w->setValue(module->conf_.time_stamp_divisor);
            it++;
        }
    }
    QList<QRadioButton*> crb = findChildren<QRadioButton*>();
    if(!crb.empty())
    {
        QList<QRadioButton*>::const_iterator it = crb.begin();
        while(it != crb.end())
        {
            QRadioButton* w = (*it);
            if(w->objectName() == "enable_cblt_first")     w->setChecked(module->conf_.enable_cblt_first);
            if(w->objectName() == "enable_cblt_last")      w->setChecked(module->conf_.enable_cblt_last);
            if(w->objectName() == "enable_cblt_middle")    w->setChecked(module->conf_.enable_cblt_middle);
            it++;
        }
    }
\
    QLabel* b_addr = (QLabel*) uif.getWidgets()->find("base_addr").value();
    b_addr->setText(tr("0x%1").arg(module->conf_.base_addr,8,16,QChar('0')));

    applyingSettings = false;
}


