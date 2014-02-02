#include "entity.h"

Entity::Entity(unsigned int id, unsigned int type,
               EntityType eType, QObject *parent) :
  QObject(parent),
  m_id(id),
  m_type(type),
  m_eType(eType)
{
}

Entity::~Entity()
{
}
