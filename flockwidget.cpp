#include <Eigen/Array>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtGui/QPainter>

#include <stdlib.h>
#include <time.h>

#include "flockwidget.h"
#include "flocker.h"
#include "target.h"

FlockWidget::FlockWidget(QWidget *parent) :
  QWidget(parent),
  m_timer(new QTimer (this)),
  m_numFlockers(1000),
  m_flockerIdHead(0),
  m_numTypes(12),
  m_numTargetTypes(12),
  m_numTargetsPerType(3),
  m_targetIdHead(0),
  m_initialSpeed(0.0020),
  m_minSpeed(    0.0035),
  m_maxSpeed(    0.0075)
{
  this->initializeFlockers();
  this->initializeTargets();

  // Initialize RNG
  srand(time(NULL));

  connect(m_timer, SIGNAL(timeout()), this, SLOT(takeStep()));

  m_timer->start(20);
}

FlockWidget::~FlockWidget()
{
  this->cleanupFlockers();
  this->cleanupTargets();
}

inline double V_morse(const double r) {
  const double depth = 1.00;
  const double rad   = 0.075;
  const double alpha = 2.00;

  const double tmpTerm = ( 1 - exp( -alpha * (r - rad) ) );
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
  QVector<Eigen::Vector3d> newDirections;
  newDirections.reserve(this->m_flockers.size());

  // These flockers need to be removed:
  QVector<Flocker*> deadFlockers;
  deadFlockers.reserve(this->m_numFlockers);

  // These targets have been eaten:
  QVector<Target*> deadTargets;
  deadTargets.reserve(m_targets.size());

  // Loop through each flocker
  foreach (Flocker *f_i, this->m_flockers) {
    // Calculate a force vector
    Eigen::Vector3d samePotForce (0.0, 0.0, 0.0);
    Eigen::Vector3d diffPotForce (0.0, 0.0, 0.0);
    Eigen::Vector3d alignForce   (0.0, 0.0, 0.0);
    Eigen::Vector3d targetForce  (0.0, 0.0, 0.0);

    // Calculate a distance-weighted average vector towards the relevant
    // targets
    foreach (Target *t, this->m_targets[f_i->type()]) {
      const Eigen::Vector3d r = t->pos() - f_i->pos();
      const double rNorm = r.norm();
      targetForce += (1.0/(rNorm*rNorm*rNorm)) * r;
      if (rNorm < 0.05) {
        deadTargets.push_back(t);
      }
    }

    // Average together V(|r_ij|) * r_ij
    foreach (Flocker *f_j, this->m_flockers) {
      if (f_i == f_j) continue;
      Eigen::Vector3d r = f_j->pos() - f_i->pos();
      const double rNorm = r.norm();

      // General cutoff
      if (rNorm > 0.5) {
        continue;
      }

      double V = std::numeric_limits<double>::max();

      // Apply cutoff for morse interaction
      if (rNorm < 0.10) {
        V = V_morse_ND(rNorm);
        diffPotForce += (V /(rNorm*rNorm)) * r;
      }

      // Alignment -- steer towards the heading of nearby flockers
      if (f_i->type() == f_j->type() &&
          rNorm < 0.20 && rNorm > 0.001) {
        if (V == std::numeric_limits<double>::max()) {
          V = V_morse_ND(rNorm);
        }
        samePotForce += (V /(rNorm*rNorm)) * r;
        alignForce += (1.0/rNorm) * f_j->direction();
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
    if (!targetForce.isZero(0.1)) {
      targetForce.normalize();
    }

    // Scale the force so that it will turn faster when "direction" is not
    // aligned well with force:
    const double fracSamePot = 0.15;
    const double fracDiffPot = 0.10;
    const double fracTarget  = 0.60;
    const double fracAlign   = 1.00 - (fracSamePot + fracDiffPot + fracTarget);
    const double maxScale = 0.1;
    //
    Eigen::Vector3d force (fracSamePot * samePotForce +
                           fracDiffPot * diffPotForce +
                           fracAlign   * alignForce +
                           fracTarget  * targetForce);
    force.normalize();
    double directionDotForce = f_i->direction().dot(force);
    if (directionDotForce < 1e-4) {
      directionDotForce = 1e-4;
    }
    const double scale ( (1 - 0.5 * (directionDotForce + 1)) *
                         maxScale);

    // Accelerate towards goal
    const double speedupFactor = 0.075;
    //
    const double directionDotPotForce = f_i->direction().dot(samePotForce);
    f_i->velocity() *= 1.0 + speedupFactor * directionDotPotForce;
    if (f_i->velocity() < m_minSpeed) f_i->velocity() = m_minSpeed;
    else if (f_i->velocity() > m_maxSpeed) f_i->velocity() = m_maxSpeed;

    newDirections.append((f_i->direction() + scale * force).normalized());
  }

  // Take step:
  int ind = 0;
  foreach (Flocker *f, this->m_flockers) {
    f->direction() = newDirections[ind++];
    f->takeStep();

//    // Check that each flocker is still one the screen. If not remove it
//    if ( f->pos().x() < 0 || f->pos().x() > 1.0 ||
//         f->pos().y() < 0 || f->pos().y() > 1.0 ){
//      deadFlockers.append(f);
//    }

    // Bounce at boundaries, slow down
    const double bounceSlowdownFactor = 0.50;
    if (f->pos().x() < 0.0) {
      f->direction().x() =  fabs(f->direction().x());
      f->pos().x() = 0.001;
      f->velocity() *= bounceSlowdownFactor;
    }
    else if (f->pos().x() > 1.0) {
      f->direction().x() = -fabs(f->direction().x());
      f->pos().x() = 0.999;
      f->velocity() *= bounceSlowdownFactor;
    }
    if (f->pos().y() < 0.0) {
      f->direction().y() =  fabs(f->direction().y());
      f->pos().y() = 0.001;
      f->velocity() *= bounceSlowdownFactor;
    }
    else if (f->pos().y() > 1.0) {
      f->direction().y() = -fabs(f->direction().y());
      f->pos().y() = 0.999;
      f->velocity() *= bounceSlowdownFactor;
    }
    if (f->pos().z() < 0.0) {
      f->direction().z() =  fabs(f->direction().z());
      f->pos().z() = 0.001;
      f->velocity() *= bounceSlowdownFactor;
    }
    else if (f->pos().z() > 1.0) {
      f->direction().z() = -fabs(f->direction().z());
      f->pos().z() = 0.999;
      f->velocity() *= bounceSlowdownFactor;
    }
  }

  // Replace dead floaters
  foreach (Flocker *f, deadFlockers) {
    f->deleteLater();
    this->m_flockers.removeOne(f);
    this->addRandomFlocker();
  }

  // Randomize dead targets
  foreach (Target *t, deadTargets) {
    this->randomizeTarget(t);
  }

  this->update();
}

void FlockWidget::paintEvent(QPaintEvent *)
{
  QPainter p (this);

  p.setBackground(QBrush(Qt::black));
  p.eraseRect(this->rect());

  // Sort Entities by z-depth:
  QLinkedList<Entity*> sortedEntities;

  // Sort flockers
  for(QLinkedList<Flocker*>::const_iterator i = m_flockers.constBegin(),
      i_end = m_flockers.constEnd(); i != i_end; ++i) {
    bool inserted = false;
    for(QLinkedList<Entity*>::iterator j = sortedEntities.begin(),
        j_end = sortedEntities.end(); j != j_end; ++j) {
      if ((*i)->pos().z() < (*j)->pos().z()) {
        sortedEntities.insert(j, *i);
        inserted = true;
        break;
      }
    }
    if (!inserted) {
      sortedEntities.append(*i);
    }
  }

  // Sort targets
  for (int ind = 0; ind < m_targets.size(); ++ind) {
    for(QLinkedList<Target*>::const_iterator it = m_targets[ind].constBegin(),
        it_end = m_targets[ind].constEnd(); it != it_end; ++it) {
      bool inserted = false;
      for(QLinkedList<Entity*>::iterator jt = sortedEntities.begin(),
          jt_end = sortedEntities.end(); jt != jt_end; ++jt) {
        if ((*it)->pos().z() < (*jt)->pos().z()) {
          sortedEntities.insert(jt, *it);
          inserted = true;
          break;
        }
      }
      if (!inserted) {
        sortedEntities.append(*it);
      }
    }
  }

  foreach (Entity *e, sortedEntities) {
    e->draw(&p);
  }

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
  qDeleteAll(this->m_flockers);
  this->m_flockers.clear();
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

void FlockWidget::addRandomFlocker()
{
  Flocker *f = new Flocker (m_flockerIdHead, m_flockerIdHead % m_numTypes);
  m_flockerIdHead++;

  this->randomizeVector(&f->pos());

  f->direction().setRandom();
  f->direction().normalize();

  f->velocity() = m_initialSpeed;

  f->color() = this->typeToColor(f->type());

  this->m_flockers.push_back(f);
}

void FlockWidget::addRandomTarget(const unsigned int type)
{
  Target *newTarget = new Target(m_targetIdHead++, type);

  randomizeVector(&newTarget->pos());
  newTarget->color() = this->typeToColor(type);

  if (type + 1 > static_cast<unsigned int>(m_targets.size())) {
    m_targets.resize(type + 1);
  }

  m_targets[type].push_back(newTarget);
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
  const unsigned int colorMod = (m_numTypes < numColors) ? m_numTypes
                                                         : numColors;

  switch (type % colorMod)
  {
  case 0:
    return QColor(Qt::blue);
  case 1:
    return QColor(Qt::red);
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
