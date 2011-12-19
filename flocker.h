#ifndef FLOCKER_H
#define FLOCKER_H

#include <QtCore/QObject>

#include <QtGui/QColor>

#include <Eigen/Core>

class QPainter;

class Flocker : public QObject
{
  Q_OBJECT
public:
  explicit Flocker(unsigned int id, unsigned int type, QObject *parent = 0);
  virtual ~Flocker();

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

signals:
  
public slots:
  virtual void draw(QPainter *p);
  virtual void takeStep();

protected:
  unsigned int m_id;
  unsigned int m_type;
  Eigen::Vector3d m_pos;
  Eigen::Vector3d m_direction;
  double m_velocity;
  QColor m_color;
  
};

#endif // FLOCKER_H
