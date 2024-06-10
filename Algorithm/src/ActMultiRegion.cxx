#include "ActMultiRegion.h"

#include "ActAlgoFuncs.h"
#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActRegion.h"
#include "ActVoxel.h"

#include <algorithm>
#include <cstdlib>
#include <ios>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

ActAlgorithm::MultiRegion::MultiRegion()
{
    fIsVerbose = ActRoot::Options::GetInstance()->GetIsVerbose();
}

void ActAlgorithm::MultiRegion::AddRegion(unsigned int r, const std::vector<double>& vec)
{
    // Assert right dimension
    if(vec.size() != 4)
        throw std::runtime_error("MultiRegion::AddRegion(): vec in config file for idx " + std::to_string(r) +
                                 " has size != 4 required for 2D");
    RegionType type;
    if(r == 0)
        type = RegionType::EBeam;
    else if(r == 1)
        type = RegionType::ELight;
    else if(r == 2)
        type = RegionType::EHeavy;
    else
        type = RegionType::ENone;
    fRegions[type] = {vec[0], vec[1], vec[2], vec[3]};
}

void ActAlgorithm::MultiRegion::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "multiregion.conf";
    // Parse
    ActRoot::InputParser parser {conf};
    auto mr {parser.GetBlock("MultiRegion")};
    // Add regions
    auto regions {mr->GetMappedValuesVectorOf<double>("r")};
    for(const auto& [idx, vec] : regions)
        AddRegion(idx, vec);
    CheckRegions();

    // Enable global algorithm or not
    if(mr->CheckTokenExists("IsEnabled", false))
        fIsEnabled = mr->GetBool("IsEnabled");

    // Parameters of pileup cleaning
    if(mr->CheckTokenExists("EnableCleanPileUp"))
        fEnableCleanPileUp = mr->GetBool("EnableCleanPileUp");
    if(mr->CheckTokenExists("PileUpXPercent"))
        fPileUpXPercent = mr->GetDouble("PileUpXPercent");
    if(mr->CheckTokenExists("PileUpLowerZ"))
        fPileUpLowerZ = mr->GetDouble("PileUpLowerZ");
    if(mr->CheckTokenExists("PileUpUpperZ"))
        fPileUpUpperZ = mr->GetDouble("PileUpUpperZ");

    // Parameters of Merge algorithm
    if(mr->CheckTokenExists("EnableMerge"))
        fEnableMerge = mr->GetBool("EnableMerge");
    if(mr->CheckTokenExists("MergeDistThresh", !fEnableMerge))
        fMergeDistThresh = mr->GetDouble("MergeDistThresh");
    if(mr->CheckTokenExists("MergeMinParallel", !fEnableMerge))
        fMergeMinParallel = mr->GetDouble("MergeMinParallel");
    if(mr->CheckTokenExists("MergeChi2Factor", !fEnableMerge))
        fMergeChi2Factor = mr->GetDouble("MergeChi2Factor");

    // Determination of beam-likes
    if(mr->CheckTokenExists("BLXDirThresh", !fIsEnabled))
        fBlXDirThresh = mr->GetDouble("BLXDirThresh");
    if(mr->CheckTokenExists("BLXBegin", !fIsEnabled))
        fBLXBegin = mr->GetDouble("BLXBegin");

    // Parameters of cluster fine cleaning
    if(mr->CheckTokenExists("EnableClean"))
        fEnableClean = mr->GetBool("EnableClean");
    if(mr->CheckTokenExists("CleanCylinderR", !fEnableClean))
        fCleanCylinderR = mr->GetDouble("CleanCylinderR");
    if(mr->CheckTokenExists("CleanMinVoxels", !fEnableClean))
        fCleanMinVoxels = mr->GetInt("CleanMinVoxels");
    if(mr->CheckTokenExists("CleanMaxChi2", !fEnableClean))
        fCleanMaxChi2 = mr->GetDouble("CleanMaxChi2");

    // Parameters of find RP
    if(mr->CheckTokenExists("RPMaxDist", !fIsEnabled))
        fRPMaxDist = mr->GetDouble("RPMaxDist");
    if(mr->CheckTokenExists("RPClusterDist", !fIsEnabled))
        fRPClusterDist = mr->GetDouble("RPClusterDist");
    if(mr->CheckTokenExists("RPDelete", !fIsEnabled))
        fRPDelete = mr->GetBool("RPDelete");
    if(mr->CheckTokenExists("RPPivotDist", !fIsEnabled))
        fRPPivotDist = mr->GetDouble("RPPivotDist");
    if(mr->CheckTokenExists("RPEnableFine", !fIsEnabled))
        fRPEnableFine = mr->GetBool("RPEnableFine");

    // Cleaning of SplitRP
    if(mr->CheckTokenExists("RPBreakAfter", !fIsEnabled))
        fRPBreakAfter = mr->GetBool("RPBreakAfter");
    if(mr->CheckTokenExists("RPKeepSplit", !fIsEnabled))
        fKeepSplitRP = mr->GetBool("RPKeepSplit");

    // Force a cluster outside beam region
    if(mr->CheckTokenExists("RPOutsideBeam", !fIsEnabled))
        fRPOutsideBeam = mr->GetBool("RPOutsideBeam");

    // Clean
    if(mr->CheckTokenExists("EnableFinalClean", !fIsEnabled))
        fEnableFinalClean = mr->GetBool("EnableFinalClean");
    if(mr->CheckTokenExists("CausalShiftX", !fIsEnabled))
        fCausalShiftX = mr->GetDouble("CausalShiftX");

    // Init clocks
    fClockLabels.push_back("BreakIntoRegions");
    fClocks.push_back({});

    fClockLabels.push_back("BreakingClusters");
    fClocks.push_back({});

    fClockLabels.push_back("ProcessingNotBeam");
    fClocks.push_back({});

    fClockLabels.push_back("MergeClusters");
    fClocks.push_back({});
}

