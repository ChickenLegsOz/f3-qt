#ifndef F3_LAUNCHER_H
#define F3_LAUNCHER_H
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QMap>
#include <QScopedPointer>


enum class F3Status {
    Ready = 0,
    Running = 1,
    Finished = 2,
    Stopped = 3,
    Staged = 4,
    Progressed = 5
};

enum class F3Error {
    Ok = 0,
    PathIncorrect = 128,
    NoCui = 129,
    NoPermission = 130,
    NoSpace = 131,
    NoProgress = 132,
    NoQuick = 133,
    CacheNotFound = 134,
    NoMemory = 135,
    NotDirectory = 136,
    NotDisk = 137,
    NotUSB = 138,
    NoFix = 139,
    NoReport = 140,
    Oversize = 141,
    Damaged = 142,
    NotDevice = 143,
    Unknown = 255
};

// For backward compatibility
using f3_launcher_status = F3Status;
using f3_launcher_error_code = F3Error;

struct f3_launcher_report
{
    bool success;
    QString ReadingSpeed;
    QString WritingSpeed;
    QString ReportedFree;
    QString ActualFree;
    QString LostSpace;
    float availability;
    QString ModuleSize;
    QString BlockSize;
};


class f3_launcher : public QObject
{
    Q_OBJECT

public:
    f3_launcher();
    ~f3_launcher();
    f3_launcher_status getStatus();
    f3_launcher_error_code getErrCode();
    void startCheck(QString devPath);
    void stopCheck();
    f3_launcher_report getReport();
    int getStage();
    bool setOption(QString key, QString value);
    QString getOption(QString key);
    void startFix();
    QString f3_cui_output;
    int progress10K;

signals:
    void f3_launcher_status_changed(f3_launcher_status status);
    void f3_launcher_error(f3_launcher_error_code errCode);

private:
    QScopedPointer<QProcess> f3_cui;
    QScopedPointer<QTimer> timer;
    QString devPath;
    QString f3_path;
    QMap<QString,QString> options;
    bool showProgress;
    int stage;
    F3Status status;
    F3Error errCode;

    void emitError(f3_launcher_error_code errorCode);
    bool probeCommand(QString command);
    float probeVersion();
    bool probeDiskFull(QString& devPath);
    bool probeCacheFile(QString& devPath);
    int parseOutput();

private slots:
    void on_f3_cui_finished();
    void on_timer_timeout();
};

#endif // F3_LAUNCHER_H
