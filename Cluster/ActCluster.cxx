#include "ActCluster.h"

ActCluster::Cluster::Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels)
    : fClusterID(id), fLine(line), fVoxels(voxels)
{}
