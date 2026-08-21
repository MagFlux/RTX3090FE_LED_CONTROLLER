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
#include <QSignalBlocker>
#include <QIcon>

MainWindow::MainWindow(bool startHidden, QWidget *parent)
    : QMainWindow(parent)
    , mirroringSliders(false)
    , startedHidden(startHidden)
    , trayIcon(nullptr)
    , controller(nullptr)
    , currentRGBColor(Qt::red)
    , currentRGBBrightness(100)
    , currentWhiteBrightness(100)
    , currentMode(1) // Direct mode
{
    setupUI();
    setupTrayIcon();
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

    // Brightness Tab (separate sliders for the RGB zone and the white zone)
    QWidget* whiteTab = new QWidget();
    QVBoxLayout* whiteLayout = new QVBoxLayout(whiteTab);

    whiteBrightnessGroup = new QGroupBox("Brightness");
    QVBoxLayout* brightnessLayout = new QVBoxLayout(whiteBrightnessGroup);

    // RGB light zone slider
    QLabel* rgbSliderTitle = new QLabel("RGB Light");
    rgbSliderTitle->setAlignment(Qt::AlignCenter);

    rgbBrightnessSlider = new QSlider(Qt::Horizontal);
    rgbBrightnessSlider->setMinimum(0);
    rgbBrightnessSlider->setMaximum(100);
    rgbBrightnessSlider->setValue(currentRGBBrightness);

    rgbBrightnessLabel = new QLabel(QString::number(currentRGBBrightness) + "%");
    rgbBrightnessLabel->setAlignment(Qt::AlignCenter);

    brightnessLayout->addWidget(rgbSliderTitle);
    brightnessLayout->addWidget(rgbBrightnessSlider);
    brightnessLayout->addWidget(rgbBrightnessLabel);

    // White light zone slider
    QLabel* whiteSliderTitle = new QLabel("White Light");
    whiteSliderTitle->setAlignment(Qt::AlignCenter);

    whiteBrightnessSlider = new QSlider(Qt::Horizontal);
    whiteBrightnessSlider->setMinimum(0);
    whiteBrightnessSlider->setMaximum(100);
    whiteBrightnessSlider->setValue(currentWhiteBrightness);

    whiteBrightnessLabel = new QLabel(QString::number(currentWhiteBrightness) + "%");
    whiteBrightnessLabel->setAlignment(Qt::AlignCenter);

    brightnessLayout->addWidget(whiteSliderTitle);
    brightnessLayout->addWidget(whiteBrightnessSlider);
    brightnessLayout->addWidget(whiteBrightnessLabel);

    // Link control: mirrors both sliders to the same value
    linkCheckBox = new QCheckBox("Link sliders");
    linkCheckBox->setChecked(true);

    brightnessLayout->addWidget(linkCheckBox);

    whiteLayout->addWidget(whiteBrightnessGroup);

    // Mode selection
    QGroupBox* modeGroup = new QGroupBox("Mode");
    QVBoxLayout* modeLayout = new QVBoxLayout(modeGroup);

    modeComboBox = new QComboBox();
    modeComboBox->addItem("Off");
    modeComboBox->addItem("Direct");
    modeComboBox->setCurrentIndex(currentMode);

    modeLayout->addWidget(modeComboBox);

    // Settings Tab (application-level options such as start-at-logon)
    QWidget* settingsTab = new QWidget();
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsTab);

    QGroupBox* settingsGroup = new QGroupBox("Settings");
    QVBoxLayout* settingsGroupLayout = new QVBoxLayout(settingsGroup);

    startAtLoginCheckBox = new QCheckBox("Start at logon");
    settingsGroupLayout->addWidget(startAtLoginCheckBox);

    settingsLayout->addWidget(settingsGroup);
    settingsLayout->addStretch();

    // Add tabs
    tabWidget->addTab(rgbTab, "RGB Color");
    tabWidget->addTab(whiteTab, "Brightness");
    tabWidget->addTab(settingsTab, "Settings");

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
    connect(rgbBrightnessSlider, &QSlider::valueChanged, this, &MainWindow::onRBGBrightnessChanged);
    connect(whiteBrightnessSlider, &QSlider::valueChanged, this, &MainWindow::onWhiteBrightnessChanged);
    connect(linkCheckBox, &QCheckBox::toggled, this, &MainWindow::onLinkToggled);
    connect(modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);
    connect(applyButton, &QPushButton::clicked, this, &MainWindow::onApplyClicked);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    connect(startAtLoginCheckBox, &QCheckBox::toggled, this, &MainWindow::onToggleStartAtLogin);
}

