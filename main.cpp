#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QPainter>
#include <QtGui/QWidget>

#include <flockwidget.h>

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  QMainWindow mw;
  QWidget *target = new FlockWidget(&mw);
  mw.setCentralWidget(target);
  QRect rect = mw.geometry();
  rect.setWidth(600);
  rect.setHeight(600);
  mw.setGeometry(rect);

  mw.show();

  return app.exec();
}
