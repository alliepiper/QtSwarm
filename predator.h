#ifndef PREDATOR_H
#define PREDATOR_H

#include "flocker.h"

#include <QtCore/QVector>

class Predator : public Flocker
{
  Q_OBJECT
public:
  explicit Predator(unsigned int id, unsigned int type, QObject *parent = 0);
  virtual ~Predator();

public slots:
  void draw(QPainter *p);

private:
  typedef QVector<Qt::BrushStyle> BrushVector;
   BrushVector m_brushes;
   BrushVector::const_iterator m_current;
   unsigned int m_repeat;
};

#endif // PREDATOR_H
