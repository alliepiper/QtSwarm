#include "blast.h"

#include <QtGui/QPainter>

namespace {
const unsigned int maxRepeat = 3;
const unsigned int lifetime  = 50;
}

Blast::Blast(unsigned int id, unsigned int type, QObject *parent) :
  Entity(id, type, BlastEntity, parent),
  m_done(false),
  m_time(0),
  m_repeat(0)
{
}

Blast::~Blast()
{
}

void Blast::draw(QPainter *p)
{
  p->save();

  // Device coordinates:
  const double width = p->device()->width();
  const double height = p->device()->height();
  QPointF devPos (m_pos.x() * width, m_pos.y() * height);

  double radius = (100 * m_time / lifetime) * (0.001 * m_pos.z()) + 0.035;
  radius *= 0.5 * (width + height);

  QColor color(m_color);
  color.setAlpha(128 - (126 * m_time) / (lifetime + 1));
  p->setPen(color);
  p->setBrush(Qt::NoBrush);

  p->drawEllipse(devPos, radius, radius);

  p->restore();
}

void Blast::takeStep()
{
  if (!m_done) {
    if (m_repeat++ > maxRepeat) {
      m_repeat = 0;
      if (m_time++ > lifetime) {
        m_done = true;
      }
    }
  }
}