void ActAlgorithm::MultiRegion::CheckRegions()
{
    bool beam {};
    for(const auto& [name, r] : fRegions)
        if(name == RegionType::EBeam)
            beam = true;
    if(!beam)
        throw std::runtime_error("MultiRegion::CheckRegions(): algorithm does not work without a Beam region set. Add "
                                 "it with the r0 command");
}

void ActAlgorithm::MultiRegion::Run()
{
    if(!fIsEnabled)
        return;
    // 1-> Break set vector of clusters into regions
    fClocks[0].Start(false);
    BreakIntoRegions();
    fClocks[0].Stop();
    ResetID();
    // Before merging, clean pileup
    if(fEnableCleanPileUp)
    {
        CleanPileUp();
        ResetID();
    }
    // 2-> Merge similar tracks
    if(fEnableMerge)
    {
        fClocks[3].Start(false);
        MergeClusters();
        fClocks[3].Stop();
        ResetID();
    }
    // 3-> Clean before finding RP
    CleanClusters();
    ResetID();
    // 4-> Identify valid BL in Beam region
    MarkBeamLikes();
    // 5-> Final assignment and sorting
    Sort();
    ResetID();
    // Assign();
    // 6-> RP computation
    FindRP();
    if(fRPDelete)
        DeleteAfterRP();
    // 7-> Fine cluster treatment after RP is found
    DoFinerFits();
    ResetID();
    if(fEnableFinalClean)
        FinalClean();
    // 8-> Improve fit
    if(fRPEnableFine)
        FindFineRP();
    // Always reset ID at the end
    ResetID();
}

ActAlgorithm::RegionType ActAlgorithm::MultiRegion::AssignRangeToRegion(ClusterIt it)
{
    for(const auto& [name, r] : fRegions)
    {
        auto x {it->GetXRange()};
        auto y {it->GetYRange()};
        if(r.IsInside(x, y))
            return name;
    }
    return RegionType::ENone;
}

ActAlgorithm::RegionType ActAlgorithm::MultiRegion::AssignVoxelToRegion(const ActRoot::Voxel& v)
{
    for(const auto& [name, r] : fRegions)
    {
        if(r.IsInside(v.GetPosition()))
            return name;
    }
    return RegionType::ENone;
}

