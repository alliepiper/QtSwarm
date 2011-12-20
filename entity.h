#ifndef ENTITY_H
#define ENTITY_H

#include <Eigen/Core>

#include <QtCore/QObject>

#include <QtGui/QColor>

class QPainter;

class Entity : public QObject
{
  Q_OBJECT
public:
  explicit Entity(unsigned int it, unsigned int type, QObject *parent = 0);
  virtual ~Entity();

  unsigned int id() const {return m_id;}
  unsigned int type() const {return m_type;}
  Eigen::Vector3d & pos() {return m_pos;}
  Eigen::Vector3d & direction() {return m_direction;}
  double & velocity() {return m_velocity;}
  QColor & color() {return m_color;}

  const Eigen::Vector3d & pos() const {return m_pos;}
  const Eigen::Vector3d & direction() const {return m_direction;}
  const double & velocity() const {return m_velocity;}
  const QColor & color() const {return m_color;}

public slots:
  virtual void draw(QPainter *p) = 0;
  virtual void takeStep() = 0;

protected:
  unsigned int m_id;
  unsigned int m_type;
  Eigen::Vector3d m_pos;
  Eigen::Vector3d m_direction;
  double m_velocity;
  QColor m_color;
};

#endif // ENTITY_H
