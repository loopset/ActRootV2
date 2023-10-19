#include "ActCluster.h"
#include "ActColors.h"
#include <iostream>

ActCluster::Cluster::Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels)
    : fClusterID(id)
    , fLine(line)
    , fVoxels(voxels)
{
}

ActCluster::Cluster::XYZPoint ActCluster::Cluster::GetGravityPointInRegion(double xmin, double xmax, double ymin,
                                                                           double ymax, double zmin, double zmax)
{
    float xsum {};
    float ysum {};
    float zsum {};
    int count {};
    for(const auto& voxel : fVoxels)
    {
        const auto& pos {voxel.GetPosition()};
        bool condX {(xmin <= pos.X()) && (pos.X() <= xmax)};
        bool condY {true};
        if(ymin != -1 && ymax != -1)
            condY = (ymin <= pos.Y()) && (pos.Y() <= ymax);
        bool condZ {true};
        if(zmin != -1 && zmax != -1)
            condZ = (zmin <= pos.Z()) && (pos.Z() <= zmax);
        if(condX && condY && condZ)
        {
            xsum += pos.X();
            ysum += pos.Y();
            zsum += pos.Z();
            count++;
        }
    }
    // Divide by count of voxels
    xsum /= count;
    ysum /= count;
    zsum /= count;

    return XYZPoint(xsum, ysum, zsum);
}

void ActCluster::Cluster::ReFit() { fLine.FitVoxels(fVoxels); }

void ActCluster::Cluster::Print() const
{
    std::cout << BOLDCYAN << ".... Cluster " << fClusterID << " ...." << '\n';
    std::cout << "-> N of voxels : " << fVoxels.size() << '\n';
    std::cout << "...................." << RESET << '\n';
}