void ActAlgorithm::MultiRegion::ProcessNotBeam(BrokenVoxels& broken)
{
    fClocks[2].Start(false);
    // Auxiliar structure
    std::vector<std::unordered_map<RegionType, std::vector<ActRoot::Voxel>>> aux;
    // 1-> Run for each voxel
    for(const auto& cluster : broken)
    {
        // Init row
        aux.push_back({});
        for(const auto& v : cluster)
        {
            auto r {AssignVoxelToRegion(v)};
            aux.back()[r].push_back(v);
        }
    }
    // Build new clusters and insert back
    auto& clusters {fData->fClusters};
    for(auto& regions : aux)
    {
        for(auto& [name, voxels] : regions)
        {
            auto [newClusters, noise] {fAlgo->Run(voxels)};
            for(int idx = 0, size = newClusters.size(); idx < size; idx++)
            {
                newClusters[idx].SetClusterID(fData->fClusters.size() + idx);
                newClusters[idx].SetRegionType(name);
                if(fIsVerbose)
                {
                    std::cout << "-> Adding new cluster " << idx << '\n';
                    clusters.back().Print();
                }
            }
            fData->fClusters.insert(fData->fClusters.end(), std::make_move_iterator(newClusters.begin()),
                                    std::make_move_iterator(newClusters.end()));
        }
    }
    fClocks[2].Stop();
}

bool ActAlgorithm::MultiRegion::BreakCluster(ClusterIt it, BrokenVoxels& broken)
{
    fClocks[1].Start(false);
    it->SetRegionType(RegionType::EBeam);
    // 1-> Attempt to break the beam region
    if(auto r {fRegions.find(RegionType::EBeam)}; r != fRegions.end())
    {
        if(fIsVerbose)
        {
            std::cout << "-> Breaking cluster " << it->GetClusterID() << '\n';
            std::cout << "   with initial size : " << it->GetSizeOfVoxels() << '\n';
        }
        // Get voxels by reference
        auto& refVoxels {it->GetRefToVoxels()};
        // Partition them according to beam region
        auto part {std::partition(refVoxels.begin(), refVoxels.end(),
                                  [&](const ActRoot::Voxel& v) { return r->second.IsInside(v.GetPosition()); })};
        // New voxels
        broken.push_back({});
        broken.back().insert(broken.back().end(), std::make_move_iterator(part),
                             std::make_move_iterator(refVoxels.end()));
        refVoxels.erase(part, refVoxels.end());
        if(fIsVerbose)
            std::cout << "   after size : " << refVoxels.size() << '\n';
        // Refit
        it->ReFit();
        it->ReFillSets();
    }
    fClocks[1].Stop();
    // Mark to delete is sized fell below threshold
    // Return true if it is fine
    // False if must delete
    return (it->GetSizeOfVoxels() >= fAlgo->GetMinPoints());
}

void ActAlgorithm::MultiRegion::BreakIntoRegions()
{
    if(fIsVerbose)
        std::cout << BOLDYELLOW << "---- MultiRegion Verbose ----" << '\n';
    // New voxels to process later
    BrokenVoxels broken;
    // Clusters to delete
    std::set<unsigned int, std::greater<unsigned int>> toDelete;
    for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
    {
        // Attempt to assign region based on cluster ranges
        auto r {AssignRangeToRegion(it)};
        // If found, save and continue
        if(r != RegionType::ENone)
        {
            it->SetRegionType(r);
            continue;
        }
        else
        {
            auto ok {BreakCluster(it, broken)};
            // If size of remaining voxels is lower than Cluster algorithm threshold, mark to delete
            if(!ok)
                toDelete.insert(std::distance(fData->fClusters.begin(), it));
        }
    }
    // 2-> Delete clusters
    for(const auto& idx : toDelete)
    {
        fData->fClusters.erase(fData->fClusters.begin() + idx);
        if(fIsVerbose)
            std::cout << "-> Deleting cluster " << (fData->fClusters.begin() + idx)->GetClusterID() << '\n';
    }

    // 2-> Process the leftover voxels
    ProcessNotBeam(broken);
    ResetID();
    if(fIsVerbose)
        std::cout << RESET << '\n';
}

void ActAlgorithm::MultiRegion::ResetID()
{
    for(int i = 0, size = fData->fClusters.size(); i < size; i++)
        (fData->fClusters)[i].SetClusterID(i);
}

void ActAlgorithm::MultiRegion::CleanPileUp()
{
    ActAlgorithm::ErasePileup(&fData->fClusters, fPileUpXPercent, fPileUpLowerZ, fPileUpUpperZ, fTPC);
}

