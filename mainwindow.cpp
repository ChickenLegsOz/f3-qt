#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "helpwindow.h"
#include <QScreen>
#include <QLabel>
#include <QGuiApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QDir>
#include <QStatusBar>
#include <QCryptographicHash>

void f3_qt_fillReport(f3_launcher_report &report)
{
    const QString defaultValue = "(N/A)";
    if (report.ReportedFree.isEmpty())
        report.ReportedFree = defaultValue;
    if (report.ActualFree.isEmpty())
        report.ActualFree = defaultValue;
    if (report.availability < 0)
        report.availability = -1;
    if (report.ReadingSpeed.isEmpty())
        report.ReadingSpeed = defaultValue;
    if (report.WritingSpeed.isEmpty())
        report.WritingSpeed = defaultValue;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    currentStatus(new QLabel(this)),
    progressBar(new QProgressBar(this))
{    
    F3Error cuiError = cui.getErrCode();
    if (cuiError != F3Error::Ok)
        on_cuiError(cuiError);

    ui->setupUi(this);
    auto statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    // Configure the label
    currentStatus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    currentStatus->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    currentStatus->setMinimumWidth(200);

    // Configure the progress bar
    progressBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);

    // Create container widget and layout with smart pointer
    auto statusWidget = std::unique_ptr<QWidget>(new QWidget(this));
    statusWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto statusLayout = new QHBoxLayout(statusWidget.get());
    statusLayout->setContentsMargins(0, 0, 0, 0);

    // Add widgets to layout
    statusLayout->addWidget(currentStatus.get());
    statusLayout->addStretch(1);  // Use addStretch instead of QSpacerItem
    statusLayout->addWidget(progressBar.get());

    // Add to status bar
    statusBar->addWidget(statusWidget.release(), 1);  // Transfer ownership to status bar
    connect(&cui, &f3_launcher::f3_launcher_status_changed, this, &MainWindow::on_cuiStatusChanged);
    connect(&cui, &f3_launcher::f3_launcher_error, this, &MainWindow::on_cuiError);
    connect(&timer, &QTimer::timeout, this, &MainWindow::on_timerTimeout);
    checking = false;
    // Center window on screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move((screenGeometry.width() - width()) / 2 + screenGeometry.left(),
             (screenGeometry.height() - height()) / 2 + screenGeometry.top());
    }
    setFixedSize(width(), height());
    clearStatus();
}

MainWindow::~MainWindow()
{
    // Smart pointers will clean up automatically
}

void MainWindow::showStatus(const QString &string)
{
    currentStatus->setText(string);
}

void MainWindow::clearStatus()
{
    currentStatus->setText("Ready");
    ui->labelSpace->clear();
    ui->labelSpeed->clear();
    progressBar->setValue(0);
    //progressBar->setVisible(false);
    showResultPage(false);
}

void MainWindow::showProgress(int progress10K)
{
    progressBar->setVisible(true);
    
    // Handle special cases
    if (progress10K == -1) {
        // Show an indeterminate state
        progressBar->setMinimum(0);
        progressBar->setMaximum(0);
        showStatus("Processing...");
        return;
    } else if (progress10K < -1) {
        // Error or reset state
        progressBar->setMaximum(0);
        progressBar->setValue(0);
        showStatus("");
        return;
    }

    // Validate progress value
    progress10K = qBound(0, progress10K, 10000);
    
    // Ensure proper range is set
    if (progressBar->maximum() <= 0) {
        progressBar->setMinimum(0);
        progressBar->setMaximum(10000);
    }
    
    // Update progress
    progressBar->setValue(progress10K);
    
    // Format percentage with 2 decimal places
    double percentage = progress10K / 100.0;
    showStatus(QString("Processing: %1%").arg(percentage, 0, 'f', 2));
    
    // Ensure visual update
    progressBar->repaint();
}

void MainWindow::showCapacity(int value)
{
    // Validate the input value
    if (value > 100) {
        qWarning() << "Invalid capacity value:" << value << "(should be 0-100)";
        value = 100;
    }
    
    if (value >= 0)
    {
        ui->capacityBar->setFormat("%p% Available");
        timerTarget = value;
    }
    else
    {
        ui->capacityBar->setFormat("Capacity not available");
        timerTarget = 0;
    }

    ui->capacityBar->setValue(0);
    if (value <= 0)
        return;
        
    // Prevent division by zero and ensure reasonable update rate
    int interval = qBound(10, 1000 / value, 1000);  // Between 10ms and 1000ms
    timer.setInterval(interval);
    timer.start();
}

