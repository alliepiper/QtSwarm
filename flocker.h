#ifndef FLOCKER_H
#define FLOCKER_H

#include "entity.h"

class QPainter;

class Flocker : public Entity
{
  Q_OBJECT
public:
  explicit Flocker(unsigned int id, unsigned int type, QObject *parent = 0);
  virtual ~Flocker();
  
public slots:
  virtual void draw(QPainter *p);
  virtual void takeStep(double t);

protected:
  void drawInternal(QPainter *p, Qt::BrushStyle style);
};

#endif // FLOCKER_H
