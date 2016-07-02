#ifndef FLOCKWIDGET_H
#define FLOCKWIDGET_H

#include <QtCore/QDateTime>

#include <QtWidgets/QWidget>

class FlockEngine;

class FlockWidget : public QWidget
{
  Q_OBJECT
public:
  explicit FlockWidget(QWidget *parent = 0);
  virtual ~FlockWidget();

protected slots:
  void takeStep();

protected:
  virtual void paintEvent(QPaintEvent *);
  virtual void keyPressEvent(QKeyEvent *);
  virtual void mouseMoveEvent(QMouseEvent *);
  virtual void mousePressEvent(QMouseEvent *);
  virtual void mouseReleaseEvent(QMouseEvent *);

  void enableBlasts();
  void disableBlasts();

  void setClickPoint(const QPointF &loc);

  QTimer *m_timer;

  FlockEngine *m_engine;

  QDateTime m_lastRender;
  float m_currentFPS;
  float m_fpsSum;
  float m_fpsCount;

  bool m_aborted;
};

#endif // FLOCKWIDGET_H
