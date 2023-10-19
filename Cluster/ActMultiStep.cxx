#include "ActMultiStep.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"

#include "ActTPCData.h"
#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"
#include "TEnv.h"
#include "TSystem.h"

#include <algorithm>
#include <cmath>
#include <ios>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

void ActCluster::MultiStep::ReadConfigurationFile(const std::string& infile)
{
    // automatically get project path from gEnv
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/filter.climb";
    std::string realfile {};
    if(!gSystem->AccessPathName(envfile.c_str()))
        realfile = envfile;
    else if(infile.length() > 0)
        realfile = infile;
    else
    {
        std::cout << BOLDMAGENTA << ".ransac config file not found -> Using built-in configuration" << '\n';
        return;
    }

    // Parse!
    ActRoot::InputParser parser {realfile};
    auto mb {parser.GetBlock("MultiStep")};
    if(mb->CheckTokenExists("IsEnabled"))
        fIsEnabled = mb->GetBool("IsEnabled");
    if(mb->CheckTokenExists("FitNotBeam"))
        fFitNotBeam = mb->GetBool("FitNotBeam");
    if(mb->CheckTokenExists("Chi2Threshold"))
        fChi2Threshold = mb->GetDouble("Chi2Threshold");
    if(mb->CheckTokenExists("EntranceBeamRegionX"))
        fEntranceBeamRegionX = mb->GetDouble("EntranceBeamRegionX");
    if(mb->CheckTokenExists("BeamWindowY"))
        fBeamWindowY = mb->GetDouble("BeamWindowY");
    if(mb->CheckTokenExists("BeamWindowZ"))
        fBeamWindowZ = mb->GetDouble("BeamWindowZ");
}

void ActCluster::MultiStep::RunBreakBeamClusters()
{
    if(!fIsEnabled)
        return;
    std::vector<ActCluster::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // 1-> Check whether we meet conditions to execute this
        if(it->GetLine().GetChi2() < fChi2Threshold)
            continue;
        // 2-> Calculate gravity point in region
        auto gravity {it->GetGravityPointInRegion(0, fEntranceBeamRegionX)};
        // Return if it is nan
        if(std::isnan(gravity.X()) || std::isnan(gravity.Y()) || std::isnan(gravity.Z()))
            continue;

        std::cout << BOLDGREEN << "Running for cluster " << it->GetClusterID() << '\n';
        std::cout << BOLDRED << "Gravity = " << gravity << '\n';
        // 3->Modify original cluster: move non-beam voxels outside to
        //  be clusterized independently
        auto& refToVoxels {it->GetRefToVoxels()};
        std::cout << "Original size = " << refToVoxels.size() << '\n';
        auto toMove {std::partition(refToVoxels.begin(), refToVoxels.end(),
                                    [&](const ActRoot::Voxel& voxel)
                                    {
                                        const auto& pos {voxel.GetPosition()};
                                        return IsInBeamCylinder(pos, gravity);
                                    })};
        // Create vector to move to
        std::vector<ActRoot::Voxel> notBeam {};
        std::move(toMove, refToVoxels.end(), std::back_inserter(notBeam));
        refToVoxels.erase(toMove, refToVoxels.end());
        std::cout << "Remaining beam = " << refToVoxels.size() << '\n';
        std::cout << "Not beam       = " << notBeam.size() << RESET << '\n';
        // ReFit remaining voxels!
        it->ReFit();
        // 4-> Run cluster algorithm again
        std::vector<ActCluster::Cluster> newClusters;
        if(fFitNotBeam)
            newClusters = fClimb->Run(notBeam);
        // Move to vector
        std::move(newClusters.begin(), newClusters.end(), std::back_inserter(toAppend));
    }
    // Append clusters to original TPCData
    std::move(toAppend.begin(), toAppend.end(), std::back_inserter(*fClusters));
    // Reset cluster index
    int id {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        it->SetClusterID(id);
        id++;
    }
}

bool ActCluster::MultiStep::IsInBeamCylinder(const XYZPoint& pos, const XYZPoint& gravity)
{
    bool condY {(gravity.Y() - fBeamWindowY) <= pos.Y() && pos.Y() <= (gravity.Y() + fBeamWindowY)};
    bool condZ {(gravity.Z() - fBeamWindowZ) <= pos.Z() && pos.Z() <= (gravity.Z() + fBeamWindowZ)};

    return condY && condZ;
}

void ActCluster::MultiStep::Print() const
{
    std::cout << BOLDCYAN << "==== MultiStep settings ====" << '\n';
    std::cout << "-> IsEnabled     : " << std::boolalpha << fIsEnabled << '\n';
    if(fIsEnabled)
    {
        std::cout << "-> FitNotBeam    : " << std::boolalpha << fFitNotBeam << '\n';
        std::cout << "-> Chi2 thresh   : " << fChi2Threshold << '\n';
        std::cout << "-> X extent beam : " << fEntranceBeamRegionX << '\n';
        std::cout << "-> BeamWindowY   : " << fBeamWindowY << '\n';
        std::cout << "-> BeamWindowZ   : " << fBeamWindowZ << '\n';
    }
    std::cout << "=======================" << RESET << '\n';
}
