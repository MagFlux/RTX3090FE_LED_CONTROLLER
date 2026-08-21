#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QGroupBox>
#include <QApplication>
#include <QTimer>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QCloseEvent>
#include <QEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include "nvidiacontroller.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(bool startHidden, QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onRGBColorButtonClicked();
    void onRBGBrightnessChanged(int value);
    void onWhiteBrightnessChanged(int value);
    void onLinkToggled(bool checked);
    void onModeChanged(int index);
    void updateColorDisplay();
    void onApplyClicked();
    void onCancelClicked();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onToggleStartAtLogin(bool checked);

private:
    void setupUI();
    void setupConnections();
    void initializeController();
    void setupTrayIcon();
    bool startAtLoginEnabled() const;
    void setStartAtLoginEnabled(bool enabled);

    // UI Elements
    QPushButton* rgbColorButton;
    QSlider* rgbBrightnessSlider;
    QSlider* whiteBrightnessSlider;
    QLabel* colorDisplay;
    QLabel* rgbBrightnessLabel;
    QLabel* whiteBrightnessLabel;
    QCheckBox* linkCheckBox;
    QComboBox* modeComboBox;
    QGroupBox* rgbColorGroup;
    QGroupBox* whiteBrightnessGroup;
    QPushButton* applyButton;
    QPushButton* cancelButton;
    QTabWidget* tabWidget;

    // Startup-on-login checkbox (GUI toggle for the shell:startup shortcut)
    QCheckBox* startAtLoginCheckBox;
    QMenu* trayMenu;
    QAction* trayToggleStartupAction;

    // Guard to stop the link-checkbox mirroring from re-triggering itself
    bool mirroringSliders;

    // True when launched with --background (e.g. from the startup folder)
    bool startedHidden;

    // Tray icon (always present, created in the ctor)
    QSystemTrayIcon* trayIcon;

    // Controller
    NVIDIAController* controller;

    // Current settings
    QColor currentRGBColor;
    int currentRGBBrightness;
    int currentWhiteBrightness;
    int currentMode;
};

#endif // MAINWINDOW_H