void ActAlgorithm::MultiRegion::FindRP()
{
    // Verbose
    if(fIsVerbose)
    {
        std::cout << BOLDGREEN << "---- FindRP verbose ----" << '\n';
        for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
            std::cout << "   cluster #" << it->GetClusterID() << " at region : " << RegionTypeAsStr(it->GetRegionType())
                      << '\n';
    }
    // Init RP strucuture
    RPVector rps;
    for(int i = 0, size = fData->fClusters.size(); i < size; i++)
    {
        auto iit {fData->fClusters.begin() + i};
        for(int j = i + 1; j < size; j++)
        {
            auto jit {fData->fClusters.begin() + j};
            // Excluse comparison of both BL
            if(iit->GetIsBeamLike() && jit->GetIsBeamLike())
                continue;
            // Assert one of them is BL
            if(!(iit->GetIsBeamLike() || jit->GetIsBeamLike()))
                continue;
            if(fRPOutsideBeam &&
               (iit->GetRegionType() == RegionType::EBeam && jit->GetRegionType() == RegionType::EBeam))
                continue;
            // Compute RP
            auto [pA, pB, dist] {ComputeRPIn3D(iit->GetLine().GetPoint(), iit->GetLine().GetDirection(),
                                               jit->GetLine().GetPoint(), jit->GetLine().GetDirection())};
            // Get mean
            XYZPoint rp {(pA.X() + pB.X()) / 2, (pA.Y() + pB.Y()) / 2, (pA.Z() + pB.Z()) / 2};
            // Ckeck all of them are valid
            auto okA {IsRPValid(pA, fTPC)};
            auto okB {IsRPValid(pB, fTPC)};
            auto okAB {IsRPValid(rp, fTPC)};
            // Distance condition
            auto okDist {std::abs(dist) <= fRPMaxDist};
            // Verbose
            if(fIsVerbose)
            {
                std::cout << "······························" << '\n';
                std::cout << "<i, j> : <" << iit->GetClusterID() << ", " << jit->GetClusterID() << ">" << '\n';
                std::cout << "   okA    : " << std::boolalpha << okA << '\n';
                std::cout << "   okB    : " << std::boolalpha << okB << '\n';
                std::cout << "   okAB   : " << std::boolalpha << okAB << '\n';
                std::cout << "   okDist : " << std::boolalpha << okDist << '\n';
                std::cout << "   dist   : " << dist << '\n';
            }
            if(okA && okB && okAB && okDist)
                rps.push_back({rp, {iit->GetClusterID(), jit->GetClusterID()}});
        }
    }
    if(rps.size() == 0)
    {
        if(fIsVerbose)
            std::cout << RESET << '\n';
        // Mark all clusters to delete
        for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
            it->SetToDelete(true);
        return;
    }
    // Print before clustering
    if(fIsVerbose)
    {
        std::cout << "-> Before clustering :" << '\n';
        for(const auto& [rp, idxs] : rps)
            std::cout << "   RP : " << rp << " at <" << idxs.first << ", " << idxs.second << ">" << '\n';
    }

    // Process RPs
    auto proc {SimplifyRPs(rps, fRPClusterDist)};
    fData->fRPs.push_back(proc.first);
    if(fIsVerbose)
    {
        std::cout << "-> IDs in RP : " << '\n';
        for(const auto& id : proc.second)
            std::cout << id << ", ";
        std::cout << '\n';
    }
    // Mark to delete clusters not belonging to RP
    // and the other with the HaveRP flag
    for(int i = 0, size = fData->fClusters.size(); i < size; i++)
    {
        auto it {fData->fClusters.begin() + i};
        bool keep {proc.second.find(i) != proc.second.end()};
        if(!keep)
            it->SetToDelete(true);
        else
            it->SetHasRP(true);
    }

    // Print after clustering
    if(fIsVerbose)
    {
        std::cout << "-> Final RP : " << proc.first << '\n';
        std::cout << "   with #";
        for(const auto& idx : proc.second)
            std::cout << idx << ", ";
        std::cout << '\n';
        std::cout << RESET << '\n';
    }
}

void ActAlgorithm::MultiRegion::DeleteAfterRP()
{
    for(auto it = fData->fClusters.begin(); it != fData->fClusters.end();)
    {
        if(it->GetToDelete() || !it->GetHasRP())
            it = fData->fClusters.erase(it);
        else
            it++;
    }
}

