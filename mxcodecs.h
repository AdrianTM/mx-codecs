#ifndef MXCODECS_H
#define MXCODECS_H

#include <QDialog>

#include <QMessageBox>
#include <QComboBox>
#include <QDir>

namespace Ui {
class mxcodecs;
}

class mxcodecs : public QDialog
{
  Q_OBJECT

public:
  explicit mxcodecs(QWidget *parent = 0);
  ~mxcodecs();
  // helpers
  static QString getCmdOut(QString cmd);
  void updateStatus(QString msg, int val);

  QString downloadDebs();
  void installDebs(QString path);

public slots:
  virtual void on_buttonOk_clicked();
  virtual void on_buttonAbout_clicked();

private:
  Ui::mxcodecs *ui;
};

#endif // MXCODECS_H
