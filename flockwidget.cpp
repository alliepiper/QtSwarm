#include <Eigen/Core>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QBrush>
#include <QtGui/QKeyEvent>
#include <QtGui/QPen>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include <QtConcurrentMap>

#include <stdlib.h>
#include <time.h>

#include "flockwidget.h"
#include "flocker.h"
#include "predator.h"
#include "target.h"

namespace {
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

// Weight for each competing force
static double diffPotWeight  = 0.10; // morse potential, all type()s
static double samePotWeight  = 0.40; // morse potential, same type()
static double alignWeight    = 0.50; // Align to average neighbor heading
static double predatorWeight = 1.00; // 1/r^2 attraction/repulsion to all predators
static double targetWeight   = 0.85; // 1/r^2 attraction to all targets
static double clickWeight    = 2.00; // 1/r^2 attraction to clicked point
// newDirection = (oldDirection + (factor) * maxTurn * force).normalized()
static const double maxTurn = 0.25;
// velocity *= 1.0 + speedupFactor * direction.dot(goalForce)
static const double speedupFactor = 0.075;

// Normalize force weights:
static double invWeightSum = 1.0 / (diffPotWeight + samePotWeight + alignWeight +
                                    predatorWeight + targetWeight);

static void initWorker()
{
  static bool inited = false;
  if (inited) {
    diffPotWeight  *= invWeightSum;
    samePotWeight  *= invWeightSum;
    alignWeight    *= invWeightSum;
    predatorWeight *= invWeightSum;
    targetWeight   *= invWeightSum;
  }
}
} // end anon namespace

FlockWidget::FlockWidget(QWidget *parent) :
  QWidget(parent),
  m_timer(new QTimer (this)),
  m_entityIdHead(0),
  m_numFlockers(500),
  m_numFlockerTypes(12),
  m_numPredators(30),
  m_numPredatorTypes(2),
  m_numTargetTypes(12),
  m_numTargetsPerType(4),
  m_initialSpeed(0.0050),
  m_minSpeed(    0.0015),
  m_maxSpeed(    0.0075),
  m_lastRender(QDateTime::currentDateTime()),
  m_currentFPS(0.f),
  m_fpsSum(0.f),
  m_fpsCount(0),
  m_aborted(false),
  m_clicked(false)
{
  initWorker();
  this->initializeFlockers();
  this->initializePredators();
  this->initializeTargets();

  this->setFocusPolicy(Qt::WheelFocus);

  // Initialize RNG
  srand(time(NULL));

  connect(m_timer, SIGNAL(timeout()), this, SLOT(takeStep()));

  m_timer->start(12);
}

FlockWidget::~FlockWidget()
{
  this->cleanupFlockers();
  this->cleanupPredators();
  this->cleanupTargets();
}

struct FlockWidget::TakeStepResult
{
  Eigen::Vector3d newDirection;
  double newVelocity;
  const Target *deadTarget;
  const Flocker *deadFlocker;
};

struct FlockWidget::TakeStepFunctor
{
  TakeStepFunctor(FlockWidget &w) : widget(w) {}
  FlockWidget &widget;

  typedef TakeStepResult result_type;

  FlockWidget::TakeStepResult operator()(const Flocker *f)
  {
    return widget.takeStepWorker(f);
  }
};

bool isNan(double d)
{
  return d != d;
}

