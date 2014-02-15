#include <QApplication>
#include "mxcodecs.h"
#include <unistd.h>



int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setWindowIcon(QIcon("/usr/share/icons/Tango/16x16/mimetypes/sound.png"));


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
