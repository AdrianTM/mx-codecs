/*****************************************************************************
 * mxcodecs.h
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

#ifndef MXCODECS_H
#define MXCODECS_H

#include <QDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QDir>
#include <QProcess>

#include "lockfile.h"

namespace Ui {
class mxcodecs;
}

class mxcodecs : public QDialog
{
    Q_OBJECT

public:
    explicit mxcodecs(QWidget *parent = 0);
    ~mxcodecs();
    bool i386_flag = true;
    bool arch_flag = true;

    void updateStatus(const QString &msg, int val);
    void displayDoc(const QString &url) const;
    void installDebs(const QString &path);

    QString downloadDebs();

public slots:
    virtual void on_buttonOk_clicked();
    virtual void on_buttonAbout_clicked();
    virtual void on_buttonHelp_clicked();

private:
    Ui::mxcodecs *ui;
    LockFile lock_file;
    QString arch;
};

#endif // MXCODECS_H
