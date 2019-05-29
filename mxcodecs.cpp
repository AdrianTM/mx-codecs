/*****************************************************************************
 * mx-codecs.cpp
 *****************************************************************************
 * Copyright (C) 2014 MX Authors with the exeption of getCmdOut function
 *  getCmdOut function copyright (C) 2003-2014 Warren Woodford
 *   released under the Apache License version 2.0
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

#include "mxcodecs.h"
#include "ui_mxcodecs.h"

#include <stdio.h>

#include <QDir>
#include <QTextEdit>
#include <QDebug>



mxcodecs::mxcodecs(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::mxcodecs), lock_file("/var/lib/dpkg/lock")
{
  ui->setupUi(this);
  if (ui->buttonOk->icon().isNull()) {
      ui->buttonOk->setIcon(QIcon(":/icons/dialog-ok.svg"));
  }
}

mxcodecs::~mxcodecs()
{
  delete ui;
}

void mxcodecs::updateStatus(QString msg, int val) {
  ui->labelDownload->setText(msg);
  ui->progressBar->setValue(val);
  qApp->processEvents();
}

void mxcodecs::displayDoc(QString url)
{
    QString exec = "xdg-open";
    QString user = cmd.getOutput("logname");
    if (system("command -v mx-viewer") == 0) { // use mx-viewer if available
        exec = "mx-viewer";
    }
    QString cmd = "su " + user + " -c \"" + exec + " " + url + "\"&";
    system(cmd.toUtf8());
}

// Get version of the program
QString mxcodecs::getVersion(QString name) {
    Cmd cmd;
    return cmd.getOutput("dpkg-query -f '${Version}' -W " + name);
}

/////////////////////////////////////////////////////////////////////////
// general

void mxcodecs::on_buttonOk_clicked() {
  if (ui->stackedWidget->currentIndex() == 0) {
    setCursor(QCursor(Qt::WaitCursor));
    installDebs(downloadDebs());
    setCursor(QCursor(Qt::ArrowCursor));
  } else {
    qApp->exit(0);
  }
}

//download .deb codecs returns download path
QString mxcodecs::downloadDebs() {
  QString cmd_str, out, msg;
  QString path, arch, release;
  QString url = "http://deb-multimedia.org";

  //set progressBar and refresh
  ui->progressBar->setValue(0);
  ui->stackedWidget->setCurrentIndex(1);
  qApp->processEvents();

  // create temp folder and set it current
  path = cmd.getOutput("mktemp -d");
  QDir dir(path);
  dir.mkdir(path);
  dir.setCurrent(path);

  // get arch info
  arch = cmd.getOutput("dpkg --print-architecture");

  // get release info
  release = cmd.getOutput("grep VERSION /etc/os-release | grep -Eo [a-z]+ ");

  cmd_str = "wget -qO- " + url + "/dists/" + release + "/main/binary-" + arch + "/Packages.gz | zgrep ^Filename | grep libdvdcss2 | awk \'{print $2}\'";
  updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 10);
  out = cmd.getOutput(cmd_str);
  if (out == "") {
    QMessageBox::critical(0, tr("Error"),
                          tr("Cannot connect to the download site"));
  } else {
    cmd_str = "wget -q " + url + "/" + out;
    updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 20);
    if (cmd.run(cmd_str) != 0) {
      QMessageBox::critical(0, windowTitle(),
                            QString(tr("Error downloading %1")).arg(out));
    }
  }

  cmd_str = "wget -qO- " + url + "/dists/" + release + "/non-free/binary-" + arch + "/Packages.gz | zgrep ^Filename | grep w.*codecs | awk \'{print $2}\'";
  updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 30);
  out = cmd.getOutput(cmd_str);
  if (out == "") {
    QMessageBox::critical(0, tr("Error"),
                          tr("Cannot connect to the download site"));
  } else {
    cmd_str = "wget -q " + url + "/" + out;
    updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 40);
    if (cmd.run(cmd_str) != 0) {
      QMessageBox::critical(0, tr("Error"),
                            QString(tr("Error downloading %1")).arg(out));
    }
  }

  cmd_str = "wget -qO- " + url + "/dists/" + release + "/main/binary-" + arch + "/Packages.gz | zgrep ^Filename | grep libtxc-dxtn0 | awk \'{print $2}\'";
  updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 50);
  out = cmd.getOutput(cmd_str);
  if (out == "") {
    QMessageBox::critical(0, tr("Error"),
                          tr("Cannot connect to the download site"));
  } else {
    cmd_str = "wget -q " + url + "/" + out;
    updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 60);
    if (cmd.run(cmd_str) != 0) {
      QMessageBox::critical(0, tr("Error"),
                            QString(tr("Error downloading %1")).arg(out));
    }
  }
    //if 64 bit, also install 32 bit libtxc-dxtn0 package
  if (arch == "amd64") {
      cmd_str = "wget -qO- " + url + "/dists/" + release + "/main/binary-i386/Packages.gz | zgrep ^Filename | grep libtxc-dxtn0 | awk \'{print $2}\'";
      updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 70);
      out = cmd.getOutput(cmd_str);
      if (out == "") {
        QMessageBox::critical(0, tr("Error"),
                              tr("Cannot connect to the download site"));
      } else {
        cmd_str = "wget -q " + url + "/" + out;
        updateStatus(tr("<b>Running command...</b><p>") + cmd_str, 75);
        if (cmd.run(cmd_str) != 0) {
          QMessageBox::critical(0, tr("Error"),
                                QString(tr("Error downloading %1")).arg(out));
        }
      }
  }

  updateStatus(tr("<b>Download Finished.</b>"), 85);

  return path;
}

//install downloaded .debs
void mxcodecs::installDebs(QString path) {
  QString cmd_str, out, msg;
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
    QMessageBox::critical(0, tr("Error"),
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
  if (cmd.run(cmd_str) != 0) {
      QMessageBox::critical(0, windowTitle(),
                            QString(tr("Error installing %1")).arg(file));
      error = true;
  }

  updateStatus("<b>" + tr("Fix missing dependencies...") + "</b><p>", 99);
  if (cmd.run("apt-get -f install")) {
      QMessageBox::critical(0, windowTitle(), (tr("Error running 'apt-get -fm install' command")));
      error = true;
  }


  while (!fileList.isEmpty()) {
      file = fileList.takeFirst();
      dir.remove(file);
  }

  dir.rmdir(path);
  ui->groupBox->setTitle("");
  updateStatus(tr("<b>Installation process has finished</b>"), 100);

  if (!error) {
    QMessageBox::information(0, tr("Finished"),
                             tr("Codecs files have been downloaded and installed successfully."));
    qApp->exit(0);
  } else {
    QMessageBox::critical(0, tr("Error"),
                             tr("Process finished. Errors have occurred during the installation."));
    qApp->exit(1);
  }
}


/////////////////////////////////////////////////////////////////////////
// slots


// show about
void mxcodecs::on_buttonAbout_clicked() {
    Cmd cmd;
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX Codecs"), "<p align=\"center\"><b><h2>" +
                       tr("MX Codecs") + "</h2></b></p><p align=\"center\">" + tr("Version: ") +
                       getVersion("mx-codecs") + "</p><p align=\"center\"><h3>" +
                       tr("Simple codecs downloader for MX Linux") + "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>", 0, this);
    QPushButton *btnLicense = msgBox.addButton(tr("License"), QMessageBox::HelpRole);
    QPushButton *btnChangelog = msgBox.addButton(tr("Changelog"), QMessageBox::HelpRole);
    QPushButton *btnCancel = msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    btnCancel->setIcon(QIcon::fromTheme("window-close"));

    msgBox.exec();

    if (msgBox.clickedButton() == btnLicense) {
        displayDoc("file:///usr/share/doc/mx-codecs/license.html");
    } else if (msgBox.clickedButton() == btnChangelog) {
        QDialog *changelog = new QDialog(this);
        changelog->resize(600, 500);

        QTextEdit *text = new QTextEdit;
        text->setReadOnly(true);
        text->setText(cmd.getOutput("zless /usr/share/doc/" + QFileInfo(QCoreApplication::applicationFilePath()).fileName()  + "/changelog.gz"));

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
void mxcodecs::on_buttonHelp_clicked() {
    QLocale locale;
    QString lang = locale.bcp47Name();

    QString url = "https://mxlinux.org/wiki/help-files/help-mx-codecs-installer";

    if (lang.startsWith("fr")) {
        url = "https://mxlinux.org/french-wiki/help-files/help-mx-codecs-installer";
    }
    displayDoc(url);
}

