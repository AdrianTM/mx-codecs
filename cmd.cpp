#include "cmd.h"

#include <QDebug>
#include <QEventLoop>
#include <QProcess>


// util function for getting bash command output
QString getCmdOut(const QString &cmd)
{
    qDebug().noquote() << cmd;
    QProcess proc;
    QEventLoop loop;
    proc.setReadChannelMode(QProcess::MergedChannels);
    proc.start("/bin/bash", QStringList() << "-c" << cmd);
    proc.waitForFinished();
    return proc.readAll().trimmed();
}


bool run(const QString &cmd)
{
    qDebug().noquote() << cmd;
    QProcess proc;
    QEventLoop loop;
    proc.start("/bin/bash", QStringList() << "-c" << cmd);
    proc.waitForFinished();
    return (proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0);
}

