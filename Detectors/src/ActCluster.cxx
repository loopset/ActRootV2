#include "ActCluster.h"

#include "ActColors.h"
#include "ActRegion.h"
#include "ActVoxel.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "Math/Vector3Dfwd.h"

#include <algorithm>
#include <ios>
#include <iostream>

ActRoot::Cluster::Cluster(int id) : fClusterID(id) {}

ActRoot::Cluster::Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels)
    : fClusterID(id),
      fLine(line),
      fVoxels(voxels)
{
    FillSets(); // not sure yet... X and Y extents can only be computed at fill time for ClIMB
}

void ActRoot::Cluster::UpdateRange(float val, RangeType& range)
{
    // Min
    if(val < range.first)
        range.first = val;
    // Max
    if(val > range.second)
        range.second = val;
}

void ActRoot::Cluster::FillSets(const ActRoot::Voxel& voxel)
{
    const auto& pos {voxel.GetPosition()};
    UpdateRange(pos.X(), fXRange);
    UpdateRange(pos.Y(), fYRange);
    UpdateRange(pos.Z(), fZRange);
}

void ActRoot::Cluster::FillSets()
{
    fXRange = {1111, -1};
    fYRange = {1111, -1};
    fZRange = {1111, -1};
    for(const auto& voxel : fVoxels)
    {
        const auto& pos {voxel.GetPosition()};
        UpdateRange(pos.X(), fXRange);
        UpdateRange(pos.Y(), fYRange);
        UpdateRange(pos.Z(), fZRange);
    }
}

void ActRoot::Cluster::AddVoxel(const ActRoot::Voxel& voxel)
{
    fVoxels.push_back(voxel);
    FillSets(fVoxels.back());
}

void ActRoot::Cluster::AddVoxel(ActRoot::Voxel&& voxel)
{
    fVoxels.push_back(std::move(voxel));
    FillSets(fVoxels.back());
}

ActRoot::Cluster::XYZPointF
ActRoot::Cluster::GetGravityPointInRegion(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
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

    return XYZPointF(xsum, ysum, zsum);
}

ActRoot::Cluster::XYZPointF ActRoot::Cluster::GetGravityPointInXRange(double length)
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
    return XYZPointF(xsum, ysum, zsum);
}

void ActRoot::Cluster::ReFit()
{
    fLine.FitVoxels(fVoxels, true, true, fUseExtVoxels);
}

void ActRoot::Cluster::ReFillSets()
{
    FillSets();
}

std::pair<float, float> ActRoot::Cluster::GetXRange() const
{
    if(fXRange.second != -1)
        return fXRange;
    else
        return {0, 0};
}

std::pair<float, float> ActRoot::Cluster::GetYRange() const
{
    if(fYRange.second != -1)
        return fYRange;
    else
        return {0, 0};
}

std::pair<float, float> ActRoot::Cluster::GetZRange() const
{
    if(fZRange.second != -1)
        return fZRange;
    else
        return {0, 0};
}

void ActRoot::Cluster::SortAlongDir()
{
    // Not necessary to correct for {0.5, 0.5, 0.5} offset since
    // this we are comparing all points and it would be a common factor
    XYZPointF ref {fLine.GetPoint() - 1000 * fLine.GetDirection().Unit()};
    std::sort(fVoxels.begin(), fVoxels.end(),
              [&](const Voxel& l, const Voxel& r)
              {
                  // Sort using distance to the reference point
                  auto ld {(fLine.ProjectionPointOnLine(l.GetPosition()) - ref).R()};
                  auto rd {(fLine.ProjectionPointOnLine(r.GetPosition()) - ref).R()};
                  return ld < rd;
              });
}

void ActRoot::Cluster::ScaleVoxels(float xy, float z)
{
    std::for_each(fVoxels.begin(), fVoxels.end(),
                  [&](Voxel& v)
                  {
                      auto pos {v.GetPosition()}; // not extended voxels!
                      // In this inner function is mandatory to add offset
                      // Because we're working with the pad/tb number!
                      pos += ROOT::Math::XYZVectorF {0.5, 0.5, 0.5};
                      v.SetPosition({pos.X() * xy, pos.Y() * xy, pos.Z() * z});
                  });
    fLine.Scale(xy, z);
    ReFillSets();
}

void ActRoot::Cluster::Print() const
{
    auto [xmin, xmax] = GetXRange();
    auto [ymin, ymax] = GetYRange();
    auto [zmin, zmax] = GetZRange();
    std::cout << BOLDCYAN << ".... Cluster " << fClusterID << " ...." << '\n';
    std::cout << "-> N of voxels : " << fVoxels.size() << '\n';
    std::cout << "-> X range     : [" << xmin << " , " << xmax << "]" << '\n';
    std::cout << "-> Y range     : [" << ymin << " , " << ymax << "]" << '\n';
    std::cout << "-> Z range     : [" << zmin << " , " << zmax << "]" << '\n';
    std::cout << "-> IsBeamLike  ? " << std::boolalpha << fIsBeamLike << '\n';
    std::cout << "-> IsRecoil    ? " << std::boolalpha << fIsRecoil << '\n';
    std::cout << "-> IsToMerge   ? " << std::boolalpha << fToMerge << '\n';
    std::cout << "-> BreakBeam   ? " << std::boolalpha << fIsBreak << '\n';
    std::cout << "-> SplitRP     ? " << std::boolalpha << fIsSplit << '\n';
    std::cout << "-> IsToDelete  ? " << std::boolalpha << fToDelete << '\n';
    std::cout << "-> RegionType  : " << ActAlgorithm::RegionTypeAsStr(fRegion) << '\n';
    std::cout << "-> HasRP       ? " << std::boolalpha << fHasRP << '\n';
    std::cout << "...................." << RESET << '\n';
}
