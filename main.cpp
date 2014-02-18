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
