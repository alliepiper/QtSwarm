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
  enum EntityType {
    Invalid = 0,
    FlockerEntity,
    PredatorEntity,
    TargetEntity,
    BlastEntity,
  };

  explicit Entity(unsigned int id, unsigned int type, EntityType eType,
                  QObject *parent);
  virtual ~Entity();

  unsigned int id() const {return m_id;}
  unsigned int type() const {return m_type;}
  EntityType eType() const {return m_eType;}

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
  EntityType m_eType;
  Eigen::Vector3d m_pos;
  Eigen::Vector3d m_direction;
  double m_velocity;
  QColor m_color;
};

#endif // ENTITY_H