void MainWindow::showResultPage(bool visible)
{
    if (visible) {
        ui->tabWidget->setTabVisible(2, true);
        ui->tabWidget->setCurrentIndex(2);
    } else {
        ui->tabWidget->setTabVisible(2, false);
        ui->tabWidget->setCurrentIndex(0);
    }
}

QString MainWindow::mountDisk(const QString& device)
{
    // Sanitize and validate the device path
    QString sanitizedDevice = device.trimmed();
    QFileInfo deviceInfo(sanitizedDevice);
    
    if (sanitizedDevice.isEmpty() || !deviceInfo.exists()) {
        QMessageBox::critical(this, "Mount Error", "Invalid device path specified.");
        return QString();
    }
    
    // Create a safe mount directory name
    QDir dir;
    QString mountDir = QString("%1/f3qt_mount_%2").arg(
        QDir::tempPath(),
        QString(QCryptographicHash::hash(device.toUtf8(), QCryptographicHash::Sha256).toHex()).left(8)
    );
    
    // Ensure the directory doesn't exist and create it
    if (dir.exists(mountDir)) {
        if (!dir.rmdir(mountDir)) {
            QMessageBox::critical(this, "Mount Error", "Unable to clean up existing mount point.");
            return QString();
        }
    }
    
    if (!dir.mkdir(mountDir)) {
        QMessageBox::critical(this, "Mount Error", "Unable to create mount directory.");
        return QString();
    }
    
    // Setup and execute mount command with smart pointer
    auto proc = std::unique_ptr<QProcess>(new QProcess());
    QStringList args;
    args << sanitizedDevice << mountDir;
    
    proc->start("mount", args);
    if (!proc->waitForStarted(5000)) {  // 5 second timeout
        dir.rmdir(mountDir);
        QMessageBox::critical(this, "Mount Error", "Failed to start mount command.");
        return QString();
    }
    
    if (!proc->waitForFinished(30000)) {  // 30 second timeout
        proc->terminate();
        proc->waitForFinished(5000);
        dir.rmdir(mountDir);
        QMessageBox::critical(this, "Mount Error", "Mount operation timed out.");
        return QString();
    }
    
    if (proc->exitCode() != 0) {
        QString errorOutput = QString::fromUtf8(proc->readAllStandardError());
        dir.rmdir(mountDir);
        QMessageBox::critical(this, "Mount Error", 
            QString("Failed to mount device.\nError: %1").arg(errorOutput.isEmpty() ? "Unknown error" : errorOutput));
        return QString();
    }
    
    // Verify the mount was successful
    QDir mountedDir(mountDir);
    if (!mountedDir.exists()) {
        QMessageBox::critical(this, "Mount Error", "Mount point doesn't exist after mount operation.");
        return QString();
    }
    
    return mountDir;
}

bool MainWindow::unmountDisk(const QString& mountPoint)
{
    // Validate mount point
    QString sanitizedMountPoint = mountPoint.trimmed();
    if (sanitizedMountPoint.isEmpty()) {
        QMessageBox::critical(this, "Unmount Error", "Invalid mount point specified.");
        return false;
    }
    
    QDir mountDir(sanitizedMountPoint);
    if (!mountDir.exists()) {
        QMessageBox::warning(this, "Unmount Warning", "Mount point doesn't exist.");
        return false;
    }
    
    // Setup and execute unmount command with smart pointer
    auto proc = std::unique_ptr<QProcess>(new QProcess());
    QStringList args;
    args << sanitizedMountPoint;
    
    proc->start("umount", args);
    if (!proc->waitForStarted(5000)) {  // 5 second timeout
        QMessageBox::critical(this, "Unmount Error", "Failed to start unmount command.");
        return false;
    }
    
    if (!proc->waitForFinished(30000)) {  // 30 second timeout
        proc->terminate();
        proc->waitForFinished(5000);
        QMessageBox::critical(this, "Unmount Error", "Unmount operation timed out.");
        return false;
    }
    
    if (proc->exitCode() != 0) {
        QString errorOutput = QString::fromUtf8(proc->readAllStandardError());
        QMessageBox::critical(this, "Unmount Error", 
            QString("Failed to unmount device.\nError: %1").arg(errorOutput.isEmpty() ? "Unknown error" : errorOutput));
        return false;
    }
    
    // Clean up the mount point directory
    if (!mountDir.rmdir(sanitizedMountPoint)) {
        QMessageBox::warning(this, "Unmount Warning", 
            "Device unmounted successfully but failed to remove mount point directory.");
    }
    
    return true;
}

