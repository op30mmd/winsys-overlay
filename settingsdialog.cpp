#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSettings>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QComboBox>
#include <QGroupBox>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Settings");

    // --- Appearance Group ---
    QGroupBox* appearanceGroup = new QGroupBox("Appearance");
    QFormLayout* appearanceLayout = new QFormLayout();

    m_layoutOrientationComboBox = new QComboBox();
    m_layoutOrientationComboBox->addItems({"Vertical", "Horizontal"});
    appearanceLayout->addRow("Layout Orientation:", m_layoutOrientationComboBox);

    m_fontSizeSpinBox = new QSpinBox();
    m_fontSizeSpinBox->setRange(8, 24);
    appearanceLayout->addRow("Font Size:", m_fontSizeSpinBox);

    m_fontColorButton = new QPushButton("Choose Color...");
    connect(m_fontColorButton, &QPushButton::clicked, this, &SettingsDialog::chooseFontColor);
    appearanceLayout->addRow("Font Color:", m_fontColorButton);

    m_backgroundColorButton = new QPushButton("Choose Color...");
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::chooseBackgroundColor);
    appearanceLayout->addRow("Background Color:", m_backgroundColorButton);

    m_backgroundOpacitySpinBox = new QSpinBox();
    m_backgroundOpacitySpinBox->setRange(0, 255);
    m_backgroundOpacitySpinBox->setSuffix(" (0-255)");
    appearanceLayout->addRow("Background Opacity:", m_backgroundOpacitySpinBox);
    appearanceGroup->setLayout(appearanceLayout);

    // --- Display Group ---
    QGroupBox* displayGroup = new QGroupBox("Displayed Information");
    QVBoxLayout* displayLayout = new QVBoxLayout();
    m_displayChecks.append(new QCheckBox("CPU Load"));
    m_displayChecks.append(new QCheckBox("Memory Usage %"));
    m_displayChecks.append(new QCheckBox("RAM Usage (MB)"));
    m_displayChecks.append(new QCheckBox("Disk Activity"));
    m_displayChecks.append(new QCheckBox("GPU Load"));
    for(auto* check : m_displayChecks) {
        displayLayout->addWidget(check);
    }
    displayGroup->setLayout(displayLayout);

    // --- Behavior Group ---
    QGroupBox* behaviorGroup = new QGroupBox("Behavior");
    QFormLayout* behaviorLayout = new QFormLayout();
    m_updateIntervalSpinBox = new QSpinBox();
    m_updateIntervalSpinBox->setRange(250, 5000);
    m_updateIntervalSpinBox->setSuffix(" ms");
    m_updateIntervalSpinBox->setSingleStep(100);
    behaviorLayout->addRow("Update Interval:", m_updateIntervalSpinBox);
    behaviorGroup->setLayout(behaviorLayout);

    // --- Dialog Buttons ---
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveAndApplySettings);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::saveAndApplySettings);

    // --- Main Layout ---
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(appearanceGroup);
    mainLayout->addWidget(displayGroup);
    mainLayout->addWidget(behaviorGroup);
    mainLayout->addWidget(buttonBox);

    loadSettings();
}

void SettingsDialog::loadSettings()
{
    QSettings s;
    m_layoutOrientationComboBox->setCurrentText(s.value("appearance/layoutOrientation", "Vertical").toString());
    m_fontSizeSpinBox->setValue(s.value("appearance/fontSize", 11).toInt());
    m_backgroundOpacitySpinBox->setValue(s.value("appearance/backgroundOpacity", 120).toInt());
    m_updateIntervalSpinBox->setValue(s.value("behavior/updateInterval", 1000).toInt());

    m_displayChecks[0]->setChecked(s.value("display/showCpu", true).toBool());
    m_displayChecks[1]->setChecked(s.value("display/showMem", true).toBool());
    m_displayChecks[2]->setChecked(s.value("display/showRam", true).toBool());
    m_displayChecks[3]->setChecked(s.value("display/showDisk", true).toBool());
    m_displayChecks[4]->setChecked(s.value("display/showGpu", true).toBool());
}

void SettingsDialog::saveAndApplySettings()
{
    QSettings s;
    s.setValue("appearance/layoutOrientation", m_layoutOrientationComboBox->currentText());
    s.setValue("appearance/fontSize", m_fontSizeSpinBox->value());
    s.setValue("appearance/backgroundOpacity", m_backgroundOpacitySpinBox->value());
    s.setValue("behavior/updateInterval", m_updateIntervalSpinBox->value());

    s.setValue("display/showCpu", m_displayChecks[0]->isChecked());
    s.setValue("display/showMem", m_displayChecks[1]->isChecked());
    s.setValue("display/showRam", m_displayChecks[2]->isChecked());
    s.setValue("display/showDisk", m_displayChecks[3]->isChecked());
    s.setValue("display/showGpu", m_displayChecks[4]->isChecked());

    emit settingsApplied();
}

void SettingsDialog::chooseFontColor()
{
    QSettings s;
    QColor color = QColorDialog::getColor(s.value("appearance/fontColor", QColor(Qt::white)).value<QColor>(), this, "Choose Font Color");
    if (color.isValid()) s.setValue("appearance/fontColor", color);
}

void SettingsDialog::chooseBackgroundColor()
{
    QSettings s;
    QColor color = QColorDialog::getColor(s.value("appearance/backgroundColor", QColor(Qt::black)).value<QColor>(), this, "Choose Background Color");
    if (color.isValid()) s.setValue("appearance/backgroundColor", color);
}