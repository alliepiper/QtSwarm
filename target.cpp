#include "target.h"

#include <QtGui/QPainter>

Target::Target(unsigned int id, unsigned int type, QObject *parent) :
  Entity(id, type, TargetEntity, parent)
{
}

Target::~Target()
{
}

void Target::draw(QPainter *p)
{
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

void Target::takeStep()
{
  m_pos += m_velocity * m_direction;

  // Bounce at boundaries
  if (m_pos.x() < 0.0) {
    m_direction.x() =  fabs(m_direction.x());
    m_pos.x() = 0.001;
  }
  else if (m_pos.x() > 1.0) {
    m_direction.x() = -fabs(m_direction.x());
    m_pos.x() = 0.999;
  }
  if (m_pos.y() < 0.0) {
    m_direction.y() =  fabs(m_direction.y());
    m_pos.y() = 0.001;
  }
  else if (m_pos.y() > 1.0) {
    m_direction.y() = -fabs(m_direction.y());
    m_pos.y() = 0.999;
  }
  if (m_pos.z() < 0.0) {
    m_direction.z() =  fabs(m_direction.z());
    m_pos.z() = 0.001;
  }
  else if (m_pos.z() > 1.0) {
    m_direction.z() = -fabs(m_direction.z());
    m_pos.z() = 0.999;
  }
}
