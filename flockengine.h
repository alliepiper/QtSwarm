#ifndef FLOCKENGINE_H
#define FLOCKENGINE_H

#include <QtCore/QObject>

#include <QtCore/QFuture>
#include <QtCore/QLinkedList>

#include <Eigen/Core>

class Blast;
class Entity;
class Flocker;
class Predator;
class QTimer;
class Target;

class FlockEngine : public QObject
{
  Q_OBJECT
public:
  explicit FlockEngine(QObject *parent = 0);
  ~FlockEngine();

  const QLinkedList<Entity*>& entities() const { return m_entities; }
  const QLinkedList<Flocker*>& flockers() const { return m_flockers; }
  const QVector<QLinkedList<Target*>>& targets() const { return m_targets; }
  const QLinkedList<Predator*>& predators() const { return m_predators; }
  const QLinkedList<Blast*>& blasts() const { return m_blasts; }

  const Eigen::Vector3d& forceTarget() const;
  void setForceTarget(const Eigen::Vector3d &v);

  bool useForceTarget() const;
  void setUseForceTarget(bool use);

  void computeNextStep();
  void commitNextStep();

  bool createBlasts() const;
  void setCreateBlasts(bool b);

  QColor typeToColor(const unsigned int type);

  unsigned int numFlockerTypes() const;

  unsigned int numTargetsPerFlockerType() const;

private:
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

  void addBlastFromEntity(const Entity *e);
  void removeBlast(Blast *b);

  void randomizeVector(Eigen::Vector3d *vec);

  struct TakeStepFunctor;
  friend struct TakeStepFunctor;
  struct TakeStepResult;
  TakeStepResult takeStepWorker(const Flocker *f_i) const;

private:
  QLinkedList<Entity*> m_entities;
  QLinkedList<Flocker*> m_flockers;
  QVector<QLinkedList<Target*> > m_targets;
  QLinkedList<Predator*> m_predators;
  QLinkedList<Blast*> m_blasts;

  bool m_useForceTarget;
  Eigen::Vector3d m_forceTarget;

  bool m_createBlasts;

  unsigned int m_entityIdHead;
  unsigned int m_numFlockers;
  unsigned int m_numFlockerTypes;
  unsigned int m_numPredators;
  unsigned int m_numPredatorTypes;
  unsigned int m_numTargetsPerFlockerType;

  double m_initialSpeed;
  double m_minSpeed;
  double m_maxSpeed;

  QFuture<FlockEngine::TakeStepResult> m_future;
};

#endif // FLOCKENGINE_H
