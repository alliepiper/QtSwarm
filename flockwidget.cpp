#include <Eigen/Core>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtGui/QPainter>

#include <stdlib.h>
#include <time.h>

#include "flockwidget.h"
#include "flocker.h"
#include "predator.h"
#include "target.h"

FlockWidget::FlockWidget(QWidget *parent) :
  QWidget(parent),
  m_timer(new QTimer (this)),
  m_entityIdHead(0),
  m_numFlockers(500),
  m_numFlockerTypes(12),
  m_numPredators(30),
  m_numTargetTypes(12),
  m_numTargetsPerType(3),
  m_initialSpeed(0.0020),
  m_minSpeed(    0.0035),
  m_maxSpeed(    0.0075),
  m_lastRender(QDateTime::currentDateTime()),
  m_currentFPS(0.f),
  m_fpsSum(0.f),
  m_fpsCount(0)
{
  this->initializeFlockers();
  this->initializePredators();
  this->initializeTargets();

  // Initialize RNG
  srand(time(NULL));

  connect(m_timer, SIGNAL(timeout()), this, SLOT(takeStep()));

  m_timer->start(20);
}

FlockWidget::~FlockWidget()
{
  this->cleanupFlockers();
  this->cleanupPredators();
  this->cleanupTargets();
}

// See http://www.musicdsp.org/showone.php?id=222 for more info
inline float fastexp5(float x) {
    return (120+x*(120+x*(60+x*(20+x*(5+x)))))*0.0083333333f;
}

inline double V_morse(const double r) {
  const double depth = 1.00;
  const double rad   = 0.10;
  const double alpha = 2.00;

  const double tmpTerm = ( 1 - fastexp5( -alpha * (r - rad) ) );
  double V = depth * tmpTerm * tmpTerm;

  return V - depth;
}

// Numerical derivatives
inline double V_morse_ND(const double r) {
  const double delta = r * 1e-5;
  const double v1 = V_morse(r - delta);
  const double v2 = V_morse(r + delta);
  return (v2 - v1) / (delta + delta);
}

