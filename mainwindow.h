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
#include "nvidiacontroller.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRGBColorButtonClicked();
    void onRBGBrightnessChanged(int value);
    void onWhiteBrightnessChanged(int value);
    void onLinkToggled(bool checked);
    void onModeChanged(int index);
    void updateColorDisplay();
    void onApplyClicked();
    void onCancelClicked();

private:
    void setupUI();
    void setupConnections();
    void initializeController();

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

    // Guard to stop the link-checkbox mirroring from re-triggering itself
    bool mirroringSliders;

    // Controller
    NVIDIAController* controller;

    // Current settings
    QColor currentRGBColor;
    int currentRGBBrightness;
    int currentWhiteBrightness;
    int currentMode;
};

#endif // MAINWINDOW_H