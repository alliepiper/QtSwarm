#include "target.h"

#include <QtGui/QPainter>

bool Target::m_visible = false;

Target::Target(unsigned int id, unsigned int type, QObject *parent) :
  Entity(id, type, TargetEntity, parent)
{
}

Target::~Target()
{
}

void Target::draw(QPainter *p)
{
  if (!Target::m_visible) {
    return;
  }

  p->save();

  // Device coordinates:
  const double width = p->device()->width();
  const double height = p->device()->height();
  QPointF devPos (m_pos.x() * width, m_pos.y() * height);

  double radius = 0.005 * m_pos.z() + 0.005;
  radius *= 0.5 * (width + height);

  p->setPen(Qt::black);
  p->setBrush(QBrush(m_color, Qt::SolidPattern));

  p->drawEllipse(devPos, radius, radius);

  p->restore();
}

void Target::takeStep(double t)
{
  m_pos += m_velocity * m_direction * t;

  // Use a reduced boundary for these -- keeps the flockers from bouncing off
  // of the walls as much
  const double validFraction = 0.90;

  const double minVal = (1.0 - validFraction) / 2.0;
  const double maxVal = 1.0 - minVal;

  // Bounce at boundaries
  if (m_pos.x() < minVal) {
    m_direction.x() =  fabs(m_direction.x());
    m_pos.x() = minVal;
  }
  else if (m_pos.x() > maxVal) {
    m_direction.x() = -fabs(m_direction.x());
    m_pos.x() = maxVal;
  }
  if (m_pos.y() < minVal) {
    m_direction.y() =  fabs(m_direction.y());
    m_pos.y() = minVal;
  }
  else if (m_pos.y() > maxVal) {
    m_direction.y() = -fabs(m_direction.y());
    m_pos.y() = maxVal;
  }
  if (m_pos.z() < minVal) {
    m_direction.z() =  fabs(m_direction.z());
    m_pos.z() = minVal;
  }
  else if (m_pos.z() > maxVal) {
    m_direction.z() = -fabs(m_direction.z());
    m_pos.z() = maxVal;
  }
}

bool Target::visible()
{
  return Target::m_visible;
}

void Target::setVisible(bool visible)
{
  Target::m_visible = visible;
}