void FlockWidget::takeStep()
{
  QDateTime now = QDateTime::currentDateTime();
  qint64 elapsed_ms = m_lastRender.msecsTo(now);
  float elapsed_s = elapsed_ms / 1000.f;
  m_lastRender = now;
  m_currentFPS = 1.f / elapsed_s;
  m_fpsSum += m_currentFPS;
  ++m_fpsCount;

  QVector<Eigen::Vector3d> newDirections;
  newDirections.reserve(m_flockers.size());

  // These flockers need to be removed:
  QVector<Flocker*> deadFlockers;

  // These targets have been eaten:
  QVector<Target*> deadTargets;

  // Allocate loop variables
  Eigen::Vector3d samePotForce;
  Eigen::Vector3d diffPotForce;
  Eigen::Vector3d alignForce;
  Eigen::Vector3d predatorForce;
  Eigen::Vector3d targetForce;
  Eigen::Vector3d r;
  // Weight for each competing force
  double diffPotWeight  = 0.25; // morse potential, all type()s
  double samePotWeight  = 0.50; // morse potential, same type()
  double alignWeight    = 0.50; // Align to average neighbor heading
  double predatorWeight = 1.50; // 1/r^2 attraction/repulsion to all predators
  double targetWeight   = 0.75; // 1/r^2 attraction to all targets
  // newDirection = (oldDirection + (factor) * maxTurn * force).normalized()
  const double maxTurn = 0.25;
  // velocity *= 1.0 + speedupFactor * direction.dot(samePotForce)
  const double speedupFactor = 0.075;

  // Normalize force weights:
  double invWeightSum = 1.0 / (diffPotWeight + samePotWeight + alignWeight +
                               predatorWeight + targetWeight);
  diffPotWeight  *= invWeightSum;
  samePotWeight  *= invWeightSum;
  alignWeight    *= invWeightSum;
  predatorWeight *= invWeightSum;
  targetWeight   *= invWeightSum;

  // Loop through each flocker and predator:
  foreach (Flocker *f_i, m_flockers) {
    // Reset forces
    samePotForce.setZero();
    diffPotForce.setZero();
    alignForce.setZero();
    predatorForce.setZero();
    targetForce.setZero();

    // Calculate a distance-weighted average vector towards the relevant
    // targets
    foreach (Target *t, m_targets[f_i->type()]) {
      r = t->pos() - f_i->pos();
      const double rNorm = r.norm();
      targetForce += (1.0/(rNorm*rNorm*rNorm)) * r;
      if (rNorm < 0.025) {
        deadTargets.push_back(t);
      }
    }

    // Average together V(|r_ij|) * r_ij
    foreach (Flocker *f_j, m_flockers) {
      if (f_i == f_j) continue;
      r = f_j->pos() - f_i->pos();

      // General cutoff
      if (r.x() > 0.3 || r.y() > 0.3 || r.z() > 0.3) {
        continue;
      }

      const double rNorm = r.norm();
      const double rInvNorm = 1.0 / rNorm;

      Predator *pred_i = qobject_cast<Predator*>(f_i);
      Predator *pred_j = qobject_cast<Predator*>(f_j);
      // Both or neither are predators, use morse potential
      if ((!pred_i && !pred_j) ||
          ( pred_i &&  pred_j) ){
        double V = std::numeric_limits<double>::max();

        // Apply cutoff for morse interaction
        if (rNorm < 0.10) {
          V = V_morse_ND(rNorm);
          diffPotForce += (V*rInvNorm*rInvNorm) * r;
        }

        // Alignment -- steer towards the heading of nearby flockers
        if (f_i->type() == f_j->type() &&
            rNorm < 0.20 && rNorm > 0.001) {
          if (V == std::numeric_limits<double>::max()) {
            V = V_morse_ND(rNorm);
          }
          samePotForce += (V *rInvNorm*rInvNorm) * r;
          alignForce += rInvNorm * f_j->direction();
        }
      }
      // One is a predator, one is not. Evade / Pursue
      else {
        Predator *p;
        Flocker *f;
        if (pred_i) {
          p = pred_i;
          f = f_j;
        }
        else {
          p = pred_j;
          f = f_i;
        }

        // Calculate distance between them
        r = f->pos() - p->pos();
        const double rNorm = r.norm();
        const double rInvNorm = 1.0 / rNorm;
        // The flocker is being updated
        if (f == f_i) {
          if (rNorm < 0.3) {
            // Did the predator catch the flocker?
            if (rNorm < 0.025) {
              deadFlockers.append(f);
            }
            //               normalize          1/r^2        vector
            predatorForce += rInvNorm * (rInvNorm * rInvNorm) * r;
          }
        }
        // The predator is being updated
        else {
          // Cutoff distance for predator
          if (rNorm < 0.15) {
            //               normalize          1/r^2        vector
            predatorForce += rInvNorm * (rInvNorm * rInvNorm) * r;
          }
        }
      }
    }

    if (!diffPotForce.isZero(0.1)) {
      diffPotForce.normalize();
    }
    if (!samePotForce.isZero(0.1)) {
      samePotForce.normalize();
    }
    if (!alignForce.isZero(0.1)) {
      alignForce.normalize();
    }
    if (!predatorForce.isZero(0.1)) {
      predatorForce.normalize();
    }
    if (!targetForce.isZero(0.1)) {
      targetForce.normalize();
    }

    // Scale the force so that it will turn faster when "direction" is not
    // aligned well with force:
    Eigen::Vector3d force (samePotWeight  * samePotForce  +
                           diffPotWeight  * diffPotForce  +
                           alignWeight    * alignForce    +
                           predatorWeight * predatorForce +
                           targetWeight   * targetForce   );
    force.normalize();
    // cap dot factor, ensures that *some* finite turning will occur
    double directionDotForce = f_i->direction().dot(force);
    if (directionDotForce > 0.85) {
      directionDotForce = 0.85;
    }
    const double scale ( (1 - 0.5 * (directionDotForce + 1)) *
                         maxTurn);

    // Accelerate towards goal
    const double directionDotPotForce = f_i->direction().dot(samePotForce);
    f_i->velocity() *= 1.0 + speedupFactor * directionDotPotForce;
    if (f_i->velocity() < m_minSpeed) f_i->velocity() = m_minSpeed;
    else if (f_i->velocity() > m_maxSpeed) f_i->velocity() = m_maxSpeed;

    // Keep new direction
    newDirections.append((f_i->direction() + scale * force).normalized());
  }

  // Update flocker directions:
  int ind = 0;
  foreach (Flocker *f, m_flockers) {
    f->direction() = newDirections[ind++];
  }

  // Take steps
  foreach (Entity *e, m_entities) {
    e->takeStep();
  }

  // Replace dead flockers
  foreach (Flocker *f, deadFlockers) {
//    this->addRandomFlocker(f->type());
//    this->addRandomFlocker();
    this->removeFlocker(f);
  }

  // Randomize dead targets
  foreach (Target *t, deadTargets) {
    this->addFlockerFromEntity(t);
    this->randomizeTarget(t);
  }

  this->update();
}