void ActAlgorithm::MultiRegion::MarkBeamLikes()
{
    for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
    {
        // Check if in Beam region
        if(it->GetRegionType() == RegionType::EBeam)
        {
            // Beam-like criteria :
            // -> beamDirection.X() parallel to X
            // -> contains an important amount of Actar.X length
            auto xDir {it->GetLine().GetDirection().Unit().X()};
            bool condDir {std::abs(xDir) >= fBlXDirThresh};
            auto [xmin, xmax] {it->GetXRange()};
            bool condBegin {xmin <= fBLXBegin};
            if(condDir && condBegin)
                it->SetBeamLike(true);
        }
    }
}

void ActAlgorithm::MultiRegion::MergeClusters()
{
    MergeSimilarClusters(&(fData->fClusters), fMergeDistThresh, fMergeMinParallel, fMergeChi2Factor, fIsVerbose);
    // Reset index
    ResetID();
}

void ActAlgorithm::MultiRegion::CleanClusters()
{
    // 1-> Clean using cylinder subroutine
    CylinderCleaning(&fData->fClusters, fCleanCylinderR, fAlgo->GetMinPoints(), fIsVerbose);
    // 2-> Delete small cluster or with high chi2
    Chi2AndSizeCleaning(&fData->fClusters, fCleanMaxChi2, fCleanMinVoxels, fIsVerbose);
}

void ActAlgorithm::MultiRegion::Sort()
{
    // Sort according to size
    std::sort(fData->fClusters.begin(), fData->fClusters.end(),
              [](const auto& l, const auto& r) { return l.GetSizeOfVoxels() > r.GetSizeOfVoxels(); });
}

void ActAlgorithm::MultiRegion::DoFinerFits()
{
    if(fData->fRPs.size() == 0)
        return;
    // 1-> Split BL into heavy
    BreakBeamToHeavy(&fData->fClusters, fData->fRPs.front(), fAlgo->GetMinPoints(), fKeepSplitRP, fIsVerbose);
    // Decide whether to keep or delete SplitRP
    // int nBL {};
    // int notBLnotSplit {};
    // bool deleteSplit {false};
    // for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
    // {
    //     if(it->GetIsBeamLike())
    //         nBL++;
    //     else
    //     {
    //         if(!it->GetIsSplitRP())
    //             notBLnotSplit++;
    //     }
    // }
    // // Only set to delete in this case
    // if(nBL == 1)
    // {
    //     if(notBLnotSplit > 1)
    //         deleteSplit = true;
    // }
    // if(deleteSplit && !fKeepSplitRP)
    // {
    //     for(auto it = fData->fClusters.begin(); it != fData->fClusters.end();)
    //     {
    //         if(it->GetIsSplitRP())
    //         {
    //             if(fIsVerbose)
    //             {
    //                 std::cout << BOLDRED << "-> Deleting SplitRP at #" << it->GetClusterID() << RESET << '\n';
    //             }
    //             it = fData->fClusters.erase(it);
    //         }
    //         else
    //             it++;
    //     }
    // }

    // 2-> Mask region around RP
    if(fRPEnableFine)
        MaskBeginEnd(&fData->fClusters, fData->fRPs.front(), fRPPivotDist, fAlgo->GetMinPoints(), fIsVerbose);
}

