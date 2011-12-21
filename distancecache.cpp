#include "distancecache.h"

#include <QtCore/QHash>

#include <math.h>

struct CacheEntry
{
  double vector[3];
  double norm;
  double invNorm;
};

class DistanceCachePrivate
{
public:
  DistanceCachePrivate()
    : calculateInverses(false)
  {
  }

  ~DistanceCachePrivate()
  {
  }

  inline static quint64 getHashKey(quint32 id1, quint32 id2)
  {
    return (id1 <= id2) ?
          (static_cast<quint64>(id1) << 32) | id2 :
          (static_cast<quint64>(id2) << 32) | id1;
  }

  inline const CacheEntry getCacheEntry(quint32 id1, quint32 id2) const
  {
    return cache[getHashKey(id1, id2)];
  }

  inline CacheEntry& getCacheEntry(quint32 id1, quint32 id2)
  {
    return cache[getHashKey(id1, id2)];
  }

  QHash<quint64, CacheEntry> cache;
  bool calculateInverses;
};

DistanceCache::DistanceCache(QObject *parent)
  : QObject(parent),
    d_ptr(new DistanceCachePrivate)
{
}

Eigen::Vector3d DistanceCache::getVector(quint32 id_from, quint32 id_to)
{
  Eigen::Vector3d r (this->getStoredVector(id_from, id_to));
  if (id_from > id_to)
    return -r;

  return r;
}

double DistanceCache::getDistance(quint32 id1, quint32 id2)
{
  Q_D(const DistanceCache);
  return d->getCacheEntry(id1, id2).norm;
}

double DistanceCache::getInverseDistance(quint32 id1, quint32 id2)
{
  Q_D(const DistanceCache);
  return d->getCacheEntry(id1, id2).invNorm;
}

bool DistanceCache::calculateInverseDistance() const
{
  Q_D(const DistanceCache);
  return d->calculateInverses;
}

void DistanceCache::reset()
{
  Q_D(DistanceCache);
  return d->cache.clear();
}

void DistanceCache::squeeze()
{
  Q_D(DistanceCache);
  return d->cache.squeeze();
}

void DistanceCache::reserve(quint32 numIds)
{
  Q_D(DistanceCache);
  return d->cache.reserve(numIds);
}

void DistanceCache::addPosition(quint32 id1, const double vec1[3],
                                quint32 id2, const double vec2[3])
{
  Q_D(DistanceCache);

  CacheEntry entry;
  entry.vector[0] = vec2[0] - vec1[0];
  entry.vector[1] = vec2[1] - vec1[1];
  entry.vector[2] = vec2[2] - vec1[2];

  // Reverse sign of vector if id2 > id1
  if (id2 > id1) {
    entry.vector[0] = -entry.vector[0];
    entry.vector[1] = -entry.vector[1];
    entry.vector[2] = -entry.vector[2];
  }

  entry.norm = entry.vector[0]*entry.vector[0] +
      entry.vector[1]*entry.vector[1] + entry.vector[2]*entry.vector[2];

  entry.norm = sqrt(entry.norm);

  if (d->calculateInverses) {
    entry.invNorm = 1.0 / entry.norm;
  }

  d->cache.insert(d->getHashKey(id1, id2), entry);
}

void DistanceCache::setCalculateInverseDistanceOn()
{
  Q_D(DistanceCache);
  d->calculateInverses = true;
}

void DistanceCache::setCalculateInverseDistanceOff()
{
  Q_D(DistanceCache);
  d->calculateInverses = false;
}

const double *DistanceCache::getStoredVector(quint32 id1, quint32 id2)
{
  Q_D(const DistanceCache);
  return d->getCacheEntry(id1, id2).vector;
}
