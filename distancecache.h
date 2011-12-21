#ifndef DISTANCECACHE_H
#define DISTANCECACHE_H

#include <QObject>

#include <Eigen/Core>

class DistanceCachePrivate;
class DistanceCache : public QObject
{
  Q_OBJECT
public:
  // The highest id must fit in a quint32
  explicit DistanceCache(QObject *parent = 0);

  Eigen::Vector3d getVector(quint32 id_from, quint32 id_to);
  double getDistance(quint32 id1, quint32 id2);
  double getInverseDistance(quint32 id1, quint32 id2);

  bool calculateInverseDistance() const;

public slots:
  void reset();
  void squeeze();
  // numIds used to optimize internal storage. Should be the approximately the
  // number of ids, not the highest id in the set.
  void reserve(quint32 numIds);

  void addPosition(quint32 id1, const double vec1[3],
                   quint32 id2, const double vec2[3]);

  void setCalculateInverseDistanceOn();
  void setCalculateInverseDistanceOff();

protected:
  DistanceCachePrivate * const d_ptr;

  // @warning This vector always points from min(id1, id2) --> max(id1, id2)
  const double* getStoredVector(quint32 id1, quint32 id2);

private:
  Q_DECLARE_PRIVATE(DistanceCache);
};

#endif // DISTANCECACHE_H