void MainWindow::setupTrayIcon()
{
    // A real icon asset is bundled in resources/resources.qrc (as :/icon.ico).
    // Use it for both the window and the system-tray icon so the user has a
    // consistent, recognizable glyph everywhere (taskbar, Alt+Tab, tray).
    const QIcon appIcon(QStringLiteral(":/icon.ico"));
    setWindowIcon(appIcon);

    trayMenu = new QMenu(this);
    trayToggleStartupAction = trayMenu->addAction("Start at logon");
    trayToggleStartupAction->setCheckable(true);
    connect(trayToggleStartupAction, &QAction::toggled, this,
            &MainWindow::onToggleStartAtLogin);
    trayMenu->addAction(trayToggleStartupAction);
    trayMenu->addSeparator();
    trayMenu->addAction("Show", this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    trayMenu->addAction("Quit", qApp, &QApplication::quit);

    trayIcon = new QSystemTrayIcon(appIcon, this);
    trayIcon->setToolTip("RTX 3090 FE LED Controller");
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
    trayIcon->show();

    // Reflect current startup-folder state in both check boxes.
    const bool enabled = startAtLoginEnabled();
    QSignalBlocker guard1(startAtLoginCheckBox);
    QSignalBlocker guard2(trayToggleStartupAction);
    startAtLoginCheckBox->setChecked(enabled);
    trayToggleStartupAction->setChecked(enabled);
}

bool MainWindow::startAtLoginEnabled() const
{
    // The Run key is a fixed, well-known location; we open it in "raw" mode so
    // QSettings maps this path verbatim to
    // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
    QSettings runKey(
        QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
    return !runKey.value(QStringLiteral("RTX3090Controller")).toString().isEmpty();
}

void MainWindow::setStartAtLoginEnabled(bool enabled)
{
    QSettings runKey(
        QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);

    if (enabled) {
        // Quote the exe in case its path contains spaces, then pass the flag
        // that makes the app start hidden in the tray on logon.
        const QString cmd = QStringLiteral("\"%1\" --background")
                                .arg(QCoreApplication::applicationFilePath());
        runKey.setValue(QStringLiteral("RTX3090Controller"), cmd);
    } else {
        runKey.remove(QStringLiteral("RTX3090Controller"));
    }
    runKey.sync();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        // Activating the tray icon always brings the window to the foreground.
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::onToggleStartAtLogin(bool checked)
{
    setStartAtLoginEnabled(checked);

    // Keep the GUI checkbox and the tray menu action in sync. The signal
    // source is whichever one the user toggled, so guard *only* the other one
    // to avoid a re-entry loop.
    if (this->sender() == startAtLoginCheckBox) {
        QSignalBlocker guard(trayToggleStartupAction);
        trayToggleStartupAction->setChecked(checked);
    } else if (this->sender() == trayToggleStartupAction) {
        QSignalBlocker guard(startAtLoginCheckBox);
        startAtLoginCheckBox->setChecked(checked);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Closing the window quits the whole application (per spec).
    qApp->quit();
    event->accept();
}

void MainWindow::changeEvent(QEvent *event)
{
    // Minimizing the window drops it out of the taskbar into the system tray
    // instead of staying minimized on the taskbar; the tray icon (which is
    // always present) becomes the way to bring the app back to the foreground.
    if (event->type() == QEvent::WindowStateChange
            && isMinimized()
            && trayIcon != nullptr && trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
    QMainWindow::changeEvent(event);
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

    // Seed the GUI controls from the GPU's current lighting state so the
    // sliders / colour preview match what the GPU is actually showing.
    currentRGBColor = controller->getCurrentRGBColor();
    const int gpuRgb = controller->getCurrentRGBBrightness();
    const int gpuWhite = controller->getCurrentWhiteBrightness();

    // Suppress link-mirroring while restoring so both zones keep their own
    // (possibly different) GPU values.
    mirroringSliders = true;
    rgbBrightnessSlider->setValue(gpuRgb);
    whiteBrightnessSlider->setValue(gpuWhite);
    mirroringSliders = false;

    currentRGBBrightness = gpuRgb;
    currentWhiteBrightness = gpuWhite;
    linkCheckBox->setChecked(gpuRgb == gpuWhite);
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

void MainWindow::onRBGBrightnessChanged(int value)
{
    currentRGBBrightness = value;
    rgbBrightnessLabel->setText(QString::number(value) + "%");

    if (linkCheckBox->isChecked() && !mirroringSliders)
    {
        mirroringSliders = true;
        whiteBrightnessSlider->setValue(value);
        mirroringSliders = false;
    }
}

void MainWindow::onWhiteBrightnessChanged(int value)
{
    currentWhiteBrightness = value;
    whiteBrightnessLabel->setText(QString::number(value) + "%");

    if (linkCheckBox->isChecked() && !mirroringSliders)
    {
        mirroringSliders = true;
        rgbBrightnessSlider->setValue(value);
        mirroringSliders = false;
    }
}

void MainWindow::onLinkToggled(bool)
{
    // No action needed; the checkbox is read by the two slider handlers to
    // decide whether to mirror the opposite slider.
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
        controller->setBrightness(currentRGBBrightness);
        controller->setWhiteBrightness(currentWhiteBrightness);
        controller->setRGBColor(currentRGBColor);
        controller->updateLEDs();
    }
}

void MainWindow::onCancelClicked()
{
    // Reset both sliders to their stored values
    mirroringSliders = true;
    rgbBrightnessSlider->setValue(currentRGBBrightness);
    whiteBrightnessSlider->setValue(currentWhiteBrightness);
    mirroringSliders = false;
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