FlockWidget::TakeStepResult FlockWidget::takeStepWorker(const Flocker *f_i) const
{
  const bool pred_i = f_i->eType() == Entity::PredatorEntity;

  TakeStepResult result;
  result.deadTarget = NULL;
  result.deadFlocker = NULL;

  Eigen::Vector3d samePotForce(0., 0., 0.);
  Eigen::Vector3d diffPotForce(0., 0., 0.);
  Eigen::Vector3d alignForce(0., 0., 0.);
  Eigen::Vector3d predatorForce(0., 0., 0.);
  Eigen::Vector3d targetForce(0., 0., 0.);
  Eigen::Vector3d r(0., 0., 0.);

  if (!m_clicked) {
    // Calculate a distance-weighted average vector towards the relevant
    // targets
    if (!pred_i) {
      foreach (const Target *t, m_targets[f_i->type()]) {
        r = t->pos() - f_i->pos();
        const double rNorm = r.norm();
        if (rNorm < 0.025)
          result.deadTarget = t;
        else
          targetForce += (1.0/(rNorm*rNorm*rNorm*rNorm)) * r;
      }
    }
  }
  else {
    // Ignore targets and pull towards clicked point.
    r = m_clickPoint - f_i->pos();
    const double rNorm = r.norm();
    if (rNorm > 0.01)
      targetForce = (1.0/(rNorm*rNorm*rNorm)) * r;
  }

  // Average together V(|r_ij|) * r_ij
  foreach (const Flocker *f_j, m_flockers) {
    if (f_i == f_j) continue;
    r = f_j->pos() - f_i->pos();

    // General cutoff
    const double cutoff = pred_i ? 0.6 : 0.3;
    if (r.x() > cutoff || r.y() > cutoff || r.z() > cutoff)
      continue;

    const double rNorm = r.norm();
    const double rInvNorm = rNorm > 0.01 ? 1.0 / rNorm : 1.0;

    const bool pred_j = f_j->eType() == Entity::PredatorEntity;

    // Neither are predators, use morse potential
    bool bothArePredators = pred_i && pred_j;
    bool neitherArePredators = !pred_i && !pred_j;
    bool typesMatch = f_i->type() == f_j->type();

    if (neitherArePredators || (bothArePredators && typesMatch)) {
      double V = std::numeric_limits<double>::max();

      // Apply cutoff for morse interaction
      if (rNorm < 0.20) {
        V = V_morse_ND(rNorm);
        diffPotForce += (V*rInvNorm*rInvNorm) * r;
      }

      // Alignment -- steer towards the heading of nearby flockers
      if (typesMatch &&
          rNorm < 0.30 && rNorm > 0.001) {
        if (V == std::numeric_limits<double>::max()) {
          V = V_morse_ND(rNorm);
        }
        samePotForce += (V *rInvNorm*rInvNorm) * r;
        alignForce += rInvNorm * f_j->direction();
      }
    }
    else if (bothArePredators && !typesMatch) {
      r = f_i->pos() - f_j->pos();
      const double rNorm = r.norm();
      if (rNorm < 0.5) {
        const double rInvNorm = rNorm > 0.01 ? 1.0 / rNorm : 1.0;
        //              2    normalize     1/r2     vector
        predatorForce = 2. * rInvNorm * (rInvNorm * rInvNorm) * r;
      }
    }
    // One is a predator, one is not. Evade / Pursue
    else {
      const Flocker *p;
      const Flocker *f;
      if (pred_i) {
        p = f_i;
        f = f_j;
      }
      else {
        p = f_j;
        f = f_i;
      }

      // Calculate distance between them
      r = f->pos() - p->pos();
      const double rNorm = r.norm();
      const double rInvNorm = rNorm > 0.01 ? 1.0 / rNorm : 1.0;
      if (pred_j) {
        // The flocker is being updated
        if (rNorm < 0.3) {
          // Did the predator catch the flocker?
          if (rNorm < 0.025) {
            result.deadFlocker = f;
          }
          else {
            //               2    normalize          1/r^3                  vector
            predatorForce += 2. * rInvNorm * (rInvNorm) * r;
          }
        }
      }
      // The predator is being updated
      else {
        // Cutoff distance for predator
        if (rNorm < 0.15) {
          //               2    normalize          1/r*2         vector
          predatorForce += 2. * rInvNorm * (rInvNorm * rInvNorm) * r;
        }
        else {
          //               normalize     1/r     vector
          predatorForce += rInvNorm * (rInvNorm) * r;
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

  // target weight changes when clicked:
  const double rTargetWeight = m_clicked ? (pred_i ? -clickWeight
                                                   : clickWeight)
                                         : targetWeight;

  // Scale the force so that it will turn faster when "direction" is not
  // aligned well with force:
  Eigen::Vector3d force (samePotWeight  * samePotForce  +
                         diffPotWeight  * diffPotForce  +
                         alignWeight    * alignForce    +
                         predatorWeight * predatorForce +
                         rTargetWeight  * targetForce   );
  if (!force.isZero(0.1))
    force.normalize();

  // cap dot factor, ensures that *some* finite turning will occur
  double directionDotForce = f_i->direction().dot(force);
  if (directionDotForce > 0.85) {
    directionDotForce = 0.85;
  }
  const double scale ( (1 - 0.5 * (directionDotForce + 1)) *
                       maxTurn);

  // Accelerate towards goal
  const double goalForce = f_i->direction().dot(0.15 * predatorForce +
                                                0.25 * targetForce +
                                                0.6 * samePotForce);
  result.newVelocity = f_i->velocity() * (1.0 + speedupFactor * goalForce);
  if (result.newVelocity < m_minSpeed)
    result.newVelocity = m_minSpeed;
  else if (result.newVelocity > m_maxSpeed)
    result.newVelocity = m_maxSpeed;

  // Keep new direction
  result.newDirection = (f_i->direction() + scale * force).normalized();

  return result;
}

namespace {
struct EntityStepFunctor
{
  void operator()(Entity *e) const { e->takeStep(); }
};
}

void FlockWidget::takeStep()
{
  if (m_aborted)
    qApp->exit();

  QDateTime now = QDateTime::currentDateTime();
  qint64 elapsed_ms = m_lastRender.msecsTo(now);
  float elapsed_s = elapsed_ms / 1000.f;
  m_lastRender = now;
  m_currentFPS = 1.f / elapsed_s;
  m_fpsSum += m_currentFPS;
  ++m_fpsCount;

  // Loop through each flocker and predator:
  QFuture<FlockWidget::TakeStepResult> future =
      QtConcurrent::mapped(m_flockers, TakeStepFunctor(*this));

  // Render while waiting on future...
  this->update();
  qApp->processEvents();

  QVector<Flocker*> deadFlockers;
  QVector<Target*> deadTargets;

  future.waitForFinished();
  QLinkedList<Flocker*>::iterator current = m_flockers.begin();
  foreach (const TakeStepResult &result, future.results()) {
    (*current)->direction() = result.newDirection;
    (*current)->velocity() = result.newVelocity;
    if (result.deadFlocker)
      deadFlockers.push_back(const_cast<Flocker*>(result.deadFlocker));
    if (result.deadTarget)
      deadTargets.push_back(const_cast<Target*>(result.deadTarget));
    ++current;
  }

  foreach (Flocker *f, deadFlockers)
    this->removeFlocker(f);

  foreach (Target *t, deadTargets) {
    this->addFlockerFromEntity(t);
    this->randomizeTarget(t);
  }

  QFuture<void> stepFuture = QtConcurrent::map(m_entities, EntityStepFunctor());
  qApp->processEvents();
  stepFuture.waitForFinished();
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

  // FPS
  unsigned int y = 10;
  p.setPen(Qt::white);
  p.drawText(5, y, QString("FPS: %1 (%2)")
             .arg(m_fpsSum / m_fpsCount).arg(m_currentFPS));
  y += 20;

  // Print out number of types
  for (unsigned int i = 0; i < m_numFlockerTypes + 1; ++i) {
    unsigned int count = counts[i];
    if (i < m_numFlockerTypes)
      p.setPen(this->typeToColor(i));
    else
      p.setPen(Qt::red);
    p.drawText(5, y, QString::number(count));
    y += 20;
  }

}

void FlockWidget::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Q) {
    m_aborted = true;
    return;
  }

  QWidget::keyPressEvent(e);
}

void FlockWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (m_clicked)
    setClickPoint(e->posF());
}

void FlockWidget::mousePressEvent(QMouseEvent *e)
{
  m_clicked = true;
  setClickPoint(e->posF());
}

void FlockWidget::mouseReleaseEvent(QMouseEvent *)
{
  m_clicked = false;
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

  unsigned int type = 0;
  for (unsigned int i = 0; i < m_numPredators; ++i) {
    this->addRandomPredator(type);
    if (++type == m_numPredatorTypes)
      type = 0;
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

void FlockWidget::addRandomPredator(const unsigned int type)
{
  Predator *p = new Predator (m_entityIdHead++, type);

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

void FlockWidget::setClickPoint(const QPointF &point)
{
  const QRect rect = this->rect();
  double width = rect.width();
  double height = rect.height();
  m_clickPoint(0) = point.x() / width;
  m_clickPoint(1) = point.y() / height;
  m_clickPoint(2) = rand() / static_cast<double>(RAND_MAX);
}