void ActAlgorithm::MultiRegion::FindFineRP()
{
    if(fData->fRPs.size() == 0)
        return;
    // Get statistics
    int nBL {};
    int nHasRP {};
    for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
    {
        if(it->GetHasRP())
        {
            nHasRP++;
            if(it->GetIsBeamLike())
                nBL++;
        }
    }
    // Determine whether to consider all nonBL clusters in fine RP or just the light one
    // when there are 2 recoil tracks (light + heavy), just use the light in the computation
    bool onlyUseLight {std::abs(nHasRP - nBL) <= 2};

    if(fIsVerbose)
    {
        std::cout << BOLDCYAN << "---- Fine RP ----" << '\n';
        std::cout << "-> Cluster stats :" << '\n';
        std::cout << "   nBL           : " << nBL << '\n';
        std::cout << "   nHasRP        : " << nHasRP << '\n';
        std::cout << "   onlyUseLight  ? " << std::boolalpha << onlyUseLight << '\n';
    }

    auto comp {[](const auto& l, const auto& r) { return l.first > r.first; }};
    std::set<std::pair<double, std::pair<XYZPoint, std::pair<int, int>>>, decltype(comp)> rpSet(comp);
    for(auto iit = fData->fClusters.begin(); iit != fData->fClusters.end(); iit++)
    {
        if(iit->GetIsBeamLike() && iit->GetHasRP())
        {
            for(auto jit = fData->fClusters.begin(); jit != fData->fClusters.end(); jit++)
            {
                if(iit == jit || jit->GetIsBeamLike() || !jit->GetHasRP())
                    continue;
                // Find rps// Compute RP
                auto [pA, pB, dist] {ComputeRPIn3D(iit->GetLine().GetPoint(), iit->GetLine().GetDirection(),
                                                   jit->GetLine().GetPoint(), jit->GetLine().GetDirection())};
                // Get mean
                XYZPoint rp {(pA.X() + pB.X()) / 2, (pA.Y() + pB.Y()) / 2, (pA.Z() + pB.Z()) / 2};
                // Ckeck all of them are valid
                auto okA {IsRPValid(pA, fTPC)};
                auto okB {IsRPValid(pB, fTPC)};
                auto okAB {IsRPValid(rp, fTPC)};
                // Distance condition
                auto okDist {std::abs(dist) <= fRPMaxDist};
                // Verbose
                if(fIsVerbose)
                {
                    std::cout << "······························" << '\n';
                    std::cout << "<i, j> : <" << iit->GetClusterID() << ", " << jit->GetClusterID() << ">" << '\n';
                    std::cout << "   okA    : " << std::boolalpha << okA << '\n';
                    std::cout << "   okB    : " << std::boolalpha << okB << '\n';
                    std::cout << "   okAB   : " << std::boolalpha << okAB << '\n';
                    std::cout << "   okDist : " << std::boolalpha << okDist << '\n';
                    std::cout << "   dist   : " << dist << '\n';
                }
                if(okA && okB && okAB && okDist)
                {
                    auto theta {GetClusterAngle(iit->GetLine().GetDirection(), jit->GetLine().GetDirection())};
                    rpSet.insert({theta, {rp, {iit->GetClusterID(), jit->GetClusterID()}}});
                }
            }
        }
    }
    // Get RP vector
    RPVector rpVec;
    for(const auto& [theta, vals] : rpSet)
    {
        const auto& rp {vals.first};
        const auto& idxs {vals.second};
        rpVec.push_back({rp, idxs});
        if(fIsVerbose)
        {
            std::cout << "-> Using in fine computation :" << '\n';
            std::cout << "   RP : " << rp << " at <" << idxs.first << ", " << idxs.second << ">" << '\n';
            std::cout << "   Theta : " << theta << '\n';
        }
        if(onlyUseLight)
            break;
    }
    // Process RPs
    // If fine RP fails, keep prelimar RP
    if(rpVec.size() == 0)
    {
        if(fIsVerbose)
            std::cout << "-> Fine RP failed! rpVec.size() == 0" << RESET << '\n';
        return;
    }
    auto proc {SimplifyRPs(rpVec, fRPClusterDist)};
    fData->fRPs.clear();
    fData->fRPs.push_back(proc.first);
    if(fIsVerbose)
    {
        std::cout << "-> Fine RP : " << fData->fRPs.front() << RESET << '\n';
    }
}

void ActAlgorithm::MultiRegion::FinalClean()
{
    if(fData->fClusters.size() == 0)
        return;
    // 1-> Cleaning based on size of cluster
    Chi2AndSizeCleaning(&fData->fClusters, fCleanMaxChi2, fCleanMinVoxels, fIsVerbose);
    // 2-> Cleaning based on causality
    if(fData->fRPs.size() > 0)
    {
        // Get the X position of the RP
        auto rpX {fData->fRPs.front().X()};
        // Get the minimum position of a non-beam cluster
        double otherXMin {11111};
        for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
        {
            if(!it->GetIsBeamLike())
            {
                auto [xmin, xmax] {it->GetXRange()};
                if(xmin < otherXMin)
                    otherXMin = xmin;
            }
        }
        // Clear
        if(otherXMin < (rpX - fCausalShiftX))
        {
            if(fIsVerbose)
            {
                std::cout << BOLDGREEN;
                std::cout << "-> Causal cleaning : " << '\n';
                std::cout << "   RP.X() : " << rpX << '\n';
                std::cout << "   otherX : " << otherXMin << '\n';
                std::cout << "   ShiftX : " << fCausalShiftX << RESET << '\n';
            }
            // Delete
            fData->fClusters.clear();
            fData->fRPs.clear();
        }
    }
    // // Get beam like last position in X
    // double maxXBeam {-1};
    // double minXOther {11111};
    // for(auto it = fData->fClusters.begin(); it != fData->fClusters.end(); it++)
    // {
    //     if(it->GetIsBeamLike())
    //     {
    //         auto [xmin, xmax] {it->GetXRange()};
    //         if(xmax > maxXBeam)
    //             maxXBeam = xmax;
    //     }
    //     else
    //     {
    //         auto [xmin, xmax] {it->GetXRange()};
    //         if(xmin < minXOther)
    //             minXOther = xmin;
    //     }
    // }
    // And delete everything based on size
    if(fData->fClusters.size() <= 1 && fEnableCleanMult1)
    {
        if(fIsVerbose)
        {
            std::cout << BOLDRED << "-> Final clean : erasing all in this event" << RESET << '\n';
        }
        fData->fClusters.clear();
        fData->fRPs.clear();
    }
}

