/*****************************************************************************
 * main.cpp
 *****************************************************************************
 * Copyright (C) 2014 MX Authors
 * 
 * Authors: Jerry 3904
 *          Anticaptilista
 *          Adrian
 *          MEPIS Community <http://forum.mepiscommunity.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/


#include <QApplication>
#include <qtranslator.h>
#include <qlocale.h>
#include "mxcodecs.h"
#include <unistd.h>



int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setWindowIcon(QIcon("/usr/share/icons/Tango/16x16/mimetypes/sound.png"));
  
  QTranslator qtTran;
  qtTran.load(QString("qt_") + QLocale::system().name());
  a.installTranslator(&qtTran);
  
  QTranslator appTran;
  appTran.load(QString("mx-codecs_") + QLocale::system().name(), "/usr/share/mx-codecs/locale");
  a.installTranslator(&appTran);


  if (getuid() == 0) {
    mxcodecs w;
    w.show();

    return a.exec();

  } else {
    QApplication::beep();
    QMessageBox::critical(0, QString::null,
                          QApplication::tr("You must run this program as root."));
    return 1;
  }

}
