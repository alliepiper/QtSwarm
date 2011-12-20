#include "target.h"

#include <QtGui/QPainter>

Target::Target(unsigned int id, unsigned int type, QObject *parent) :
  Entity(id, type, parent)
{
  m_velocity = 0.0;
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

//  p->setPen(Qt::NoPen);
    p->setPen(Qt::black);
  p->setBrush(QBrush(m_color, Qt::SolidPattern));

  p->drawEllipse(devPos, radius, radius);

  p->restore();
}

void Target::takeStep()
{
}