bool MainWindow::sureToExit(bool manualClose)
{
    if (!manualClose)
        if (QMessageBox::question(this,"Quit F3",
                              "The program is still running a check.\n"
                              "Quit anyway?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
            return false;
    if (ui->tabWidget->currentIndex() == 1 && ui->optionQuickTest->isChecked()
                      && ui->optionDestructive->isChecked() == false)
    {
        if (QMessageBox::warning(this,"Quit F3",
                              "You are going to abort this test.\n"
                              "Quit now will cause PERMANENT DATA LOSS!\n"
                              "Do you really want to quit?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
            return false;
    }
    return true;
}

void MainWindow::promptFix()
{
    if (QMessageBox::question(this, "Wrong Capacity Detected",
                              "This device does not have the capacity it claims.\n"
                              "Would you like to fix it?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) != QMessageBox::Yes)
        return;
    if (QMessageBox::warning(this,"Fix Capacity",
                          "You are going to fix the capacity of this disk.\n"
                          "All data on this disk will be LOST!\n"
                          "Are you sure to continue?",
                          QMessageBox::Yes | QMessageBox::No,
                          QMessageBox::No) != QMessageBox::Yes)
        return;
    cui.startFix();
}

void MainWindow::on_cuiStatusChanged(F3Status status)
{
    QString qsSpinNext;
    switch(status)
    {
        case F3Status::Ready:
            showStatus("Ready.");
            break;
        case F3Status::Running:
            showStatus("Checking... Please wait...");
            ui->textDev->setReadOnly(true);
            ui->textDevPath->setReadOnly(true);
            ui->buttonSelectDev->setEnabled(false);
            ui->buttonSelectPath->setEnabled(false);
            progressBar->setVisible(true);
            break;
        case F3Status::Finished:
        {
            f3_launcher_report report = cui.getReport();
            
            // First handle the progress bar and make it invisible
            progressBar->setValue(0);
            progressBar->setVisible(false);
            
            // Then set the final status
            if (report.success)
                showStatus("Finished (without error).");
            else
                showStatus("Finished.");
                
            if (report.ReportedFree == "(Fixed)")
            {
                QMessageBox::information(this,"Fixed successfully",
                                         "The capacity of the partition of the disk\n"
                                         "has been adjusted to what it should be.");
                break;
            }
            f3_qt_fillReport(report);
            ui->labelSpace->setText(QString("Free Space: ")
                                    .append(report.ReportedFree)
                                    .append("\nActual: ")
                                    .append(report.ActualFree)
                                    );
            ui->labelSpeed->setText(QString("Read speed: ")
                                    .append(report.ReadingSpeed)
                                    .append("\nWrite speed: ")
                                    .append(report.WritingSpeed)
                                    );
            showCapacity(report.availability * 100);
            showResultPage(true);
            break;
        }
        case F3Status::Stopped:
            showStatus("Stopped.");
            showProgress(0);
            progressBar->setFormat("!");
            break;
        case F3Status::Staged:
        {
            QString progressText = QString("Progress: (Stage %1)").arg(cui.getStage());
            showStatus(progressText);
            showProgress(-1);
            progressBar->setFormat("?");
            break;
        }
        case F3Status::Progressed:
            showProgress(cui.progress10K);
            switch(progressBar->format()[0].toLatin1()){
                case '|': qsSpinNext = "/"; break;
                case '/': qsSpinNext = "---"; break;
                case '-': qsSpinNext = "\\"; break;
                case '\\': qsSpinNext = "|"; break;
                default: qsSpinNext = "|"; break;
            }
            progressBar->setFormat(qsSpinNext);
            break;
    }
    if (status == F3Status::Running ||
        status == F3Status::Staged ||
        status == F3Status::Progressed)
    {
        checking = true;
        ui->buttonCheck->setText("Stop");
    }
    else
    {
        if (ui->tabWidget->currentIndex() == 1)
        {
            if (!ui->optionQuickTest->isChecked())
                unmountDisk(mountPoint);
        }
        checking = false;
        ui->buttonCheck->setText("Check!");
        ui->textDev->setReadOnly(false);
        ui->textDevPath->setReadOnly(false);
        ui->buttonSelectDev->setEnabled(true);
        ui->buttonSelectPath->setEnabled(true);
    }
}

void MainWindow::on_cuiError(F3Error errCode)
{
    switch(errCode)
    {
        case F3Error::NoCui:
            QMessageBox::critical(this,"No f3 program",
                                  "Cannot find f3read/f3write.\n"
                                  "Please install f3 first.");
            exit(0);
        case F3Error::NoProgress:
            QMessageBox::warning(this,"No progress showing",
                                 "You are using an old version of f3read/f3write.\n"
                                 "The progress will not be shown during checking.");
            break;
        case F3Error::PathIncorrect:
            QMessageBox::critical(this,"Path error",
                              "Device path not found.\n"
                              "Please try mounting it correctly.");
            break;
        case F3Error::NoPermission:
            QMessageBox::warning(this,"Permission denied",
                          "Cannot write to device.\n"
                          "Try to re-run with sudo.");
            showStatus("No enough space for test.");
            break;
        case F3Error::NoSpace:
            QMessageBox::information(this,"No space",
                          "No enough space for checking.\n"
                          "Please delete some file, or format the device and try again.");
            showStatus("No enough space for test.");
            break;
        case F3Error::NoQuick:
            QMessageBox::warning(this,"Legacy Mode Only",
                                 "f3probe was not found.\n"
                                 "We are not able to test under quick mode.");
            break;
        case F3Error::CacheNotFound:
            showStatus("No cached data found. Test from writing...");
            break;
        case F3Error::NoMemory:
            QMessageBox::warning(this,"Out of memory",
                                 "No enough memory for checking.\n"
                                 "You may try again with following options:\n"
                                 "  -Use less memory\n"
                                 "  -Destructive Test");
            break;
        case F3Error::NotDirectory:
            QMessageBox::critical(this,"Path error",
                              "The path specified is not a directory.\n");
            break;
        case F3Error::NotDisk:
            QMessageBox::critical(this,"Device type error",
                                  "The device specified is not a disk.\n"
                                  "Please make sure what you choose is a disk,"
                                  " not a partition.");
            break;
        case F3Error::NotUSB:
            QMessageBox::critical(this,"Device type error",
                                  "The device specified is not a USB device.\n"
                                  "Currently f3 does not support quick test on "
                                  "device that is not backed by USB (e.g. mmc, scsi).");
            break;
        case F3Error::NotDevice:
            QMessageBox::critical(this,"Device type error",
                                  "The device specified is a directory not a valid device.\n"
                                  "Please make sure what you choose is a valid device.");
            break;
        case F3Error::NoFix:
            QMessageBox::warning(this,"Probing Only",
                             "f3fix was not found.\n"
                             "We are not able to fix the disk if its capacity is wrong.");
            break;
        case F3Error::NoReport:
            QMessageBox::warning(this,"No test result",
                                 "No test has been completed. Please run a test first.");
            break;
        case F3Error::Oversize:
            QMessageBox::critical(this,"Fix failed",
                                  "Cannot use detected capacity for fixing.\n"
                                  "You may need to report this as a bug.");
            break;
        case F3Error::Damaged:
            QMessageBox::critical(this,"Device inaccessible",
                                  "Cannot access the specified device.\n"
                                  "You may not have the right permission to"
                                  "read and write to it, or it has been damaged.");
            break;
        default:
            QMessageBox::warning(this,"Unknown error",
                                 "Unknown error occurred.\n"
                                 "You may need to report this as a bug.");
            showStatus("An error occurred.");
    }
}

bool MainWindow::validatePath(const QString& path, bool isDevice) {
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Path cannot be empty!");
        return false;
    }

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "Validation Error", 
            QString("The specified %1 does not exist!").arg(isDevice ? "device" : "path"));
        return false;
    }

    if (isDevice) {
        // For device validation, check if it's in /dev
        if (!path.startsWith("/dev/")) {
            QMessageBox::warning(this, "Validation Error", 
                "Device path must be in /dev/ directory!");
            return false;
        }
        
        // Check if it's a device file in /dev
        QDir devDir("/dev");
        if (!devDir.exists(path)) {
            QMessageBox::warning(this, "Validation Error", 
                "The specified device path does not exist in /dev!");
            return false;
        }
        
        // Check if it's a block device by checking if it's a special file
        QFile device(path);
        if (!device.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Validation Error", 
                "Cannot access the device. Check permissions.");
            return false;
        }
        device.close();
    } else {
        // Directory-specific validation
        if (!fileInfo.isDir()) {
            QMessageBox::warning(this, "Validation Error", 
                "The specified path is not a directory!");
            return false;
        }
        if (!fileInfo.isWritable()) {
            QMessageBox::warning(this, "Validation Error", 
                "The specified directory is not writable!");
            return false;
        }
    }

    return true;
}

void MainWindow::on_buttonCheck_clicked()
{
    if (checking)
    {
        cui.stopCheck();
        return;
    }

    QString inputPath;
    bool isDeviceMode = ui->tabWidget->currentIndex() == 1;
    
    if (!isDeviceMode)
        inputPath = ui->textDevPath->text().trimmed();
    else
        inputPath = ui->textDev->text().trimmed();
    
    if (!validatePath(inputPath, isDeviceMode)) {
        return;
    }

    if (ui->tabWidget->currentIndex() == 0)
    {
        cui.setOption("mode", "legacy");
        cui.setOption("cache", "none");
    }
    else
    {
        if (ui->optionQuickTest->isChecked())
            cui.setOption("mode", "quick");
        else
        {
            mountPoint = mountDisk(inputPath);
            if (mountPoint.isEmpty())
            {
                QMessageBox::critical(this,"Error","Cannot mount selected device!\n"
                                      "You may need to run this program as root.");
                return;
            }
            inputPath = mountPoint;
            cui.setOption("mode", "legacy");
        }

        if (ui->optionUseCache->isChecked())
            cui.setOption("cache", "write");
        else
            cui.setOption("cache", "none");

        if (ui->optionLessMem->isChecked())
            cui.setOption("memory", "minimum");
        else
            cui.setOption("memory", "full");

        if (ui->optionDestructive->isChecked())
        {
            if (QMessageBox::question(this, "Run destructive test",
                                      "You choose to run destructive test.\n"
                                      "The data on the disk will be destroyed!\n"
                                      "Continue?",
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No) != QMessageBox::Yes)
                return;
            cui.setOption("destructive", "yes");
        }
        else
            cui.setOption("destructive", "no");
    }

    clearStatus();
    cui.startCheck(inputPath);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!checking)
        return;
    if (sureToExit(false))
    {
        cui.stopCheck();
        help.close();
    }
    else
        event->ignore();
}

void MainWindow::on_buttonExit_clicked()
{
    if (checking)
    {
        if (!sureToExit(true))
            return;
        cui.stopCheck();
        checking = false;
    }
    this->close();
}

void MainWindow::on_buttonSelectPath_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "Choose Device Path", 
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (path.isEmpty())
        return;
        
    if (validatePath(path, false)) {
        ui->textDevPath->setText(path);
    }
}

