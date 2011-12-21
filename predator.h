#ifndef PREDATOR_H
#define PREDATOR_H

#include "flocker.h"

class Predator : public Flocker
{
  Q_OBJECT
public:
  explicit Predator(unsigned int id, unsigned int type, QObject *parent = 0);
  virtual ~Predator();
};

#endif // PREDATOR_H
