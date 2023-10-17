#include "ActCluster.h"

ActCluster::Cluster::Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels)
    : fClusterID(id), fLine(line), fVoxels(voxels)
{}

ActCluster::Cluster::XYZPoint ActCluster::Cluster::GetGravityPoint()
{
    float xsum {}; float ysum {}; float zsum {};
    for(const auto& voxel : fVoxels)
    {
        const auto& pos {voxel.GetPosition()};
        xsum += pos.X();
        ysum += pos.Y();
        zsum += pos.Z();
    }
    //Divide by size of vector
    xsum /= fVoxels.size();
    ysum /= fVoxels.size();
    zsum /= fVoxels.size();

    return XYZPoint(xsum, ysum, zsum);
}