void FlockWidget::paintEvent(QPaintEvent *)
{
  QPainter p (this);

  p.setBackground(QBrush(Qt::black));
  p.eraseRect(this->rect());

  // Count types
  QVector<unsigned int> counts;
  counts.resize(m_numFlockerTypes + 1);

  // Sort Entities by z-depth:
  QLinkedList<Entity*> sortedEntities;
  foreach (Entity *e, m_entities) {
    Flocker *f = qobject_cast<Flocker*>(e);
    Predator *p = qobject_cast<Predator*>(e);
    if (p) {
      ++counts[m_numFlockerTypes];
    }
    else if (f) {
      ++counts[f->type()];
    }
    bool inserted = false;
    for(QLinkedList<Entity*>::iterator jt = sortedEntities.begin(),
        jt_end = sortedEntities.end(); jt != jt_end; ++jt) {
      if (e->pos().z() < (*jt)->pos().z()) {
        sortedEntities.insert(jt, e);
        inserted = true;
        break;
      }
    }
    if (!inserted) {
      sortedEntities.append(e);
    }
  }

  foreach (Entity *e, sortedEntities) {
    e->draw(&p);
  }

  // Print out number of types
  unsigned int y = 10;
  for (unsigned int i = 0; i < m_numFlockerTypes + 1; ++i) {
    unsigned int count = counts[i];
    if (i < m_numFlockerTypes)
      p.setPen(this->typeToColor(i));
    else
      p.setPen(Qt::red);
    p.drawText(5, y, QString::number(count));
    y += 20;
  }

  p.setPen(Qt::white);
  p.drawText(5, y, QString("FPS: %1 (%2)")
             .arg(m_fpsSum / m_fpsCount).arg(m_currentFPS));
  y += 20;
}

void FlockWidget::initializeFlockers()
{
  this->cleanupFlockers();

  for (unsigned int i = 0; i < m_numFlockers; ++i) {
    this->addRandomFlocker();
  }
}

void FlockWidget::cleanupFlockers()
{
  foreach (Flocker *f, m_flockers) {
    m_entities.removeOne(f);
  }
  qDeleteAll(m_flockers);
  m_flockers.clear();
  m_predators.clear();
}

void FlockWidget::addFlockerFromEntity(const Entity *e)
{
  Flocker *newFlocker = new Flocker (m_entityIdHead++, e->type());
  newFlocker->pos() = e->pos();
  newFlocker->direction() = e->direction();
  newFlocker->velocity() = e->velocity();
  newFlocker->color() = e->color();

  m_flockers.push_back(newFlocker);
  m_entities.push_back(newFlocker);
}

void FlockWidget::addRandomFlocker()
{
  Flocker *f = new Flocker (m_entityIdHead, m_entityIdHead % m_numFlockerTypes);
  m_entityIdHead++;

  this->randomizeVector(&f->pos());

  f->direction().setRandom();
  f->direction().normalize();

  f->velocity() = m_initialSpeed;

  f->color() = this->typeToColor(f->type());

  m_flockers.push_back(f);
  m_entities.push_back(f);
}