void MainWindow::on_timerTimeout()
{
    int value = ui->capacityBar->value();
    if (value < timerTarget)
    {
        ui->capacityBar->setValue(++value);
    }
    else
    {
        timer.stop();
        if (timerTarget < 100 && ui->tabWidget->currentIndex() == 1 && ui->optionQuickTest->isChecked())
            promptFix();
    }
}

void MainWindow::on_buttonHelp_clicked()
{
    help.show();
}

void MainWindow::on_buttonSelectDev_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "Choose Device Path", "/dev",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (path.isEmpty())
        return;
        
    if (validatePath(path, true)) {
        ui->textDev->setText(path);
    }
}

void MainWindow::on_optionQuickTest_clicked()
{
    if (ui->optionQuickTest->isChecked())
    {
        ui->optionDestructive->setEnabled(true);
        ui->optionLessMem->setEnabled(true);
        ui->optionUseCache->setEnabled(false);
    }
    else
    {
        ui->optionDestructive->setChecked(false);
        ui->optionDestructive->setEnabled(false);
        ui->optionLessMem->setChecked(false);
        ui->optionLessMem->setEnabled(false);
        ui->optionUseCache->setEnabled(true);
    }
}

void MainWindow::on_optionLessMem_clicked()
{
    ui->optionDestructive->setChecked(false);
}

void MainWindow::on_optionDestructive_clicked()
{
    ui->optionLessMem->setChecked(false);
}

void MainWindow::on_buttonHideResult_clicked()
{
    showResultPage(false);
}
