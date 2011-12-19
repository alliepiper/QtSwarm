#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtGui/QBrush>
#include <QtGui/QPen>
#include <QtGui/QPainter>

#include <Eigen/Array>

#include "flockwidget.h"
#include "flocker.h"

FlockWidget::FlockWidget(QWidget *parent) :
  QWidget(parent),
  m_timer(new QTimer (this)),
  m_numFlockers(1000),
  m_flockerIdHead(0),
  m_numTypes(6),
  m_initialSpeed(0.0020),
  m_minSpeed(    0.0035),
  m_maxSpeed(    0.0075)
{
  this->initializeFlockers();

  connect(m_timer, SIGNAL(timeout()), this, SLOT(takeStep()));

  m_timer->start(20);
}

FlockWidget::~FlockWidget()
{
  this->cleanupFlockers();
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

  // Loop through each flocker
  foreach (Flocker *f_i, this->m_flockers) {
    // Calculate a force vector
    Eigen::Vector3d samePotForce (0.0, 0.0, 0.0);
    Eigen::Vector3d diffPotForce (0.0, 0.0, 0.0);
    Eigen::Vector3d alignForce (0.0, 0.0, 0.0);

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

    // Scale the force so that it will turn faster when "direction" is not
    // aligned well with force:
    const double fracSamePot = 0.50;
    const double fracDiffPot = 0.10;
    const double fracAlign   = 1.00 - (fracSamePot + fracDiffPot);
    const double maxScale = 0.05;
    //
    Eigen::Vector3d force (fracSamePot * samePotForce +
                           fracDiffPot * diffPotForce +
                           fracAlign   * alignForce);
    force.normalize();
    const double directionDotForce = f_i->direction().dot(force);
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

  // These floaters need to be removed:
  QVector<Flocker*> deadFlockers;
  deadFlockers.reserve(this->m_numFlockers);

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

  this->update();
}

void FlockWidget::paintEvent(QPaintEvent *)
{
  QPainter p (this);

  // Sort Flockers by z-depth:
  QLinkedList<Flocker*> sortedFlockers;

  for(QLinkedList<Flocker*>::const_iterator i = m_flockers.constBegin(),
      i_end = m_flockers.constEnd(); i != i_end; ++i) {
    bool inserted = false;
    for(QLinkedList<Flocker*>::iterator j = sortedFlockers.begin(),
        j_end = sortedFlockers.end(); j != j_end; ++j) {
      if ((*i)->pos().z() < (*j)->pos().z()) {
        sortedFlockers.insert(j, *i);
        inserted = true;
        break;
      }
    }
    if (!inserted) {
      sortedFlockers.append(*i);
    }
  }

  foreach (Flocker *f, sortedFlockers) {
    f->draw(&p);
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

void FlockWidget::addRandomFlocker()
{
  Flocker *f = new Flocker (m_flockerIdHead, m_flockerIdHead % m_numTypes);
  m_flockerIdHead++;

  f->pos().setRandom();
  f->pos().normalize();

  if (f->pos().x() < 0.0) f->pos().x() = fabs(f->pos().x());
  if (f->pos().y() < 0.0) f->pos().y() = fabs(f->pos().y());
  if (f->pos().z() < 0.0) f->pos().z() = fabs(f->pos().z());

  f->direction().setRandom();
  f->direction().normalize();

  f->velocity() = m_initialSpeed;

  unsigned int colorMod = (m_numTypes < 6) ? m_numTypes : 6;

  switch (f->id() % colorMod)
  {
  case 0:
    f->color() = QColor(Qt::blue);
    break;
  case 1:
    f->color() = QColor(Qt::red);
    break;
  case 2:
    f->color() = QColor(Qt::green);
    break;
  case 3:
    f->color() = QColor(Qt::yellow);
    break;
  case 4:
    f->color() = QColor(Qt::black);
    break;
  case 5:
    f->color() = QColor(Qt::cyan);
    break;
  }

  this->m_flockers.push_back(f);
}