void FlockWidget::removeFlocker(Flocker *f)
{
  m_flockers.removeOne(f);
  m_entities.removeOne(f);
  f->deleteLater();
}

void FlockWidget::initializePredators()
{
  this->cleanupPredators();

  for (unsigned int i = 0; i < m_numPredators; ++i) {
    this->addRandomPredator();
  }
}

void FlockWidget::cleanupPredators()
{
  foreach (Predator *p, this->m_predators) {
    m_flockers.removeOne(p);
    m_entities.removeOne(p);
  }
  qDeleteAll(this->m_predators);
  this->m_predators.clear();
}

void FlockWidget::addRandomPredator()
{
  Predator *p = new Predator (m_entityIdHead++, 0);

  this->randomizeVector(&p->pos());

  p->direction().setRandom();
  p->direction().normalize();

  p->velocity() = m_initialSpeed;

  p->color() = QColor(Qt::red);

  m_predators.push_back(p);
  m_flockers.push_back(p);
  m_entities.push_back(p);
}

void FlockWidget::removePredator(Predator *p)
{
  m_predators.removeOne(p);
  m_flockers.removeOne(p);
  m_entities.removeOne(p);
  p->deleteLater();
}

void FlockWidget::initializeTargets()
{
  this->cleanupTargets();

  for (unsigned int type = 0; type < m_numTargetTypes; ++type) {
    for (unsigned int i = 0; i < m_numTargetsPerType; ++i) {
      this->addRandomTarget(type);
    }
  }
}

void FlockWidget::cleanupTargets()
{
  for (int i = 0; i < m_targets.size(); ++i) {
    qDeleteAll(m_targets[i]);
  }

  this->m_targets.clear();
}

void FlockWidget::addRandomTarget(const unsigned int type)
{
  Target *newTarget = new Target(m_entityIdHead++, type);

  randomizeVector(&newTarget->pos());
  newTarget->color() = this->typeToColor(type);
  newTarget->velocity() = m_minSpeed;
  randomizeVector(&newTarget->direction());
  newTarget->direction().normalize();

  if (type + 1 > static_cast<unsigned int>(m_targets.size())) {
    m_targets.resize(type + 1);
  }

  m_targets[type].push_back(newTarget);
  m_entities.push_back(newTarget);
}

void FlockWidget::removeTarget(Target *t)
{
  m_targets[t->type()].removeOne(t);
  m_entities.removeOne(t);
  t->deleteLater();
}

void FlockWidget::randomizeTarget(Target *t)
{
  this->randomizeVector(&t->pos());
}

void FlockWidget::randomizeVector(Eigen::Vector3d *vec)
{
  static const double invRANDMAX = 1.0/static_cast<double>(RAND_MAX);
  vec->x() = rand() * invRANDMAX;
  vec->y() = rand() * invRANDMAX;
  vec->z() = rand() * invRANDMAX;
}

QColor FlockWidget::typeToColor(const unsigned int type)
{
  static const unsigned int numColors = 12;
  const unsigned int colorMod = (m_numFlockerTypes < numColors)
      ? m_numFlockerTypes : numColors;

  switch (type % colorMod)
  {
  case 0:
    return QColor(Qt::blue);
  case 1:
    return QColor(Qt::darkCyan);
  case 2:
    return QColor(Qt::green);
  case 3:
    return QColor(Qt::yellow);
  case 4:
    return QColor(Qt::white);
  case 5:
    return QColor(Qt::cyan);
  case 6:
    return QColor(Qt::magenta);
  case 7:
    return QColor(Qt::darkGray);
  case 8:
    return QColor(Qt::lightGray);
  case 9:
    return QColor(Qt::darkMagenta);
  case 10:
    return QColor(Qt::darkBlue);
  case 11:
    return QColor(Qt::darkGreen);
  default:
    qWarning() << "Unrecognized type:" << type;
    return QColor();
  }
}
