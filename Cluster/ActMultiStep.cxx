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
    // 1-> Check whether we need to do this step
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        if(it->GetLine().GetChi2() < fChi2Threshold)
            continue;
        // 2-> Get voxels outside beam region
        auto notbeam {SeparateBeamRegion(it)};
        for(const auto& e : notbeam)
            std::cout<<BOLDGREEN<<"after func = "<<e.GetPosition()<<'\n';
        // 3-> Clusterize not beam voxels
        ClusterNotBeamRegion(notbeam);
    }
}

std::vector<ActRoot::Voxel> ActCluster::MultiStep::SeparateBeamRegion(ItType cluster)
{
    // Debug
    // std::cout << BOLDRED << "Breaking cluster n " << cluster->GetClusterID() << '\n';
    // std::vector<ActRoot::Voxel> ret {};
    // // Get mean below fEntranceBeamRegionX
    // ROOT::Math::XYZPointF gravity {};
    // int count {};
    // for(const auto& voxel : cluster->GetVoxels())
    // {
    //     const auto& pos {voxel.GetPosition()};
    //     if(pos.X() < fEntranceBeamRegionX)
    //     {
    //         gravity += ROOT::Math::XYZVectorF {pos};
    //         count++;
    //     }
    // }
    // gravity /= count;
    // std::cout << "-> Gravity point at entrance = " << gravity << '\n';
    // // Open cylinder around mean point
    // // and move not beam-like voxels
    // auto& refvoxels {cluster->GetRefToVoxels()};
    // for(auto it = refvoxels.begin(); it != refvoxels.end(); it++)
    // {
    //     const auto& pos {it->GetPosition()};
    //     if(IsInBeamCylinder(pos, gravity))
    //         continue;
    //     ret.push_back(*it);
        //refvoxels.erase(it);
    // }
    // for(const auto& e : refvoxels)
    //     std::cout<<BOLDCYAN<<"original = "<<e.GetPosition()<<'\n';
    // for(const auto& e : ret)
    //     std::cout << "no beam el = " << e.GetPosition() << '\n';
    //
    // 1-> Partition keeps voxels in cylinder
    // auto toMove =
    //     std::stable_partition(refvoxels.begin(), refvoxels.end(),
    //                           [&](const ActRoot::Voxel& voxel) { return IsInBeamCylinder(voxel.GetPosition(), gravity); });
    // // 2-> Move to ret vector
    // std::move(toMove, refvoxels.end(), std::back_inserter(ret));
    // // 3-> Erase from original vector
    // refvoxels.erase(toMove, refvoxels.end());
    // for(const auto& e : refvoxels)
    //     std::cout<<BOLDGREEN<<"remaining = "<<e.GetPosition()<<'\n';
    // for(const auto& e : ret)
    //     std::cout<<BOLDRED<<" ret = "<<e.GetPosition()<<'\n';
    //
    // std::cout << "-> Beam size = " << refvoxels.size() << '\n';
    // std::cout << "-> Not beam size = " << ret.size() << RESET << '\n';
    // return std::move(ret);
    return {};
}

void ActCluster::MultiStep::ClusterNotBeamRegion(const std::vector<ActRoot::Voxel>& voxels)
{
    // Cluster again
    auto notBeamClusters = fClimb->Run(voxels);
    // Insert back
    fClusters->insert(fClusters->end(), std::make_move_iterator(notBeamClusters.begin()),
                      std::make_move_iterator(notBeamClusters.end()));
}

void ActCluster::MultiStep::Print() const
{
    std::cout << BOLDCYAN << "==== MultiStep settings ====" << '\n';
    std::cout << "-> Chi2 thresh   : " << fChi2Threshold << '\n';
    std::cout << "-> X extent beam : " << fEntranceBeamRegionX << '\n';
    std::cout << "-> BeamWindowY   : " << fBeamWindowY << '\n';
    std::cout << "-> BeamWindowZ   : " << fBeamWindowZ << '\n';
    std::cout << "=======================" << RESET << '\n';
}
