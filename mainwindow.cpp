/*****************************************************************************
 * mx-codecs.cpp
 *****************************************************************************
 * Copyright (C) 2014 MX Authors
 *
 * Authors: Jerry 3904
 *          Anticaptilista
 *          Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MX Codecs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MX Codecs.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include <QDebug>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryFile>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cmd.h"
#include "about.h"

MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainWindow), lock_file("/var/lib/dpkg/lock"), reply(nullptr)
{
    qDebug().noquote() << qApp->applicationName() << "version:" << qApp->applicationVersion();
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // get arch info
    arch = cmd.getCmdOut("dpkg --print-architecture", true);

    setWindowFlags(Qt::Window); // for the close, min and max buttons
    if (ui->buttonOk->icon().isNull()) {
        ui->buttonOk->setIcon(QIcon(":/icons/dialog-ok.svg"));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateStatus(const QString& msg, int val) {
    ui->labelDownload->setText(msg);
    ui->progressBar->setValue(val);
    qApp->processEvents();
}

// Check if online
bool MainWindow::checkOnline()
{
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QEventLoop loop;

    QNetworkRequest request;
    request.setRawHeader("User-Agent", qApp->applicationName().toUtf8() + "/" + qApp->applicationVersion().toUtf8() + " (linux-gnu)");

    QStringList addresses{"http://mxrepo.com", "http://google.com"}; // list of addresses to try
    for (const QString &address : addresses) {
        error = QNetworkReply::NoError;
        request.setUrl(QUrl(address));
        reply = manager.get(request);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [&error](const QNetworkReply::NetworkError &err) {error = err;} ); // errorOccured only in Qt >= 5.15
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), &loop, &QEventLoop::quit);
        loop.exec();
        reply->disconnect();
        if (error == QNetworkReply::NoError) {
            return true;
        }
    }
    qDebug() << "No network detected:" << reply->url() << error;
    return false;
}

void MainWindow::on_buttonOk_clicked() {
    if (ui->stackedWidget->currentIndex() == 0) {
        setCursor(QCursor(Qt::WaitCursor));
        if (!checkOnline()) {
            QMessageBox::critical(this, tr("Error"), tr("Internet is not available, won't be able to download the list of packages"));
            setCursor(QCursor(Qt::ArrowCursor));
            return;
        }
        installDebs(downloadDebs());
    } else {
        qApp->quit();
    }
}

bool MainWindow::downloadDeb(const QString &url, const QString &filepath)
{
    QFileInfo fi(filepath);
    QFile tofile(fi.fileName());
    if (!(downloadFile(url + "/" + filepath, tofile))) {
        QMessageBox::critical(this, windowTitle(),
                              QString(tr("Error downloading %1")).arg(fi.fileName()));
        return false;
    }
    return true;
}

bool MainWindow::downloadFile(const QString &url, QFile &file)
{
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        return false;
    }

    reply = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop loop;

    bool success = true;
    connect(reply, &QNetworkReply::readyRead, [this, &file, &success]() { success = file.write(reply->readAll()); });
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->disconnect();

    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("There was an error writing file: %1. Please check if you have enough free space on your drive").arg(file.fileName()));
        exit(EXIT_FAILURE);
    }

    file.close();
    return (reply->error() == QNetworkReply::NoError);
}

// download .deb codecs returns download path
QString MainWindow::downloadDebs() {
    QString cmd_str, out, msg;
    QString path, release;
    QString url = "http://deb-multimedia.org";

    //set progressBar and refresh
    ui->progressBar->setValue(0);
    ui->stackedWidget->setCurrentIndex(1);
    qApp->processEvents();

    // create temp folder and set it current
    if (!tempdir.isValid()) {
        QMessageBox::critical(this, tr("Error"), tr("Could not create temp directory. "));
        exit(EXIT_FAILURE);
    }
    path = tempdir.path();
    QDir::setCurrent(path);

    // get release info
    release = cmd.getCmdOut("grep VERSION= /etc/os-release |grep -Eo [a-z]+ ");

    int idx = 0;

    QTemporaryFile file;
    updateStatus(tr("<b>Running command...</b><p>") + tr("downloading Packages.gz from 'main'"), idx += 10);
    downloadInfoAndPackage(url, release, "main", arch, file, QStringList{"libdvdcss2", "libtxc-dxtn0"}, idx += 10);

    QTemporaryFile file_nonfree;
    updateStatus(tr("<b>Running command...</b><p>") + tr("downloading Packages.gz from 'non-free'"), idx += 10);
    if (!downloadInfoAndPackage(url, release, "non-free", arch, file_nonfree, QStringList{"w.*codecs.*deb"}, idx +=10)) arch_flag = false;

    // if 64 bit, also install 32 bit libtxc-dxtn0 package
    if (arch == "amd64") {
        QTemporaryFile file_i386;
        updateStatus(tr("<b>Running command...</b><p>") + tr("downloading Packages.gz from i386 'main'"), idx += 10);
        if (!downloadInfoAndPackage(url, release, "main", "i386", file_i386, QStringList{"libtxc-dxtn0"}, idx += 10)) i386_flag = false;
    }

    updateStatus(tr("<b>Download Finished.</b>"), idx += 10);
    return path;
}

bool MainWindow::downloadInfoAndPackage(const QString &url, const QString &release, const QString &repo, const QString &arch, QFile &file, QStringList search_terms, int progress)
{
    if (!downloadFile(url + "/dists/" + release + "/" + repo + "/binary-" + arch + "/Packages.gz", file)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Cannot connect to the download site"));
        return false;
    }

    for (const QString &search_deb : search_terms) {
        QString out = cmd.getCmdOut("zgrep ^Filename " + file.fileName() + " |grep " + search_deb + " |cut -d' ' -f2 |head -n1");
        if (out.isEmpty()) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Cannot connect find %1 package").arg(search_deb));
            return false;
        } else {
            updateStatus(tr("<b>Running command...</b><p>") + "downloading: " + out, progress);
            downloadDeb(url, out);
        }
        progress += 10;
    }
    return true;
}

// install downloaded .debs
void MainWindow::installDebs(const QString& path) {
    QString cmd_str, out, ms;
    QDir dir;
    dir.setCurrent(path);
    bool error = false;

    //filter *.deb file only
    QStringList filter;
    filter << "*.deb";
    dir.setNameFilters(filter);

    QStringList fileList = dir.entryList();
    ui->groupBox->setTitle(tr("Installing downloaded files"));

    int size = fileList.size();
    if (size == 0) {
        QMessageBox::critical(this, tr("Error"),
                              tr("No downloaded *.debs files found."));
        qApp->exit(EXIT_FAILURE);
    }

    qDebug() << "filelist " << fileList;

    lock_file.unlock();
    QString cmd_str_2;
    if (arch_flag) {
        cmd_str_2 = "dpkg --remove libtxc-dxtn-s2tc:" + arch;
        cmd.run(cmd_str_2);
    }
    if (i386_flag) {
        cmd_str_2 = "dpkg --remove libtxc-dxtn-s2tc:i386";
        cmd.run(cmd_str_2);
    }

    int idx = ui->progressBar->value();
    int inc = (100 - idx) / fileList.size();

    for (const QString &file : fileList) {
        updateStatus(tr("<b>Installing...</b><p>") + file, idx += inc);
        if (!cmd.run("dpkg -i " + file)) {
            QMessageBox::critical(this, windowTitle(), QString(tr("Error installing %1")).arg(file));
            error = true;
        }
    }

    updateStatus("<b>" + tr("Fix missing dependencies...") + "</b><p>", 99);
    if (!cmd.run("apt-get -f install")) {
        QMessageBox::critical(this, windowTitle(), (tr("Error running %1 command").arg("'apt-get -f install'")));
        error = true;
    }

    ui->groupBox->setTitle(QString());
    updateStatus(tr("<b>Installation process has finished</b>"), 100);

    setCursor(QCursor(Qt::ArrowCursor));
    if (!error) {
        QMessageBox::information(this, tr("Finished"),
                                 tr("Codecs files have been downloaded and installed successfully."));
        qApp->exit(EXIT_SUCCESS);
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Process finished. Errors have occurred during the installation."));
        qApp->exit(EXIT_FAILURE);
    }
}

// show about
void MainWindow::on_buttonAbout_clicked() {
    this->hide();
    displayAboutMsgBox(tr("About MX Codecs"), "<p align=\"center\"><b><h2>" + this->windowTitle() +"</h2></b></p><p align=\"center\">" +
                       tr("Version: ") + qApp->applicationVersion() + "</p><p align=\"center\"><h3>" +
                       tr("Simple codecs downloader for MX Linux") +
                       "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>",
                       "file:///usr/share/doc/mx-codecs/license.html", tr("%1 License").arg(this->windowTitle()), true);
    this->show();
}

// Help button clicked
void MainWindow::on_buttonHelp_clicked() {
    QLocale locale;
    QString lang = locale.bcp47Name();

    QString url = "file:///usr/share/doc/mx-codecs/mx-codecs.html";

    if (lang.startsWith("fr")) {
        url = "https://mxlinux.org/french-wiki/help-files/help-mx-codecs-installer";
    }
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()), true);
}

