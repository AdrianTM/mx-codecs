#!/usr/bin/python
# -*- coding: utf-8 -*-

# mx-codecs.py

## Install restricted codecs ##
## jerry3904, kmathern, adrian and MEPISCommunity   ##
## http://forum.mepiscommunity.org  ##
## License: GPL v3      ##


import subprocess
import sys
import tempfile
import shutil
from os import geteuid
from os.path import basename

from PyQt4 import QtGui
from PyQt4 import QtCore

class ProgressBar(QtGui.QWidget):

    def __init__(self, task, parent=None):
        super(ProgressBar, self).__init__(parent)
        # Create a progress bar, message and a button and add them to the main layout

        self.progressBar = QtGui.QProgressBar(self)
        self.msg = QtGui.QLabel()

        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.progressBar)
        self.layout.addWidget(self.msg)
        self.button = QtGui.QPushButton('Close', self)
        self.layout.addWidget(self.button)
        self.setLayout(self.layout)
        self.setWindowTitle('Processing')
        
        self.task = task
        self.task.start()

        self.button.clicked.connect(self.onAbort)
        ##self.task.taskFinished.connect(self.onFinished)
        self.task.taskEvent.connect(self.changeMsg)

        self.progressBar.setRange(0,0)
        self.show()

    def onFinished(self):
        sys.exit()

    def onAbort(self):
        self.task.terminate()
        sys.exit()
    
    def changeMsg(self, val):
        text = val.leftJustified(80)
        self.msg.setText(text)
        if val.split(' ', 1)[0] == 'Installing':
            self.button.setEnabled(False)
        elif val.split(' ', 1)[0] == 'Done.':
            self.button.setEnabled(True)
            self.progressBar.setMaximum(1)
      
    def disableButton(self):
        """disable 'abort' button when installing debs to avoid corruption"""
        self.button.setDisabled(True)
        


class TaskThread(QtCore.QThread):
    """runs a number of commands in a thread and emits signals after each command"""
    ##taskFinished = QtCore.pyqtSignal()
    taskEvent = QtCore.pyqtSignal(str)
    
    def __init__(self):
        QtCore.QThread.__init__(self)
    
    def run(self):
        self.downloadDebs()
        self.installDebs()
        shutil.rmtree(dir)
        ##self.taskFinished.emit()

    def downloadDebs(self):
        """Download codecs debs files"""

        self.taskEvent.emit('Getting info...\n')
        url = 'http://deb-multimedia.org'
        arch = subprocess.check_output(['dpkg', '--print-architecture']).rstrip()

        args = 'wget -qO- ' + url + '/dists/stable/main/binary-' + arch + '/Packages.gz | zgrep ^Filename | grep libdvdcss2 | awk \'{print $2}\''
        out = runCmd(args, dir)
        self.taskEvent.emit('Downloading...\n' + basename(out))
        args = 'wget -q ' + url + '/' + out
        print "Running: ", args
        runCmd(args, dir)

        args = 'wget -qO- ' + url + '/dists/stable/non-free/binary-' + arch + '/Packages.gz | zgrep ^Filename | grep w.*codecs | awk \'{print $2}\''
        out = runCmd(args, dir)
        self.taskEvent.emit('Downloading...\n' + basename(out))
        args = 'wget -q ' + url + '/' + out
        print "Running: ", args
        runCmd(args, dir)

    def installDebs(self):
        """Install all debs from tmp dir"""
      
        self.taskEvent.emit('Installing codecs...\n')
        out = runCmd('dpkg -i *.deb', dir)
        print 'Dpkg returned:\n--------------\n' + out + '--------------\n'
        self.taskEvent.emit('Done. \n')



def checkRoot():
    if geteuid() != 0:
        QtGui.QMessageBox.critical(None, "mx-codecs", 'You need to run the application as root', QtGui.QMessageBox.Yes)
        sys.exit(1)
    

  
def acceptResp():
    """accepting reponsibility dialog"""

    reply = QtGui.QMessageBox.question(None, "mx-codecs", 'This application allows you to install restricted codecs that permit advanced video and audio functions.\n\n'
        'In some juridictions their distribution may be limited so the user must meet local regulations. \n \n'
        'Do you assume legal responsibility for downloading these codecs?', QtGui.QMessageBox.Yes |
        QtGui.QMessageBox.No, QtGui.QMessageBox.No)

    if reply != QtGui.QMessageBox.Yes:
        sys.exit()
    else:
        return True


def runCmd(args, dir):
    """Runs bash command in directory and returns output"""
    
    run = subprocess.Popen(args, shell=True, cwd=dir, stdout=subprocess.PIPE)
    return run.stdout.read()

def main():
    """General application code"""

    global app, dir

    app = QtGui.QApplication(sys.argv)
    checkRoot()

    dir = tempfile.mkdtemp()

    if (acceptResp()):
        task = TaskThread()
        prbar = ProgressBar(task)

    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
