#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QGroupBox>
#include <QApplication>
#include <QColorDialog>
#include <QMessageBox>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , controller(nullptr)
    , currentRGBColor(Qt::red)
    , currentWhiteBrightness(100)
    , currentMode(1) // Direct mode
{
    setupUI();
    setupConnections();
    initializeController();
}

MainWindow::~MainWindow()
{
    if (controller)
    {
        delete controller;
    }
}

void MainWindow::setupUI()
{
    // Create central widget and main layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Create tab widget
    tabWidget = new QTabWidget();

    // RGB Color Tab
    QWidget* rgbTab = new QWidget();
    QVBoxLayout* rgbLayout = new QVBoxLayout(rgbTab);

    rgbColorGroup = new QGroupBox("RGB Color");
    QVBoxLayout* rgbColorLayout = new QVBoxLayout(rgbColorGroup);

    rgbColorButton = new QPushButton("Select RGB Color");
    rgbColorButton->setFixedHeight(50);

    colorDisplay = new QLabel("Color Preview");
    colorDisplay->setFixedHeight(30);
    colorDisplay->setStyleSheet("background-color: red; border: 1px solid black;");

    rgbColorLayout->addWidget(rgbColorButton);
    rgbColorLayout->addWidget(colorDisplay);

    rgbLayout->addWidget(rgbColorGroup);

    // White Brightness Tab
    QWidget* whiteTab = new QWidget();
    QVBoxLayout* whiteLayout = new QVBoxLayout(whiteTab);

    whiteBrightnessGroup = new QGroupBox("White Brightness");
    QVBoxLayout* brightnessLayout = new QVBoxLayout(whiteBrightnessGroup);

    whiteBrightnessSlider = new QSlider(Qt::Horizontal);
    whiteBrightnessSlider->setMinimum(0);
    whiteBrightnessSlider->setMaximum(100);
    whiteBrightnessSlider->setValue(currentWhiteBrightness);

    whiteBrightnessLabel = new QLabel(QString::number(currentWhiteBrightness) + "%");
    whiteBrightnessLabel->setAlignment(Qt::AlignCenter);

    brightnessLayout->addWidget(whiteBrightnessSlider);
    brightnessLayout->addWidget(whiteBrightnessLabel);

    whiteLayout->addWidget(whiteBrightnessGroup);

    // Mode selection
    QGroupBox* modeGroup = new QGroupBox("Mode");
    QVBoxLayout* modeLayout = new QVBoxLayout(modeGroup);

    modeComboBox = new QComboBox();
    modeComboBox->addItem("Off");
    modeComboBox->addItem("Direct");
    modeComboBox->setCurrentIndex(currentMode);

    modeLayout->addWidget(modeComboBox);

    // Add tabs
    tabWidget->addTab(rgbTab, "RGB Color");
    tabWidget->addTab(whiteTab, "White Brightness");

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    applyButton = new QPushButton("Apply");
    cancelButton = new QPushButton("Cancel");

    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);

    // Add all elements to main layout
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(modeGroup);
    mainLayout->addLayout(buttonLayout);

    // Set window properties
    setWindowTitle("RTX 3090 FE LED Controller");
    resize(400, 350);
}

void MainWindow::setupConnections()
{
    connect(rgbColorButton, &QPushButton::clicked, this, &MainWindow::onRGBColorButtonClicked);
    connect(whiteBrightnessSlider, &QSlider::valueChanged, this, &MainWindow::onWhiteBrightnessChanged);
    connect(modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);
    connect(applyButton, &QPushButton::clicked, this, &MainWindow::onApplyClicked);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelClicked);
}

void MainWindow::initializeController()
{
    // Try to initialize the NVIDIA controller
    controller = new NVIDIAController();

    if (!controller->isInitialized())
    {
        QMessageBox::warning(this, "Controller Error",
                            "Failed to initialize NVIDIA controller. "
                            "Make sure you have NVIDIA GPU with illumination support and proper drivers.");
        return;
    }

    // Set initial color
    updateColorDisplay();
}

void MainWindow::onRGBColorButtonClicked()
{
    QColor newColor = QColorDialog::getColor(currentRGBColor, this, "Select LED Color");
    if (newColor.isValid())
    {
        currentRGBColor = newColor;
        updateColorDisplay();
    }
}

void MainWindow::onWhiteBrightnessChanged(int value)
{
    currentWhiteBrightness = value;
    whiteBrightnessLabel->setText(QString::number(value) + "%");
}

void MainWindow::onModeChanged(int index)
{
    currentMode = index;
}

void MainWindow::onApplyClicked()
{
    if (controller && controller->isInitialized())
    {
        controller->setMode(currentMode);
        controller->setBrightness(currentWhiteBrightness);
        controller->setRGBColor(currentRGBColor);
        controller->updateLEDs();
    }
}

void MainWindow::onCancelClicked()
{
    // Reset to current values
    whiteBrightnessSlider->setValue(currentWhiteBrightness);
    updateColorDisplay();
}

void MainWindow::updateColorDisplay()
{
    // Update color display label
    QString colorStyle = QString("background-color: rgb(%1, %2, %3); border: 1px solid black;")
                        .arg(currentRGBColor.red())
                        .arg(currentRGBColor.green())
                        .arg(currentRGBColor.blue());
    colorDisplay->setStyleSheet(colorStyle);
}