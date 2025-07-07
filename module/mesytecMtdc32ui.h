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

#ifndef MESYTECMTDC32UI_H
#define MESYTECMTDC32UI_H

#include <QWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QMap>
#include <QMessageBox>
#include <QMutex>
#include <QPushButton>
#include <QSignalMapper>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include "plot2d.h"
#include "hexspinbox.h"
#include "geckoui.h"

#include "mesytec_mtdc_32_v2.h"
#include "baseui.h"

class MesytecMtdc32Module;

class MesytecMtdc32UI : public BaseUI
{
    Q_OBJECT

public:
    MesytecMtdc32UI(MesytecMtdc32Module* _module);
    ~MesytecMtdc32UI();

    void applySettings();

protected:
    MesytecMtdc32Module* module;

    // UI
    QTabWidget tabs;    // Tabs widget
    GeckoUiFactory uif; // UI generator factory
    QStringList tn; // Tab names
    QStringList gn; // Group names
    QStringList wn; // WidgetNames

    void createUI(); // Has to be implemented

    bool applyingSettings;
    bool previewRunning;

    // Preview window
    void createPreviewUI();
    QPushButton* startStopPreviewButton;
    QPushButton* singleShotPreviewButton;
    QWidget previewWindow;
    plot2d* previewCh[MTDC32V2_NUM_CHANNELS];
    QLabel* energyValueDisplay[MTDC32V2_NUM_CHANNELS];
    QLabel* timestampDisplay[MTDC32V2_NUM_CHANNELS];
    QLabel* resolutionDisplay[MTDC32V2_NUM_CHANNELS];
    QLabel* flagDisplay[MTDC32V2_NUM_CHANNELS];
    QLabel* moduleIdDisplay[MTDC32V2_NUM_CHANNELS];
    QVector<double> previewData[MTDC32V2_NUM_CHANNELS];
    QTimer* previewTimer;

public slots:
    void uiInput(QString _name);
    void clicked_irqtestbutton();
    void clicked_irqresetbutton();
    void clicked_start_button();
    void clicked_stop_button();
    void clicked_reset_button();
    void clicked_fifo_reset_button();
    void clicked_readout_reset_button();
    void clicked_counter_update_button();
    void clicked_configure_button();
    void clicked_previewButton();
    void clicked_startStopPreviewButton();
    void clicked_singleshot_button();
    void clicked_update_firmware_button();
    void timeout_previewTimer();
    void updatePreview();
};

#endif // MESYTECMTDC32UI_H
