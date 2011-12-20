#include "entity.h"

Entity::Entity(unsigned int id, unsigned int type, QObject *parent) :
  QObject(parent),
  m_id(id),
  m_type(type)
{
}

Entity::~Entity()
{
}
