#ifndef TARGET_H
#define TARGET_H

#include "entity.h"

class Target : public Entity
{
  Q_OBJECT
public:
  explicit Target(unsigned int id, unsigned int type, QObject *parent = 0);
  virtual ~Target();

  static bool visible();
  static void setVisible(bool vis);

signals:
  
public slots:
  virtual void draw(QPainter *p);
  virtual void takeStep(double t);

private:
  static bool m_visible;
};

#endif // TARGET_H
