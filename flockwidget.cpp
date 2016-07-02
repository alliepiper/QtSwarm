#include "flockwidget.h"

// TODO clean this up.
#include <Eigen/Core>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtWidgets/QApplication>

#include <QtGui/QBrush>
#include <QtGui/QKeyEvent>
#include <QtGui/QPen>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include <QtConcurrent/QtConcurrentMap>

#include "blast.h"
#include "flockengine.h"
#include "flocker.h"
#include "predator.h"
#include "target.h"

FlockWidget::FlockWidget(QWidget *parent) :
  QWidget(parent),
  m_timer(new QTimer (this)),
  m_engine(new FlockEngine(this)),
  m_lastRender(QDateTime::currentDateTime()),
  m_currentFPS(0.f),
  m_fpsSum(0.f),
  m_fpsCount(0),
  m_aborted(false),
  m_showOverlay(true)
{
  this->setFocusPolicy(Qt::WheelFocus);

  connect(m_timer, SIGNAL(timeout()), this, SLOT(takeStep()));

  m_timer->start(12);
}

FlockWidget::~FlockWidget()
{
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

  m_engine->computeNextStep();

  // Render while waiting on future...
  this->update();
  qApp->processEvents();

  m_engine->commitNextStep();
}

void FlockWidget::paintEvent(QPaintEvent *)
{
  QPainter p(this);

  p.setBackground(QBrush(Qt::black));
  p.eraseRect(this->rect());

  // Count types
  QVector<unsigned int> counts;
  size_t countsSize = m_engine->numFlockerTypes() + 1;
  counts.resize(countsSize);

  // Sort Entities by z-depth:
  QLinkedList<Entity*> sortedEntities;
  foreach (Entity *e, m_engine->entities()) {
    Flocker *f = qobject_cast<Flocker*>(e);
    Predator *pred = qobject_cast<Predator*>(e);
    if (pred) {
      ++counts[m_engine->numFlockerTypes()];
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

  if (m_showOverlay) {
    // FPS
    int skip = p.fontMetrics().height() * 1.2;
    int y = 10 + skip;

    p.setPen(Qt::white);
    p.drawText(5, y, QString("FPS: %1 (current = %2)")
               .arg(m_fpsSum / m_fpsCount)
               .arg(m_currentFPS));
    y += skip;

    p.drawText(5, y, QString("Step Size: %1x")
               .arg(m_engine->stepSize(), 0, 'f', 2));
    y += skip;

    p.drawText(5, y, QString("Entities: %1 (%2 flockers, %3 blasts, "
                             "%4 targets, %5 predators)")
               .arg(m_engine->entities().size())
               .arg(m_engine->flockers().size())
               .arg(m_engine->blasts().size())
               .arg(m_engine->numFlockerTypes() *
                    m_engine->numTargetsPerFlockerType())
               .arg(m_engine->predators().size()));
    y += skip;

    // Print out number of types
    for (unsigned int i = 0; i < countsSize; ++i) {
      unsigned int count = counts[i];
      if (i < m_engine->numFlockerTypes())
        p.setPen(m_engine->typeToColor(i));
      else
        p.setPen(Qt::red);
      p.drawText(5, y, QString::number(count));
      y += skip;
    }
  }
}

void FlockWidget::keyPressEvent(QKeyEvent *e)
{
  switch (e->key())
  {
  case Qt::Key_Q:
    m_aborted = true;
    break;

  case Qt::Key_B:
    m_engine->setCreateBlasts(!m_engine->createBlasts());
    break;

  case Qt::Key_O:
    m_showOverlay = !m_showOverlay;
    break;

  case Qt::Key_F:
    // Doesn't seem to work...maybe only works before window is shown.
    if (this->isFullScreen()) {
      this->showNormal();
    }
    else {
      this->showFullScreen();
    }
    break;

  case Qt::Key_Up:
    m_engine->setStepSize(m_engine->stepSize() * 1.25);
    break;

  case Qt::Key_Down:
    m_engine->setStepSize(m_engine->stepSize() * 0.8);
    break;
  }

  QWidget::keyPressEvent(e);
}

void FlockWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (m_engine->useForceTarget()) {
    setClickPoint(e->localPos());
  }
}

void FlockWidget::mousePressEvent(QMouseEvent *e)
{
  m_engine->setUseForceTarget(true);
  this->setClickPoint(e->localPos());
}

void FlockWidget::mouseReleaseEvent(QMouseEvent *)
{
  m_engine->setUseForceTarget(false);
}

void FlockWidget::enableBlasts()
{
  m_engine->setCreateBlasts(true);
}

void FlockWidget::disableBlasts()
{
  m_engine->setCreateBlasts(false);
}

void FlockWidget::setClickPoint(const QPointF &loc)
{
  const QRect rect = this->rect();
  double width = rect.width();
  double height = rect.height();

  Eigen::Vector3d point;
  point.x() = loc.x() / width;
  point.y() = loc.y() / height;
  point.z() = rand() / static_cast<double>(RAND_MAX);
  m_engine->setForceTarget(point);
}
