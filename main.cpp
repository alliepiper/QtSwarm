#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QPainter>
#include <QtGui/QWidget>

#include <QtCore/QDebug>

#include <flockwidget.h>

#include <string.h>

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  bool fullscreen = false;
  if (argc >= 2) {
    int argInd = 0;
    while (char *arg = argv[argInd++]) {
      if (std::strcmp(arg, "-f") == true) {
        fullscreen = true;
      }
    }
  }

  QMainWindow mw;
  QWidget *target = new FlockWidget(&mw);
  mw.setCentralWidget(target);

  if (fullscreen) {
    mw.showFullScreen();
  }
  else {
    QRect rect = mw.geometry();
    rect.setWidth(600);
    rect.setHeight(600);
    mw.setGeometry(rect);
    mw.show();
  }


  return app.exec();
}
