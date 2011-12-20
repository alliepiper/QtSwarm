#include "flocker.h"

#include <QtCore/QPointF>

#include <QtGui/QPainter>
#include <QtGui/QPaintDevice>
#include <QtGui/QRadialGradient>

const double MINRADIUS = 0.010;
const double MAXRADIUS = 0.020;

Flocker::Flocker(unsigned int id, unsigned int type, QObject *parent) :
  Entity(id, type, parent)
{
}

Flocker::~Flocker()
{
}

void Flocker::draw(QPainter *p)
{
  const double width = p->device()->width();
  const double height = p->device()->height();

  // x/y position in device coordinates:
  double devPos[2];
  devPos[0] = m_pos.x() * width;
  devPos[1] = m_pos.y() * height;

  const double depth = m_pos.z();

  double radius = depth * MAXRADIUS;
  if (radius < MINRADIUS) {
    radius = MINRADIUS;
  }

  radius *= 0.5 * (width + height);

  double devDir[2];
  devDir[0] = m_direction.x() * radius;
  devDir[1] = m_direction.y() * radius;

  p->setPen(Qt::black);
  QBrush brush (m_color, Qt::SolidPattern);
  p->setBrush(brush);

  // Draw as circle
  //  const double diameter = radius + radius;
  //  p->drawEllipse(devPos[0], devPos[1], diameter, diameter);

  // Three points defining the triangle
  QPointF triangle [3];
  triangle[0].setX(devPos[0] + devDir[0]);
  triangle[0].setY(devPos[1] + devDir[1]);

  triangle[1].setX(devPos[0] - devDir[0] + 0.5 * devDir[1]);
  triangle[1].setY(devPos[1] - devDir[1] + 0.5 * devDir[0]);

  triangle[2].setX(devPos[0] - devDir[0] - 0.5 * devDir[1]);
  triangle[2].setY(devPos[1] - devDir[1] - 0.5 * devDir[0]);
  p->drawPolygon(triangle, 3);
}

void Flocker::takeStep()
{
  m_pos += m_velocity * m_direction;

  // Bounce at boundaries, slow down
  const double bounceSlowdownFactor = 0.50;
  if (m_pos.x() < 0.0) {
    m_direction.x() =  fabs(m_direction.x());
    m_pos.x() = 0.001;
    m_velocity *= bounceSlowdownFactor;
  }
  else if (m_pos.x() > 1.0) {
    m_direction.x() = -fabs(m_direction.x());
    m_pos.x() = 0.999;
    m_velocity *= bounceSlowdownFactor;
  }
  if (m_pos.y() < 0.0) {
    m_direction.y() =  fabs(m_direction.y());
    m_pos.y() = 0.001;
    m_velocity *= bounceSlowdownFactor;
  }
  else if (m_pos.y() > 1.0) {
    m_direction.y() = -fabs(m_direction.y());
    m_pos.y() = 0.999;
    m_velocity *= bounceSlowdownFactor;
  }
  if (m_pos.z() < 0.0) {
    m_direction.z() =  fabs(m_direction.z());
    m_pos.z() = 0.001;
    m_velocity *= bounceSlowdownFactor;
  }
  else if (m_pos.z() > 1.0) {
    m_direction.z() = -fabs(m_direction.z());
    m_pos.z() = 0.999;
    m_velocity *= bounceSlowdownFactor;
  }
}
