#ifndef FLOCKWIDGET_H
#define FLOCKWIDGET_H

#include <QtCore/QLinkedList>
#include <QtCore/QDateTime>

#include <QtWidgets/QWidget>

#include <Eigen/Core>

class Blast;
class Entity;
class Flocker;
class Predator;
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
  virtual void keyPressEvent(QKeyEvent *);
  virtual void mouseMoveEvent(QMouseEvent *);
  virtual void mousePressEvent(QMouseEvent *);
  virtual void mouseReleaseEvent(QMouseEvent *);

  void initializeFlockers();
  void cleanupFlockers();
  void addFlockerFromEntity(const Entity *e);
  void addRandomFlocker();
  void removeFlocker(Flocker *f);

  void initializePredators();
  void cleanupPredators();
  void addRandomPredator(const unsigned int type);
  void removePredator(Predator *p);

  void initializeTargets();
  void cleanupTargets();
  void addRandomTarget(const unsigned int type);
  void removeTarget(Target *t);
  void randomizeTarget(Target *t);

  void enableBlasts();
  void disableBlasts();
  void addBlastFromEntity(const Entity *e);
  void removeBlast(Blast *b);

  // 0->1
  void randomizeVector(Eigen::Vector3d *vec);
  QColor typeToColor(const unsigned int type);

  QTimer *m_timer;
  QLinkedList<Entity*> m_entities;
  QLinkedList<Flocker*> m_flockers;
  QVector<QLinkedList<Target*> > m_targets;
  QLinkedList<Predator*> m_predators;
  QLinkedList<Blast*> m_blasts;
  unsigned int m_entityIdHead;
  unsigned int m_numFlockers;
  unsigned int m_numFlockerTypes;
  unsigned int m_numPredators;
  unsigned int m_numPredatorTypes;
  unsigned int m_numTargetsPerFlockerType;
  double m_initialSpeed;
  double m_minSpeed;
  double m_maxSpeed;

  QDateTime m_lastRender;
  float m_currentFPS;
  float m_fpsSum;
  float m_fpsCount;

  bool m_aborted;
  bool m_showBlasts;

  void setClickPoint(const QPointF &);
  bool m_clicked;
  Eigen::Vector3d m_clickPoint;

  struct TakeStepFunctor;
  friend struct TakeStepFunctor;
  struct TakeStepResult;
  TakeStepResult takeStepWorker(const Flocker *f_i) const;

};

#endif // FLOCKWIDGET_H
