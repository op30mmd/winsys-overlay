#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QColor>

class QCheckBox;
class QSpinBox;
class QPushButton;
class QComboBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void saveAndApplySettings();
    void chooseFontColor();
    void chooseBackgroundColor();

private:
    void loadSettings();

    // UI Elements
    QSpinBox *m_fontSizeSpinBox;
    QPushButton *m_fontColorButton;
    QPushButton *m_backgroundColorButton;
    QSpinBox *m_backgroundOpacitySpinBox;
    QComboBox *m_layoutOrientationComboBox;
    QSpinBox *m_updateIntervalSpinBox;
    QList<QCheckBox*> m_displayChecks;
};

#endif // SETTINGSDIALOG_H