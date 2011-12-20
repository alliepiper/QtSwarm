#ifndef FLOCKWIDGET_H
#define FLOCKWIDGET_H

#include <QtCore/QLinkedList>

#include <QtGui/QWidget>

#include <Eigen/Core>

class Entity;
class Flocker;
class QTimer;
class Target;

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

  void initializeFlockers();
  void cleanupFlockers();
  void initializeTargets();
  void cleanupTargets();
  void addRandomFlocker();
  void removeFlocker(Flocker *f);
  void addRandomTarget(const unsigned int type);
  void removeTarget(Target *t);
  void randomizeTarget(Target *t);

  // 0->1
  void randomizeVector(Eigen::Vector3d *vec);
  QColor typeToColor(const unsigned int type);

  QTimer *m_timer;
  QLinkedList<Entity*> m_entities;
  QLinkedList<Flocker*> m_flockers;
  QVector<QLinkedList<Target*> > m_targets;
  unsigned int m_numFlockers;
  unsigned int m_flockerIdHead;
  unsigned int m_numTypes;
  unsigned int m_numTargetTypes;
  unsigned int m_numTargetsPerType;
  unsigned int m_targetIdHead;
  double m_initialSpeed;
  double m_minSpeed;
  double m_maxSpeed;
};

#endif // FLOCKWIDGET_H