void ActAlgorithm::MultiRegion::Print() const
{
    std::cout << BOLDCYAN << "**** MultiRegion ****" << '\n';
    std::cout << "-> IsEnabled   ? " << std::boolalpha << fIsEnabled << '\n';
    if(fIsEnabled)
    {
        for(const auto& [type, r] : fRegions)
        {
            std::cout << "-> Region : " << RegionTypeAsStr(type) << '\n';
            std::cout << "   ";
            r.Print();
        }
        std::cout << "-> EnableCleanPileUp ? " << std::boolalpha << fEnableCleanPileUp << '\n';
        if(fEnableCleanPileUp)
        {
            std::cout << "  PileUpXPercent : " << fPileUpXPercent << '\n';
            std::cout << "  PileUpLowerZ   : " << fPileUpLowerZ << '\n';
            std::cout << "  PileUpUpperZ   : " << fPileUpUpperZ << '\n';
        }
        std::cout << "-> EnableMerge ? " << std::boolalpha << fEnableMerge << '\n';
        if(fEnableMerge)
        {
            std::cout << "   MergeDistThresh  : " << fMergeDistThresh << '\n';
            std::cout << "   MergeMinParallel : " << fMergeMinParallel << '\n';
            std::cout << "   MergeChi2Factor  : " << fMergeChi2Factor << '\n';
        }
        std::cout << "-> EnableClean ? " << std::boolalpha << fEnableClean << '\n';
        if(fEnableClean)
        {
            std::cout << "   CleanCylinderR   : " << fCleanCylinderR << '\n';
            std::cout << "   CleanMinVoxels   : " << fCleanMinVoxels << '\n';
            std::cout << "   CleanMaxChi2     : " << fCleanMaxChi2 << '\n';
        }
        std::cout << "-> BLXDirThresh     : " << fBlXDirThresh << '\n';
        std::cout << "-> BLXBegin         : " << fBLXBegin << '\n';
        std::cout << "-> RPMaxDist        : " << fRPMaxDist << '\n';
        std::cout << "-> RPClusterDist    : " << fRPClusterDist << '\n';
        std::cout << "-> RPDelete         ? " << std::boolalpha << fRPDelete << '\n';
        std::cout << "-> RPPivotDist      : " << fRPPivotDist << '\n';
        std::cout << "-> RPBreakAfter     ? " << std::boolalpha << fRPBreakAfter << '\n';
        std::cout << "-> RPKeepSplitRP    ? " << std::boolalpha << fKeepSplitRP << '\n';
        std::cout << "-> RPOutsideBeam    ? " << std::boolalpha << fRPOutsideBeam << '\n';
        std::cout << "-> RPEnableFine     ? " << std::boolalpha << fRPEnableFine << '\n';
        std::cout << "-> EnableFinalClean ? " << std::boolalpha << fEnableFinalClean << '\n';
        std::cout << "-> CausalShiftX     : " << fCausalShiftX << '\n';
        std::cout << "-> EnableCleanMult1 ? " << std::boolalpha << fEnableCleanMult1 << '\n';
    }
    std::cout << "******************************" << RESET << '\n';
}

void ActAlgorithm::MultiRegion::PrintReports() const
{
    std::cout << BOLDMAGENTA << "---- MultiRegion time reports ----" << '\n';
    for(int i = 0; i < fClockLabels.size(); i++)
    {
        std::cout << "-> Clock : " << fClockLabels[i] << '\n';
        std::cout << "   ";
        fClocks[i].Print();
    }
    std::cout << RESET << '\n';
}
