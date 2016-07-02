#ifndef BLAST_H
#define BLAST_H

#include "entity.h"

class Blast : public Entity
{
  Q_OBJECT
public:
  explicit Blast(unsigned int id, unsigned int type, QObject *parent = 0);
  ~Blast();

  bool done() const { return m_done; }

public slots:
  void draw(QPainter *p);
  void takeStep(double t);

private:
  bool m_done;
  unsigned int m_time;
  unsigned int m_repeat;
};

#endif // BLAST_H
