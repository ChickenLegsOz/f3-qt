#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QScreen>
#include <QSettings>
#include <memory>
#include "f3_launcher.h"
#include "helpwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void on_buttonCheck_clicked();
    void on_buttonExit_clicked();
    void on_buttonSelectPath_clicked();
    void on_buttonHelp_clicked();
    void on_actionHelp_triggered();
    void on_actionAbout_triggered();
    void on_timerTimeout();
    void on_cuiStatusChanged(f3_launcher_status status);
    void on_cuiError(f3_launcher_error_code errCode);
    void on_buttonSelectDev_clicked();
    void on_optionQuickTest_clicked();
    void on_optionLessMem_clicked();
    void on_optionDestructive_clicked();
    void on_buttonHideResult_clicked();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    f3_launcher cui;
    QTimer timer;
    HelpWindow help;
    bool checking;
    int timerTarget;
    QString mountPoint;
    std::unique_ptr<QLabel> currentStatus;
    std::unique_ptr<QProgressBar> progressBar;
    
    void saveWindowState();
    void restoreWindowState();
    void centerOnScreen();

    void showStatus(const QString& string);
    void clearStatus();
    void showProgress(int progress10K);
    void showCapacity(int value);
    void showResultPage(bool visible);
    QString mountDisk(const QString& device, bool useSudo = false);
    bool unmountDisk(const QString& mountPoint, bool useSudo = false);
    bool sureToExit(bool manualClose);
    void promptFix();
    bool validatePath(const QString& path, bool isDevice);

protected:
    void closeEvent(QCloseEvent *);

private slots:
    void on_buttonLsblk_clicked();
    void on_buttonFdisk_clicked();
    void executeCommand(const QString &command, bool requiresSudo = false);
    void updateDeviceOutput(const QString &output);
};

#endif // MAINWINDOW_H
