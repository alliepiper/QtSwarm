#include "predator.h"

#include <algorithm>

Predator::Predator(unsigned int id, unsigned int type, QObject *parent) :
  Flocker(id, type, parent),
  m_repeat(0)
{
  m_eType = PredatorEntity;

  m_brushes.push_back(Qt::SolidPattern);
  m_brushes.push_back(Qt::SolidPattern);
  m_brushes.push_back(Qt::SolidPattern);
  m_brushes.push_back(Qt::SolidPattern);
  m_brushes.push_back(Qt::SolidPattern);
  m_brushes.push_back(Qt::SolidPattern);
  m_brushes.push_back(Qt::Dense1Pattern);
  m_brushes.push_back(Qt::Dense2Pattern);
  m_brushes.push_back(Qt::Dense3Pattern);
  m_brushes.push_back(Qt::Dense4Pattern);
  m_brushes.push_back(Qt::Dense5Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense5Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense5Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::NoBrush);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense5Pattern);
  m_brushes.push_back(Qt::Dense4Pattern);
  m_brushes.push_back(Qt::Dense3Pattern);
  m_brushes.push_back(Qt::Dense2Pattern);
  m_brushes.push_back(Qt::Dense1Pattern);

  std::rotate(m_brushes.begin(), m_brushes.begin() + (id % m_brushes.size()),
              m_brushes.end());

  m_current = m_brushes.begin();
}

Predator::~Predator()
{
}

void Predator::draw(QPainter *p)
{
  if (m_current == m_brushes.end())
    m_current = m_brushes.begin();
  drawInternal(p, *m_current);

  // Change brushes every x frames:
  if (++m_repeat == 3) {
    ++m_current;
    m_repeat = 0;
  }
}
