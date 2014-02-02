#include "predator.h"

Predator::Predator(unsigned int id, unsigned int type, QObject *parent) :
  Flocker(id, type, parent)
{
  m_eType = PredatorEntity;

  m_brushes.push_back(Qt::SolidPattern);
  m_brushes.push_back(Qt::Dense1Pattern);
  m_brushes.push_back(Qt::Dense2Pattern);
  m_brushes.push_back(Qt::Dense3Pattern);
  m_brushes.push_back(Qt::Dense4Pattern);
  m_brushes.push_back(Qt::Dense5Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense7Pattern);
  m_brushes.push_back(Qt::Dense6Pattern);
  m_brushes.push_back(Qt::Dense5Pattern);
  m_brushes.push_back(Qt::Dense4Pattern);
  m_brushes.push_back(Qt::Dense3Pattern);
  m_brushes.push_back(Qt::Dense2Pattern);
  m_brushes.push_back(Qt::Dense1Pattern);

  m_current = m_brushes.begin();
}

Predator::~Predator()
{
}

void Predator::draw(QPainter *p)
{
  if (m_current == m_brushes.end())
    m_current = m_brushes.begin();
  drawInternal(p, *m_current++);
}
