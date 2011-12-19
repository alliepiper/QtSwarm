#ifndef FLOCKWIDGET_H
#define FLOCKWIDGET_H

#include <QtCore/QLinkedList>

#include <QtGui/QWidget>

class Flocker;
class QTimer;

class FlockWidget : public QWidget
{
  Q_OBJECT
public:
  explicit FlockWidget(QWidget *parent = 0);
  virtual ~FlockWidget();

signals:
  
public slots:

protected slots:
  void takeStep();

protected:
  virtual void paintEvent(QPaintEvent *);

  void initializeFlockers();
  void cleanupFlockers();
  void addRandomFlocker();

  QTimer *m_timer;
  QLinkedList<Flocker*> m_flockers;
  unsigned int m_numFlockers;
  unsigned int m_flockerIdHead;
  unsigned int m_numTypes;
  double m_initialSpeed;
  double m_minSpeed;
  double m_maxSpeed;
};

#endif // FLOCKWIDGET_H
