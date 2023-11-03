#include "ActCluster.h"

#include "ActColors.h"
#include "ActTPCData.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <algorithm>
#include <ios>
#include <iostream>

ActCluster::Cluster::Cluster(int id) : fClusterID(id) {}

ActCluster::Cluster::Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels)
    : fClusterID(id),
      fLine(line),
      fVoxels(voxels)
{
    FillSets(); // not sure yet... X and Y extents can only be computed at fill time for ClIMB
}

void ActCluster::Cluster::FillSets(const ActRoot::Voxel& voxel)
{
    const auto& pos {voxel.GetPosition()};
    fXSet.insert(pos.X());
    fYSet.insert(pos.Y());
    fZSet.insert(pos.Z());
    // for maps
    fYMap[pos.Y()] += 1;
    fXYMap[(int)pos.X()].insert((int)pos.Y());
    fXZMap[(int)pos.X()].insert((int)pos.Z());
}

void ActCluster::Cluster::FillSets()
{
    fXSet.clear();
    fYSet.clear();
    fZSet.clear();
    // Maps
    fYMap.clear();
    fXYMap.clear();
    fXZMap.clear();
    for(const auto& voxel : fVoxels)
    {
        const auto& pos {voxel.GetPosition()};
        fXSet.insert(pos.X());
        fYSet.insert(pos.Y());
        fZSet.insert(pos.Z());
        // Map
        fYMap[pos.Y()] += 1;
        fXYMap[(int)pos.X()].insert((int)pos.Y());
        fXZMap[(int)pos.X()].insert((int)pos.Z());
    }
}

void ActCluster::Cluster::AddVoxel(const ActRoot::Voxel& voxel)
{
    fVoxels.push_back(voxel);
    FillSets(fVoxels.back());
}

void ActCluster::Cluster::AddVoxel(ActRoot::Voxel&& voxel)
{
    fVoxels.push_back(std::move(voxel));
    FillSets(fVoxels.back());
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

ActCluster::Cluster::XYZPoint ActCluster::Cluster::GetGravityPointInXRange(double length)
{
    auto [xmin, xmax] = GetXRange();
    float xbreak {static_cast<float>(xmin + length)};
    float xsum {};
    float ysum {};
    float zsum {};
    int count {};
    for(const auto& voxel : fVoxels)
    {
        auto pos {voxel.GetPosition()};
        bool condX {(xmin <= pos.X()) && (pos.X() <= xbreak)};
        if(condX)
        {
            xsum += pos.X();
            ysum += pos.Y();
            zsum += pos.Z();
            count++;
        }
    }
    xsum /= count;
    ysum /= count;
    zsum /= count;
    return XYZPoint(xsum, ysum, zsum);
}

void ActCluster::Cluster::ReFit()
{
    fLine.FitVoxels(fVoxels);
}

void ActCluster::Cluster::ReFillSets()
{
    FillSets();
}

std::pair<float, float> ActCluster::Cluster::GetXRange() const
{
    if(fXSet.size() > 0)
        return {*fXSet.begin(), *fXSet.rbegin()};
    else
        return {0, 0};
}

std::pair<float, float> ActCluster::Cluster::GetYRange() const 
{
    if(fYSet.size() > 0)
        return {*fYSet.begin(), *fYSet.rbegin()};
    else
        return {0 ,0};
}

std::pair<float, float> ActCluster::Cluster::GetZRange() const 
{
    if(fZSet.size())
        return {*fZSet.begin(), *fZSet.rbegin()};
    else
        return {0, 0};
}

void ActCluster::Cluster::Print() const
{
    auto [xmin, xmax] = GetXRange();
    auto [ymin, ymax] = GetYRange();
    auto [zmin, zmax] = GetZRange();
    std::cout << BOLDCYAN << ".... Cluster " << fClusterID << " ...." << '\n';
    std::cout << "-> N of voxels : " << fVoxels.size() << '\n';
    std::cout << "-> X range     : [" << xmin << " , " << xmax << "]" << '\n';
    std::cout << "-> Y range     : [" << ymin << " , " << ymax << "]" << '\n';
    std::cout << "-> Z range     : [" << zmin << " , " << zmax << "]" << '\n';
    std::cout << "-> IsBeamLike? : " << std::boolalpha << fIsBeamLike << '\n';
    std::cout << "-> IsToMerge?  : " << std::boolalpha << fToMerge << '\n';
    std::cout << "-> HasValidRP? : " << std::boolalpha << fHasValidRP << '\n';
    std::cout << "-> IsToDelete? : " << std::boolalpha << fToDelete << '\n';
    std::cout << "...................." << RESET << '\n';
}
