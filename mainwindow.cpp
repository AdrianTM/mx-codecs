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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include "cmd.h"

#include <QDir>
#include <QTextEdit>

#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainWindow), lock_file("/var/lib/dpkg/lock")
{
    qDebug().noquote() << QCoreApplication::applicationName() << "version:" << VERSION;
    ui->setupUi(this);

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

void MainWindow::displayDoc(const QString& url) const
{
    Cmd cmd;
    QString user = cmd.getCmdOut("logname", true);
    if (system("command -v mx-viewer >/dev/null") == 0) {
        system("mx-viewer " + url.toUtf8());
    } else {
        system("runuser -l " + user.toUtf8() + " -c \"env XDG_RUNTIME_DIR=/run/user/$(id -u " + user.toUtf8() + ") xdg-open " + url.toUtf8() + "\"&");
    }
}

void MainWindow::on_buttonOk_clicked() {
    if (ui->stackedWidget->currentIndex() == 0) {
        setCursor(QCursor(Qt::WaitCursor));
        installDebs(downloadDebs());
    } else {
        qApp->exit(0);
    }
}

//download .deb codecs returns download path
QString MainWindow::downloadDebs() {
    QString cmd_str, out, msg;
    QString path, release;
    QString url = "http://deb-multimedia.org";

    //set progressBar and refresh
    ui->progressBar->setValue(0);
    ui->stackedWidget->setCurrentIndex(1);
    qApp->processEvents();

    // create temp folder and set it current
    path = cmd.getCmdOut("mktemp -d");

    QDir dir(path);
    dir.mkdir(path);
    dir.setCurrent(path);

    // get release info
    release = cmd.getCmdOut("grep VERSION= /etc/os-release | grep -Eo [a-z]+ ");

    cmd_str = "wget -qO- " + url + "/dists/" + release + "/main/binary-" + arch + "/Packages.gz | zgrep ^Filename | grep libdvdcss2 | awk \'{print $2}\'";
    updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 10);
    out = cmd.getCmdOut(cmd_str);
    if (out == "") {
        QMessageBox::critical(this, tr("Error"),
                              tr("Cannot connect to the download site"));
    } else {
        cmd_str = "wget -q " + url + "/" + out;
        updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 20);
        if (!cmd.run(cmd_str)) {
            QMessageBox::critical(this, windowTitle(),
                                  QString(tr("Error downloading %1")).arg(out));
        }
    }

    cmd_str = "wget -qO- " + url + "/dists/" + release + "/non-free/binary-" + arch + "/Packages.gz | zgrep ^Filename | grep w.*codecs | awk \'{print $2}\'";
    updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 30);
    out = cmd.getCmdOut(cmd_str);
    if (out == "") {
        arch_flag = false;
        QMessageBox::critical(this, tr("Error"),
                              tr("Cannot connect to the download site"));
    } else {
        cmd_str = "wget -q " + url + "/" + out;
        updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 40);
        if (!cmd.run(cmd_str)) {
            arch_flag = false;
            QMessageBox::critical(this, tr("Error"),
                                  QString(tr("Error downloading %1")).arg(out));
        }
    }

    cmd_str = "wget -qO- " + url + "/dists/" + release + "/main/binary-" + arch + "/Packages.gz | zgrep ^Filename | grep libtxc-dxtn0 | awk \'{print $2}\'";
    updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 50);
    out = cmd.getCmdOut(cmd_str);
    if (out == "") {
        QMessageBox::critical(this, tr("Error"),
                              tr("Cannot connect to the download site"));
    } else {
        cmd_str = "wget -q " + url + "/" + out;
        updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 60);
        if (!cmd.run(cmd_str)) {
            QMessageBox::critical(this, tr("Error"),
                                  QString(tr("Error downloading %1")).arg(out));
        }
    }
    //if 64 bit, also install 32 bit libtxc-dxtn0 package
    if (arch == "amd64") {
        cmd_str = "wget -qO- " + url + "/dists/" + release + "/main/binary-i386/Packages.gz | zgrep ^Filename | grep libtxc-dxtn0 | awk \'{print $2}\'";
        updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 70);
        out = cmd.getCmdOut(cmd_str);
        if (out == "") {
            i386_flag = false;
            QMessageBox::critical(this, tr("Error"),
                                  tr("Cannot connect to the download site"));
        } else {
            cmd_str = "wget -q " + url + "/" + out;
            updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 75);
            if (!cmd.run(cmd_str)) {
                i386_flag =false;
                QMessageBox::critical(this, tr("Error"),
                                      QString(tr("Error downloading %1")).arg(out));
            }
        }
    }

    updateStatus(tr("<b>Download Finished.</b>"), 85);

    return path;
}

