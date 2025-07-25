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
#include <QStyle>
#include <QApplication>
#include <QLabel>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Settings");
    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon));

    // --- Appearance Group ---
    QGroupBox* appearanceGroup = new QGroupBox("Appearance");
    appearanceGroup->setIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
    QFormLayout* appearanceLayout = new QFormLayout();

    // Layout Orientation
    QHBoxLayout* orientationLayout = new QHBoxLayout();
    QLabel* orientationIcon = new QLabel();
    orientationIcon->setPixmap(style()->standardIcon(QStyle::SP_FileDialogDetailedView).pixmap(16, 16));
    m_layoutOrientationComboBox = new QComboBox();
    m_layoutOrientationComboBox->addItems({"Vertical", "Horizontal"});
    orientationLayout->addWidget(orientationIcon);
    orientationLayout->addWidget(m_layoutOrientationComboBox);
    orientationLayout->addStretch();
    QWidget* orientationWidget = new QWidget();
    orientationWidget->setLayout(orientationLayout);
    appearanceLayout->addRow("Layout Orientation:", orientationWidget);

    // Font Size
    QHBoxLayout* fontSizeLayout = new QHBoxLayout();
    QLabel* fontSizeIcon = new QLabel();
    fontSizeIcon->setPixmap(style()->standardIcon(QStyle::SP_FileDialogDetailedView).pixmap(16, 16));
    m_fontSizeSpinBox = new QSpinBox();
    m_fontSizeSpinBox->setRange(8, 24);
    fontSizeLayout->addWidget(fontSizeIcon);
    fontSizeLayout->addWidget(m_fontSizeSpinBox);
    fontSizeLayout->addStretch();
    QWidget* fontSizeWidget = new QWidget();
    fontSizeWidget->setLayout(fontSizeLayout);
    appearanceLayout->addRow("Font Size:", fontSizeWidget);

    // Font Color
    QHBoxLayout* fontColorLayout = new QHBoxLayout();
    QLabel* fontColorIcon = new QLabel();
    fontColorIcon->setPixmap(style()->standardIcon(QStyle::SP_DialogYesButton).pixmap(16, 16));
    m_fontColorButton = new QPushButton("Choose Color...");
    m_fontColorButton->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    connect(m_fontColorButton, &QPushButton::clicked, this, &SettingsDialog::chooseFontColor);
    fontColorLayout->addWidget(fontColorIcon);
    fontColorLayout->addWidget(m_fontColorButton);
    fontColorLayout->addStretch();
    QWidget* fontColorWidget = new QWidget();
    fontColorWidget->setLayout(fontColorLayout);
    appearanceLayout->addRow("Font Color:", fontColorWidget);

    // Background Color
    QHBoxLayout* bgColorLayout = new QHBoxLayout();
    QLabel* bgColorIcon = new QLabel();
    bgColorIcon->setPixmap(style()->standardIcon(QStyle::SP_DialogNoButton).pixmap(16, 16));
    m_backgroundColorButton = new QPushButton("Choose Color...");
    m_backgroundColorButton->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::chooseBackgroundColor);
    bgColorLayout->addWidget(bgColorIcon);
    bgColorLayout->addWidget(m_backgroundColorButton);
    bgColorLayout->addStretch();
    QWidget* bgColorWidget = new QWidget();
    bgColorWidget->setLayout(bgColorLayout);
    appearanceLayout->addRow("Background Color:", bgColorWidget);

    // Background Opacity
    QHBoxLayout* opacityLayout = new QHBoxLayout();
    QLabel* opacityIcon = new QLabel();
    opacityIcon->setPixmap(style()->standardIcon(QStyle::SP_DialogApplyButton).pixmap(16, 16));
    m_backgroundOpacitySpinBox = new QSpinBox();
    m_backgroundOpacitySpinBox->setRange(0, 255);
    m_backgroundOpacitySpinBox->setSuffix(" (0-255)");
    opacityLayout->addWidget(opacityIcon);
    opacityLayout->addWidget(m_backgroundOpacitySpinBox);
    opacityLayout->addStretch();
    QWidget* opacityWidget = new QWidget();
    opacityWidget->setLayout(opacityLayout);
    appearanceLayout->addRow("Background Opacity:", opacityWidget);
    
    appearanceGroup->setLayout(appearanceLayout);

    // --- Display Group ---
    QGroupBox* displayGroup = new QGroupBox("Displayed Information");
    displayGroup->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    QVBoxLayout* displayLayout = new QVBoxLayout();
    
    // Create checkboxes with icons
    QStringList displayNames = {"CPU Load", "Memory Usage %", "RAM Usage (MB)", "Disk Activity", "GPU Load"};
    QList<QStyle::StandardPixmap> displayIcons = {
        QStyle::SP_ComputerIcon,
        QStyle::SP_DriveHDIcon,
        QStyle::SP_DriveHDIcon,
        QStyle::SP_DriveHDIcon,
        QStyle::SP_ComputerIcon
    };
    
    for (int i = 0; i < displayNames.size(); ++i) {
        QHBoxLayout* checkLayout = new QHBoxLayout();
        QLabel* checkIcon = new QLabel();
        checkIcon->setPixmap(style()->standardIcon(displayIcons[i]).pixmap(16, 16));
        QCheckBox* checkBox = new QCheckBox(displayNames[i]);
        checkLayout->addWidget(checkIcon);
        checkLayout->addWidget(checkBox);
        checkLayout->addStretch();
        QWidget* checkWidget = new QWidget();
        checkWidget->setLayout(checkLayout);
        displayLayout->addWidget(checkWidget);
        m_displayChecks.append(checkBox);
    }
    
    displayGroup->setLayout(displayLayout);

    // --- Behavior Group ---
    QGroupBox* behaviorGroup = new QGroupBox("Behavior");
    behaviorGroup->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    QFormLayout* behaviorLayout = new QFormLayout();
    
    QHBoxLayout* intervalLayout = new QHBoxLayout();
    QLabel* intervalIcon = new QLabel();
    intervalIcon->setPixmap(style()->standardIcon(QStyle::SP_BrowserReload).pixmap(16, 16));
    m_updateIntervalSpinBox = new QSpinBox();
    m_updateIntervalSpinBox->setRange(250, 5000);
    m_updateIntervalSpinBox->setSuffix(" ms");
    m_updateIntervalSpinBox->setSingleStep(100);
    intervalLayout->addWidget(intervalIcon);
    intervalLayout->addWidget(m_updateIntervalSpinBox);
    intervalLayout->addStretch();
    QWidget* intervalWidget = new QWidget();
    intervalWidget->setLayout(intervalLayout);
    behaviorLayout->addRow("Update Interval:", intervalWidget);
    
    behaviorGroup->setLayout(behaviorLayout);

    // --- Dialog Buttons ---
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    
    // Add icons to buttons
    buttonBox->button(QDialogButtonBox::Ok)->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
    buttonBox->button(QDialogButtonBox::Cancel)->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    buttonBox->button(QDialogButtonBox::Apply)->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    
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