//install downloaded .debs
void MainWindow::installDebs(const QString& path) {
    QString cmd_str, out, ms;
    QDir dir(path);
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
        qApp->exit(1);
    }

    qDebug() << "filelist " << fileList;

    QString file;
    foreach (const QString &item, fileList) {
        file.append("./" + item + " ");
    }

    qDebug() << "file " << file;
    cmd_str = QString("dpkg -i %1").arg(file);
    updateStatus(tr("<b>Installing...</b><p>")+file, 95);
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

    if (!cmd.run(cmd_str)) {
        QMessageBox::critical(this, windowTitle(), QString(tr("Error installing %1")).arg(file));
        error = true;
    }

    updateStatus("<b>" + tr("Fix missing dependencies...") + "</b><p>", 99);
    if (!cmd.run("apt-get -f install")) {
        QMessageBox::critical(this, windowTitle(), (tr("Error running %1 command").arg("'apt-get -f install'")));
        error = true;
    }


    while (!fileList.isEmpty()) {
        file = fileList.takeFirst();
        dir.remove(file);
    }

    dir.rmdir(path);
    ui->groupBox->setTitle("");
    updateStatus(tr("<b>Installation process has finished</b>"), 100);

    setCursor(QCursor(Qt::ArrowCursor));
    if (!error) {
        QMessageBox::information(this, tr("Finished"),
                                 tr("Codecs files have been downloaded and installed successfully."));
        qApp->exit(0);
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Process finished. Errors have occurred during the installation."));
        qApp->exit(1);
    }
}


/////////////////////////////////////////////////////////////////////////
// slots


// show about
void MainWindow::on_buttonAbout_clicked() {
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX Codecs"), "<p align=\"center\"><b><h2>" +
                       tr("MX Codecs") + "</h2></b></p><p align=\"center\">" + tr("Version: ") +
                       VERSION + "</p><p align=\"center\"><h3>" +
                       tr("Simple codecs downloader for MX Linux") + "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>", nullptr, this);
    QPushButton *btnLicense = msgBox.addButton(tr("License"), QMessageBox::HelpRole);
    QPushButton *btnChangelog = msgBox.addButton(tr("Changelog"), QMessageBox::HelpRole);
    QPushButton *btnCancel = msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    btnCancel->setIcon(QIcon::fromTheme("window-close"));

    msgBox.exec();

    if (msgBox.clickedButton() == btnLicense) {
        displayDoc("file:///usr/share/doc/mx-codecs/license.html");
    } else if (msgBox.clickedButton() == btnChangelog) {
        QDialog *changelog = new QDialog(this);
        changelog->setWindowTitle(tr("Changelog"));
        changelog->resize(600, 500);

        QTextEdit *text = new QTextEdit;
        text->setReadOnly(true);
        Cmd cmd;
        text->setText(cmd.getCmdOut("zless /usr/share/doc/" + QFileInfo(QCoreApplication::applicationFilePath()).fileName()  + "/changelog.gz"));

        QPushButton *btnClose = new QPushButton(tr("&Close"));
        btnClose->setIcon(QIcon::fromTheme("window-close"));
        connect(btnClose, &QPushButton::clicked, changelog, &QDialog::close);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(text);
        layout->addWidget(btnClose);
        changelog->setLayout(layout);
        changelog->exec();
    }
    this->show();
}

// Help button clicked
void MainWindow::on_buttonHelp_clicked() {
    QLocale locale;
    QString lang = locale.bcp47Name();

    QString url = "/usr/share/doc/mx-codecs/mx-codecs.html";

    if (lang.startsWith("fr")) {
        url = "https://mxlinux.org/french-wiki/help-files/help-mx-codecs-installer";
    }
    displayDoc(url);
}

