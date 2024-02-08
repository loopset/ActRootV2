#include "ActMultiStep.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActInterval.h"
#include "ActLine.h"
#include "ActOptions.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"
#include "ActUtils.h"
#include "ActVoxel.h"

#include "TMath.h"
#include "TMathBase.h"
#include "TMatrixD.h"
#include "TMatrixDfwd.h"
#include "TMatrixF.h"
#include "TStopwatch.h"

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

ActAlgorithm::MultiStep::MultiStep()
{
    fIsVerbose = ActRoot::Options::GetInstance()->GetIsVerbose();
}

void ActAlgorithm::MultiStep::SetTPCData(ActRoot::TPCData* data)
{
    fData = data;
    fClusters = &(data->fClusters);
    fRPs = &(data->fRPs);
}

void ActAlgorithm::MultiStep::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "/multistep.conf";
    // Parse!
    ActRoot::InputParser parser {conf};
    auto mb {parser.GetBlock("MultiStep")};
    // General parameters
    if(mb->CheckTokenExists("IsEnabled"))
        fIsEnabled = mb->GetBool("IsEnabled");
    // Parameters of break multibeam
    if(mb->CheckTokenExists("EnableBreakMultiBeam"))
        fEnableBreakMultiBeam = mb->GetBool("EnableBreakMultiBeam");
    if(mb->CheckTokenExists("FitNotBeam"))
        fFitNotBeam = mb->GetBool("FitNotBeam");
    if(mb->CheckTokenExists("Chi2Threshold"))
        fChi2Threshold = mb->GetDouble("Chi2Threshold");
    if(mb->CheckTokenExists("MinSpanX"))
        fMinSpanX = mb->GetDouble("MinSpanX");
    if(mb->CheckTokenExists("LengthXToBreak"))
        fLengthXToBreak = mb->GetDouble("LengthXToBreak");
    if(mb->CheckTokenExists("BeamWindowY"))
        fBeamWindowY = mb->GetDouble("BeamWindowY");
    if(mb->CheckTokenExists("BeamWindowZ"))
        fBeamWindowZ = mb->GetDouble("BeamWindowZ");
    if(mb->CheckTokenExists("BreakLengthThreshold"))
        fBreakLengthThres = mb->GetInt("BreakLengthThreshold");
    // Parameters to break multitracks
    if(mb->CheckTokenExists("EnableBreakMultiTracks"))
        fEnableBreakMultiTracks = mb->GetBool("EnableBreakMultiTracks");
    if(mb->CheckTokenExists("TrackChi2Threshold"))
        fTrackChi2Threshold = mb->GetDouble("TrackChi2Threshold");
    if(mb->CheckTokenExists("BeamWindowScaling"))
        fBeamWindowScaling = mb->GetDouble("BeamWindowScaling");
    // Parameters of cleaning of pileup
    if(mb->CheckTokenExists("EnableCleanPileUp"))
        fEnableCleanPileUp = mb->GetBool("EnableCleanPileUp");
    if(mb->CheckTokenExists("PileUpXPercent"))
        fPileUpXPercent = mb->GetDouble("PileUpXPercent");
    if(mb->CheckTokenExists("BeamLowerZ"))
        fBeamLowerZ = mb->GetDouble("BeamLowerZ");
    if(mb->CheckTokenExists("BeamUpperZ"))
        fBeamUpperZ = mb->GetDouble("BeamUpperZ");
    // Parameters of cleaning vertical tracks
    if(mb->CheckTokenExists("EnableCleanZs"))
        fEnableCleanZs = mb->GetBool("EnableCleanZs");
    if(mb->CheckTokenExists("ZDirectionThreshold"))
        fZDirectionThreshold = mb->GetDouble("ZDirectionThreshold");
    if(mb->CheckTokenExists("ZMinSpanInPlane"))
        fZMinSpanInPlane = mb->GetDouble("ZMinSpanInPlane");
    // Merge similar tracks
    if(mb->CheckTokenExists("EnableMerge"))
        fEnableMerge = mb->GetBool("EnableMerge");
    if(mb->CheckTokenExists("MergeMinParallelFactor"))
        fMergeMinParallelFactor = mb->GetDouble("MergeMinParallelFactor");
    if(mb->CheckTokenExists("MergeChi2CoverageF"))
        fMergeChi2CoverageFactor = mb->GetDouble("MergeChi2CoverageF");
    if(mb->CheckTokenExists("MergeDistThresh"))
        fMergeDistThreshold = mb->GetDouble("MergeDistThresh");
    // Clean delta electrons and remaining non-apt cluster
    if(mb->CheckTokenExists("EnableCleanDeltas"))
        fEnableCleanDeltas = mb->GetBool("EnableCleanDeltas");
    if(mb->CheckTokenExists("DeltaChi2Threshold"))
        fDeltaChi2Threshold = mb->GetDouble("DeltaChi2Threshold");
    if(mb->CheckTokenExists("DeltaMaxVoxels"))
        fDeltaMaxVoxels = mb->GetDouble("DeltaMaxVoxels");
    // RP calculation
    if(mb->CheckTokenExists("EnableRPRoutine"))
        fEnableRPRoutine = mb->GetBool("EnableRPRoutine");
    //// Find Beam-like
    if(mb->CheckTokenExists("BeamLikeXMinThresh"))
        fBeamLikeXMinThresh = mb->GetDouble("BeamLikeXMinThresh");
    if(mb->CheckTokenExists("BeamLikeParallelF"))
        fBeamLikeParallelF = mb->GetDouble("BeamLikeParallelF");
    if(mb->CheckTokenExists("BeamLikeMinVoxels"))
        fBeamLikeMinVoxels = mb->GetDouble("BeamLikeMinVoxels");
    //// Delete cluster without valid RP
    if(mb->CheckTokenExists("EnableRPDelete"))
        fEnableRPDelete = mb->GetBool("EnableRPDelete");
    if(mb->CheckTokenExists("RPDistThresh"))
        fRPDistThresh = mb->GetDouble("RPDistThresh");
    if(mb->CheckTokenExists("RPDistCluster"))
        fRPDistCluster = mb->GetDouble("RPDistCluster");
    if(mb->CheckTokenExists("RPDistValidate"))
        fRPDistValidate = mb->GetDouble("RPDistValidate");
    //// Enable finer determination of RP
    if(mb->CheckTokenExists("EnableFineRP"))
        fEnableFineRP = mb->GetBool("EnableFineRP");
    if(mb->CheckTokenExists("RPMaskXY"))
        fRPMaskXY = mb->GetDouble("RPMaskXY");
    if(mb->CheckTokenExists("RPMaskZ"))
        fRPMaskZ = mb->GetDouble("RPMaskZ");
    if(mb->CheckTokenExists("RPPivotDist"))
        fRPPivotDist = mb->GetDouble("RPPivotDist");
    if(mb->CheckTokenExists("EnableRPDefaultBeam"))
        fEnableRPDefaultBeam = mb->GetBool("EnableRPDefaultBeam");
    if(mb->CheckTokenExists("RPDefaultMinX"))
        fRPDefaultMinX = mb->GetDouble("RPDefaultMinX");
    if(mb->CheckTokenExists("EnableCylinder"))
        fEnableCylinder = mb->GetBool("EnableCylinder");
    if(mb->CheckTokenExists("CylinderRadius"))
        fCylinderRadius = mb->GetDouble("CylinderRadius");
    // Clean bad fits
    if(mb->CheckTokenExists("EnableCleanBadFits"))
        fEnableCleanBadFits = mb->GetBool("EnableCleanBadFits");

    // Init clocks here
    InitClocks();
}

void ActAlgorithm::MultiStep::InitClocks()
{
    // Declare labels to each clock
    fCLabels = std::vector<std::string>(9);
    fCLabels[0] = "CleanPileup";
    fCLabels[1] = "CleanZs";
    fCLabels[2] = "BreakBeam";
    fCLabels[3] = "BreakTracks";
    fCLabels[4] = "MergeSimilarTracks";
    fCLabels[5] = "CleanDeltas";
    fCLabels[6] = "FindRP";
    fCLabels[7] = "FindPreciseRP";
    fCLabels[8] = "CleanBadFits";
    // Push back clocks to vector
    for(int i = 0, size = fCLabels.size(); i < size; i++)
        fClocks.push_back(TStopwatch());
}

void ActAlgorithm::MultiStep::ResetIndex()
{
    int idx {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++, idx++)
    {
        it->SetClusterID(idx);
    }
}

void ActAlgorithm::MultiStep::PrintStep() const
{
    for(const auto& cluster : *fClusters)
    {
        cluster.Print();
        cluster.GetLine().Print();
    }
}

void ActAlgorithm::MultiStep::Run()
{
    // General disable of algorithm
    if(!fIsEnabled)
        return;
    // Set order of algorithms here, and whether they run or not
    if(fEnableBreakMultiBeam)
    {
        fClocks[2].Start(false);
        BreakBeamClusters();
        fClocks[2].Stop();
        if(fEnableBreakMultiTracks)
        {
            // doesnt make sense to run BreakTracks
            // without attempting first to BreakBeam
            fClocks[3].Start(false);
            BreakTrackClusters();
            fClocks[3].Stop();
        }
    }
    if(fEnableMerge)
    {
        fClocks[4].Start(false);
        MergeSimilarTracks();
        fClocks[4].Stop();
    }

    if(fEnableCleanBadFits)
    {
        fClocks[8].Start(false);
        CleanBadFits();
        fClocks[8].Stop();
    }
    if(fEnableCleanPileUp)
    {
        fClocks[0].Start(false);
        CleanPileup();
        fClocks[0].Stop();
    }
    if(fEnableCleanZs)
    {
        fClocks[1].Start(false);
        CleanZs();
        fClocks[1].Stop();
    }
    if(fEnableCleanDeltas)
    {
        fClocks[5].Start(false);
        CleanDeltas();
        fClocks[5].Stop();
    }
    if(fEnableRPRoutine)
    {
        fClocks[6].Start(false);
        DetermineBeamLikes();
        FindPreliminaryRP();
        fClocks[6].Stop();
        if(fEnableRPDelete)
            DeleteInvalidClusters();
        if(fEnableFineRP)
        {
            fClocks[7].Start(false);
            PerformFinerFits();
            FindPreciseRP();
            fClocks[7].Stop();
        }
    }
    ResetIndex();
}

void ActAlgorithm::MultiStep::PrintReports() const
{
    std::cout << BOLDYELLOW << "==== MultiStep time report ====" << '\n';
    for(int i = 0; i < fCLabels.size(); i++)
    {
        std::cout << "Timer : " << fCLabels[i] << '\n';
        fClocks[i].Print();
    }
    std::cout << RESET << '\n';
}

void ActAlgorithm::MultiStep::CleanBadFits()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        bool isBadFit {std::isnan(it->GetLine().GetDirection().Z())};
        if(isBadFit)
            it = fClusters->erase(it);
        else
            it++;
    }
}

void ActAlgorithm::MultiStep::CleanZs()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1->Check tracks is vertical
        bool isVertical {std::abs(it->GetLine().GetDirection().Unit().Z()) >= fZDirectionThreshold};
        // 2->Check any horizontal range is lower than threshold
        auto [xmin, xmax] {it->GetXRange()};
        auto [ymin, ymax] {it->GetYRange()};
        bool isNarrow {(xmax - xmin) <= fZMinSpanInPlane || (ymax - ymin) <= fZMinSpanInPlane};
        // Delete if yes
        if(isVertical && isNarrow) // for Z particles going upwards in Z
            it = fClusters->erase(it);
        else
            it++;
    }
}

void ActAlgorithm::MultiStep::CleanDeltas()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Check whether cluster has an exceptionally large Chi2
        bool hasLargeChi {it->GetLine().GetChi2() >= fDeltaChi2Threshold};
        // 2-> If has less voxels than required
        bool isSmall {it->GetSizeOfVoxels() <= fDeltaMaxVoxels};
        // 3-> If after all there are clusters with Chi2 = -1
        bool isBadFit {it->GetLine().GetChi2() == -1};
        if(fIsVerbose)
        {
            std::cout << BOLDCYAN << "---- CleanDeltas verbose ----" << '\n';
            std::cout << "Chi2 : " << it->GetLine().GetChi2() << '\n';
            std::cout << "SizeVoxels: " << it->GetSizeOfVoxels() << '\n';
            std::cout << "-------------------" << RESET << '\n';
        }
        if(hasLargeChi || isSmall || isBadFit)
            it = fClusters->erase(it);
        else
            it++;
    }
}

void ActAlgorithm::MultiStep::CleanPileup()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Eval condition of X range
        auto [xmin, xmax] {it->GetXRange()};
        bool spansActarX {(xmax - xmin) > fPileUpXPercent * fTPC->GetNPADSX()};
        // auto [zmin, zmax] {it->GetZRange()};
        // bool isConstantZ {(double)(zmax - zmin) <= fPileUpXPercent};
        // std::cout << "XRange : [" << xmin << ", " << xmax << "]" << '\n';
        // std::cout << "spansActarX ? " << std::boolalpha << spansActarX << '\n';
        // 2-> Get condition: out of beam region
        auto [zmin, zmax] {it->GetZRange()};
        zmin *= fTPC->GetREBINZ();
        zmax *= fTPC->GetREBINZ();
        bool isInBeamZMin {fBeamLowerZ <= zmin && zmin <= fBeamUpperZ};
        bool isInBeamZMax {fBeamLowerZ <= zmax && zmax <= fBeamUpperZ};
        bool isInBeamZ {isInBeamZMin || isInBeamZMax};
        if(spansActarX && !isInBeamZ)
        {
            it = fClusters->erase(it);
        }
        else
            it++;
    }
}

std::tuple<ActAlgorithm::MultiStep::XYZPoint, double, double> ActAlgorithm::MultiStep::DetermineBreakPoint(ItType it)
{
    const auto& xyMap {it->GetXYMap()};
    const auto& xzMap {it->GetXZMap()};
    // Create interval object
    IntervalMap<int> ivsY;
    IntervalMap<int> ivsZ;
    for(const auto& [x, yset] : xyMap)
    {
        ivsY.BuildFromSet(x, yset);
        ivsZ.BuildFromSet(x, xzMap.at(x));
    }
    //
    // std::cout << BOLDCYAN << " ==== Y ==== " << '\n';
    // ivsY.Print();
    // std::cout << " ==== Z ==== " << '\n';
    // ivsZ.Print();
    //
    // Ranges
    auto [xmin, xmax] {it->GetXRange()};
    auto breakY {ivsY.GetKeyAtLength(fBreakLengthThres, 4)};
    // std::cout << "X with Y = " << breakY << '\n';
    auto widthY {ivsY.GetMeanSizeInRange(xmin, breakY)};
    // std::cout << "Width Y = " << widthY << '\n';
    // Widths
    auto breakZ {ivsZ.GetKeyAtLength(fBreakLengthThres, 4)};
    // std::cout << "X with Z = " << breakZ << '\n';
    auto widthZ {ivsZ.GetMeanSizeInRange(xmin, breakZ)};
    // std::cout << "Width Z = " << widthZ << '\n';

    return {XYZPoint(std::min(breakY, breakZ), 0, 0), widthY, widthZ};
}

void ActAlgorithm::MultiStep::BreakBeamClusters()
{
    std::vector<ActRoot::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Check whether we meet conditions to execute this
        bool isBadFit {it->GetLine().GetChi2() > fChi2Threshold};
        auto [xmin, xmax] {it->GetXRange()};
        bool hasMinXExtent {(xmax - xmin) > fMinSpanX};
        auto isBreakable {isBadFit && hasMinXExtent};
        if(!isBreakable)
        {
            it++;
            continue;
        }
        else
        {
            // 2-> Compute gravity point
            auto gravity {it->GetGravityPointInXRange(fLengthXToBreak)};
            // 3-> PRELIMINARY: experimental method to get finer breaking point (a sort of preliminary reaction point)
            // based on cluster topology
            // auto preliminary {DetermineBreakPoint(it)};
            // auto bp {std::get<0>(preliminary)};                  // breaking point
            // auto autoWY {std::get<1>(preliminary)};              // mean width along Y
            // auto autoWZ {std::get<2>(preliminary)};              // mean width along Z
            // bool useBreakingPoint {bp.X() > (xmin + fMinSpanX)}; // since it is very preliminary, does not workk all
            // the
            //                                                      // times, fallback to default method if so

            // 3->Modify original cluster: move non-beam voxels outside to
            //  be clusterized independently
            auto& refToVoxels {it->GetRefToVoxels()};
            auto initSize {refToVoxels.size()};
            auto toMove {std::partition(refToVoxels.begin(), refToVoxels.end(),
                                        [&](const ActRoot::Voxel& voxel)
                                        {
                                            auto pos {voxel.GetPosition()};
                                            pos += XYZVector {0.5, 0.5, 0.5};
                                            // if(useBreakingPoint)
                                            //     return AutoIsInBeam(pos, gravity, (double)bp.X(), autoWY, autoWZ);
                                            // else
                                            //     return ManualIsInBeam(pos, gravity);
                                            return ManualIsInBeam(pos, gravity);
                                        })};
            // Create vector to move to
            std::vector<ActRoot::Voxel> notBeam {};
            notBeam.insert(notBeam.end(), std::make_move_iterator(toMove), std::make_move_iterator(refToVoxels.end()));
            refToVoxels.erase(toMove, refToVoxels.end());

            if(fIsVerbose)
            {
                std::cout << BOLDGREEN << "---- BreakBeam verbose for ID : " << it->GetClusterID() << " ----" << '\n';
                std::cout << "Chi2          = " << it->GetLine().GetChi2() << '\n';
                std::cout << "isBadFit      = " << std::boolalpha << isBadFit << '\n';
                std::cout << "XExtent       = [" << xmin << ", " << xmax << "]" << '\n';
                std::cout << "hasMinXExtent ? " << std::boolalpha << hasMinXExtent << '\n';
                // std::cout << "Break point = " << bp << '\n';
                // std::cout << "Using breaking point ? " << std::boolalpha << useBreakingPoint << '\n';
                std::cout << "Gravity        = " << gravity << '\n';
                std::cout << "Init clusters  = " << fClusters->size() << '\n';
                std::cout << "Init size beam = " << initSize << '\n';
                std::cout << "Remaining beam = " << refToVoxels.size() << '\n';
                std::cout << "Not beam       = " << notBeam.size() << RESET << '\n';
            }
            // Check if satisfies minimum voxel requirement to be clusterized again
            if(refToVoxels.size() <= fAlgo->GetMinPoints())
            {
                // Delete remaining beam-like cluster
                it = fClusters->erase(it);
            }
            else
            {
                // Reprocess
                it->ReFit();
                // Reset ranges
                it->ReFillSets();
                // Set it is beam-like
                it->SetBeamLike(true);
                // And of course, add to iterator
                it++;
            }
            // 4-> Run cluster algorithm again (if asked... should delete this flag)
            std::vector<ActRoot::Cluster> newClusters;
            std::vector<ActRoot::Voxel> noise;
            if(fFitNotBeam)
                std::tie(newClusters, noise) = fAlgo->Run(notBeam);
            // Set flag accordingly
            for(auto& cl : newClusters)
                cl.SetIsBreakBeam(true);
            // Move to vector
            toAppend.insert(toAppend.end(), std::make_move_iterator(newClusters.begin()),
                            std::make_move_iterator(newClusters.end()));
            if(fIsVerbose)
            {
                std::cout << BOLDGREEN << "New clusters   = " << newClusters.size() << RESET << '\n';
                std::cout << "-------------------" << RESET << '\n';
            }
        }
    }
    // Append clusters to original TPCData
    fClusters->insert(fClusters->end(), std::make_move_iterator(toAppend.begin()),
                      std::make_move_iterator(toAppend.end()));
}

void ActAlgorithm::MultiStep::BreakTrackClusters()
{
    std::vector<ActRoot::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        // 1-> Check whether cluster needs breaking
        bool isBadFit {it->GetLine().GetChi2() > fTrackChi2Threshold};
        if(!isBadFit)
        {
            it++;
            continue;
        }
        else
        {
            // Get ref to voxels
            auto& refVoxels {it->GetRefToVoxels()};
            // Identify GP and window scale
            XYZPoint gravity {};
            double scale {};
            if(it->GetIsBreakBeam()) // comes after running BreakBeam
            {
                for(auto in = fClusters->begin(); in != fClusters->end(); in++)
                    if(in->GetIsBeamLike())
                        gravity = in->GetGravityPointInXRange(fLengthXToBreak);
                scale = fBeamWindowScaling;
            }
            else // not broken from beam
            {
                gravity = it->GetLine().GetPoint(); // use fit's gravity point
                scale = 1;
            }
            // Partition to move to newVoxels
            auto toCluster {std::partition(refVoxels.begin(), refVoxels.end(),
                                           [&](const ActRoot::Voxel& voxel)
                                           {
                                               auto pos {voxel.GetPosition()};
                                               pos += XYZVector {0.5, 0.5, 0.5};
                                               return ManualIsInBeam(pos, gravity, scale);
                                           })};
            auto inBeamSize {std::distance(refVoxels.begin(), toCluster)};
            if(inBeamSize > 0)
            {
                std::vector<ActRoot::Voxel> newVoxels;
                newVoxels.insert(newVoxels.end(), std::make_move_iterator(toCluster),
                                 std::make_move_iterator(refVoxels.end()));
                refVoxels.erase(toCluster, refVoxels.end());

                // Reprocess
                std::vector<ActRoot::Cluster> newClusters;
                std::vector<ActRoot::Voxel> noise;
                std::tie(newClusters, noise) = fAlgo->Run(newVoxels);
                // Set not to merge these new ones
                // for(auto& ncl : newClusters)
                //     ncl.SetToMerge(false);
                toAppend.insert(toAppend.end(), std::make_move_iterator(newClusters.begin()),
                                std::make_move_iterator(newClusters.end()));

                // Delete current cluster
                it = fClusters->erase(it);
                // Verbose info
                if(fIsVerbose)
                {
                    std::cout << BOLDCYAN << "---- BreakTrack verbose for ID : " << it->GetClusterID() << " ----"
                              << '\n';
                    std::cout << "Gravity point     : " << gravity << '\n';
                    std::cout << "IsFromBreakBeam   ? " << std::boolalpha << it->GetIsBreakBeam() << '\n';
                    std::cout << "inBeamSize        : " << inBeamSize << '\n';
                    std::cout << "N of new clusters : " << newClusters.size() << '\n';
                    std::cout << "-------------------------" << RESET << '\n';
                }
            }
            else
            {
                // Do not do anything; chi2 >> and will be deleted in CleanDeltas
                // just in case, set flat not to merge
                it->SetToMerge(false);
                it++;
                // Verbose info
                if(fIsVerbose)
                {
                    std::cout << BOLDCYAN << "---- BreakTrack verbose for ID : " << it->GetClusterID() << " ----"
                              << '\n';
                    std::cout << "Gravity point     : " << gravity << '\n';
                    std::cout << "IsFromBreakBeam   ? " << std::boolalpha << it->GetIsBreakBeam() << '\n';
                    std::cout << "Skipping event bc there no voxels inside BeamRegion!" << '\n';
                    std::cout << "-------------------------" << RESET << '\n';
                }
                continue;
            }
        }
    }
    // Write to vector of clusters
    fClusters->insert(fClusters->end(), std::make_move_iterator(toAppend.begin()),
                      std::make_move_iterator(toAppend.end()));
}

void ActAlgorithm::MultiStep::MergeSimilarTracks()
{
    // Sort clusters by increasing voxel size
    std::sort(fClusters->begin(), fClusters->end(),
              [](const ActRoot::Cluster& l, const ActRoot::Cluster& r)
              { return l.GetSizeOfVoxels() < r.GetSizeOfVoxels(); });

    // Set of indexes to delete
    std::set<int, std::greater<int>> toDelete {};
    // Run!
    for(size_t i = 0, isize = fClusters->size(); i < isize; i++)
    {
        // Get clusters as iterators
        auto out {fClusters->begin() + i};
        for(size_t j = 0, jsize = fClusters->size(); j < jsize; j++)
        {
            bool isIinSet {toDelete.find(i) != toDelete.end()};
            bool isJinSet {toDelete.find(j) != toDelete.end()};
            if(i == j || isIinSet || isJinSet) // exclude comparison of same cluster and other already to be deleted
                continue;

            // Get inner iterator
            auto in {fClusters->begin() + j};

            // If any of them is set not to merge, do not do that :)
            if(!out->GetToMerge() || !in->GetToMerge())
                continue;

            // 1-> Compare by distance from gravity point to line!
            auto gravIn {in->GetLine().GetPoint()};
            auto distIn {out->GetLine().DistanceLineToPoint(gravIn)};
            auto gravOut {out->GetLine().GetPoint()};
            auto distOut {in->GetLine().DistanceLineToPoint(gravOut)};
            auto dist {std::max(distIn, distOut)};
            // Get threshold distance to merge
            bool isBelowThresh {dist <= fMergeDistThreshold};
            // auto threshIn {in->GetLine().GetChi2()};
            // auto threshOut {out->GetLine().GetChi2()};
            // auto distThresh {std::max(threshIn, threshOut)};
            // bool isBelowThresh {dist < std::sqrt(distThresh)};
            // std::cout << "<i, j> : <" << i << ", " << j << ">" << '\n';
            // std::cout << "i size : " << out->GetSizeOfVoxels() << " j size : " << in->GetSizeOfVoxels() << '\n';
            // std::cout << "gravOut" << gravOut << " gravIn : " << gravIn << '\n';
            // std::cout << "dist : " << dist << '\n';
            // std::cout << "distThresh: " << distThresh << '\n';
            // std::cout << "------------------" << '\n';

            // 2-> Compare by paralelity
            auto outDir {out->GetLine().GetDirection().Unit()};
            auto inDir {in->GetLine().GetDirection().Unit()};
            bool areParallel {std::abs(outDir.Dot(inDir)) > fMergeMinParallelFactor};
            // std::cout << "Parallel factor : " << std::abs(outDir.Dot(inDir)) << '\n';

            // 3-> Check if fits improves
            if(isBelowThresh && areParallel)
            {
                ActPhysics::Line aux {};
                auto outVoxels {out->GetVoxels()};
                auto inVoxels {in->GetVoxels()};
                outVoxels.reserve(outVoxels.size() + inVoxels.size());
                outVoxels.insert(outVoxels.end(), std::make_move_iterator(inVoxels.begin()),
                                 std::make_move_iterator(inVoxels.end()));
                aux.FitVoxels(outVoxels);
                // Compare Chi2
                auto newChi2 {aux.GetChi2()};
                // oldChi2 is obtained by quadratic sum of chi2s
                auto oldChi2 {std::sqrt(std::pow(out->GetLine().GetChi2(), 2) + std::pow(in->GetLine().GetChi2(), 2))};
                // auto oldChi2 {std::max(out->GetLine().GetChi2(), in->GetLine().GetChi2())};
                bool improvesFit {newChi2 < fMergeChi2CoverageFactor * oldChi2};
                // std::cout << "old chi2 : " << oldChi2 << '\n';
                // std::cout << "new chi2 :  " << newChi2 << '\n';

                if(fIsVerbose)
                {
                    std::cout << BOLDYELLOW << "---- MergeTracks verbose ----" << '\n';
                    std::cout << "for <i,j> : <" << i << ", " << j << ">" << '\n';
                    std::cout << "i size : " << out->GetSizeOfVoxels() << " j size : " << in->GetSizeOfVoxels() << '\n';
                    std::cout << "dist < distThresh ? : " << dist << " < " << fMergeDistThreshold << '\n';
                    std::cout << "are parallel ? : " << std::boolalpha << areParallel << '\n';
                    std::cout << "newChi2 < f * oldChi2 ? : " << newChi2 << " < " << fMergeChi2CoverageFactor * oldChi2
                              << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                }

                // Then, move and erase in iterator!
                if(improvesFit)
                {
                    if(fIsVerbose)
                    {
                        std::cout << BOLDYELLOW << "------------------------------" << '\n';
                        std::cout << "-> merging cluster : " << in->GetClusterID()
                                  << " with size : " << in->GetSizeOfVoxels() << '\n';
                        std::cout << " with cluster out : " << out->GetClusterID()
                                  << " and size : " << out->GetSizeOfVoxels() << '\n';
                        std::cout << "------------------------------" << RESET << '\n';
                    }
                    auto& refVoxels {out->GetRefToVoxels()};
                    refVoxels.insert(refVoxels.end(), std::make_move_iterator(inVoxels.begin()),
                                     std::make_move_iterator(inVoxels.end()));
                    // Refit and recompute ranges
                    out->ReFit();
                    out->ReFillSets();
                    // Mark to delete afterwards!
                    toDelete.insert(j);
                }
            }
        }
    }
    // Indeed delete
    for(const auto& idx : toDelete) // toDelete is sorted in greater order
        fClusters->erase(fClusters->begin() + idx);
}

bool ActAlgorithm::MultiStep::ManualIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, double scale)
{
    bool condY {(gravity.Y() - scale * fBeamWindowY) < pos.Y() && pos.Y() < (gravity.Y() + scale * fBeamWindowY)};
    bool condZ {(gravity.Z() - scale * fBeamWindowZ) < pos.Z() && pos.Z() < (gravity.Z() + scale * fBeamWindowZ)};
    return condY && condZ;
}

template <typename T>
bool ActAlgorithm::MultiStep::AutoIsInBeam(const XYZPoint& pos, const XYZPoint& gravity, T xBreak, T widthY, T widthZ,
                                         T offset)
{
    bool condX {pos.X() < xBreak + offset};
    bool condY {(gravity.Y() - widthY) <= pos.Y() && pos.Y() <= (gravity.Y() + widthY)};
    bool condZ {(gravity.Z() - widthZ) <= pos.Z() && pos.Z() <= (gravity.Z() + widthZ)};
    return condX && condY && condZ;
}

void ActAlgorithm::MultiStep::DetermineBeamLikes()
{
    int nBeam {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // 1-> Check if XRange constitutes an important amount of XLength of ACTAR
        auto [xmin, xmax] {it->GetXRange()};
        // bool isLongEnough {(xmax - xmin) >= fBeamLikeXMinThresh * fTPC->GetNPADSX()};
        // 1-> Check xMin is below threshold
        bool isInEntrance {xmin <= fBeamLikeXMinThresh};
        // 2-> Is mainly along X direction
        auto uDir {it->GetLine().GetDirection().Unit()};
        bool isAlongX {TMath::Abs(uDir.X()) >= fBeamLikeParallelF};
        // 3-> Tag it as beam-like and count
        if(isInEntrance && isAlongX)
        {
            it->SetBeamLike(true);
            nBeam++;
        }
    }
    // Print
    if(fIsVerbose)
    {
        std::cout << BOLDYELLOW << "---- Beam-Like ID verbose ----" << '\n';
        std::cout << "-> N beam clusters  : " << nBeam << '\n';
        std::cout << "-> N total clusters : " << fClusters->size() << '\n';
        std::cout << "-------------------------" << RESET << '\n';
    }
}

std::tuple<ActAlgorithm::MultiStep::XYZPoint, ActAlgorithm::MultiStep::XYZPoint, double>
ActAlgorithm::MultiStep::ComputeRPIn3D(XYZPoint pA, XYZVector vA, XYZPoint pB, XYZVector vB)
{
    // Using https://math.stackexchange.com/questions/1993953/closest-points-between-two-lines/3334866#3334866
    // 1-> Normalize all directions
    vA = vA.Unit();
    vB = vB.Unit();
    // 2-> Get cross product and normalize it
    auto vC {vB.Cross(vA)};
    vC = vC.Unit();
    // If lines are parallel, skip them
    if(ActRoot::IsEqZero(vC.R()))
        return {pA, pB, -1};
    // 3-> Matrices to solve system of equations in Math StackExchange
    TMatrixD left {3, 3}; // 3x3 matrix with double precision
    // Fill left matrix with columns as each ABC vector
    XYZVector vecs[3] {vA, -vB, vC};
    for(int col = 0; col < 3; col++)
    {
        double components[3] {};
        vecs[col].GetCoordinates(components);
        for(int row = 0; row < 3; row++)
            left[row][col] = components[row];
    }
    TMatrixD right {3, 1};
    auto diff {pB - pA};
    double components[3] {};
    diff.GetCoordinates(components);
    right.SetMatrixArray(components);
    // 4-> Invert left to solve system
    TMatrixD invLeft {TMatrixD::kInverted, left};
    // 5-> Solve system of linear eqs
    auto res {TMatrixD(invLeft, TMatrixD::kMult, right)};
    // 6-> Return results {point in A, point in B, distance AB}
    return {pA + res[0][0] * vA, pB + res[1][0] * vB, TMath::Abs(res[2][0])};
}

bool ActAlgorithm::MultiStep::IsRPValid(const XYZPoint& rp)
{
    // This function has to consider the 0.5 offset
    bool isInX {0.5 <= rp.X() && rp.X() <= fTPC->GetNPADSX() + 0.5};
    bool isInY {0.5 <= rp.Y() && rp.Y() <= fTPC->GetNPADSY() + 0.5};
    bool isInZ {0.5 <= rp.Z() && rp.Z() <= (double)fTPC->GetNPADSZ() / fTPC->GetREBINZ() + 0.5};
    return isInX && isInY && isInZ;
}

double ActAlgorithm::MultiStep::GetThetaAngle(const XYZVector& beam, const XYZVector& recoil)
{
    auto dot {beam.Unit().Dot((recoil.Unit()))};
    return TMath::ACos(dot) * TMath::RadToDeg();
}

std::vector<ActAlgorithm::MultiStep::RPCluster> ActAlgorithm::MultiStep::ClusterAndSortRPs(std::vector<RPValue>& rps)
{
    std::vector<RPCluster> ret;
    if(rps.empty())
        return ret;
    if(rps.size() == 1)
    {
        // does not require any further processing
        ret.push_back({rps.front().first, {rps.front().second.first, rps.front().second.second}});
        return ret;
    }
    // Sort
    auto cmp {[](const RPValue& vl, const RPValue& vr)
              {
                  const auto& l {vl.first};
                  const auto& r {vr.first};
                  if(l.X() != r.X())
                      return l.X() < r.X();
                  if(l.Y() != r.Y())
                      return l.Y() < r.Y();
                  return l.Z() < r.Z();
              }};
    std::sort(rps.begin(), rps.end(), cmp);
    // Cluster
    std::vector<std::vector<RPValue>> clusters;
    for(auto it = rps.begin();;)
    {
        auto last {std::adjacent_find(it, rps.end(),
                                      [&](const RPValue& l, const RPValue& r)
                                      { return (l.first - r.first).R() > fRPDistCluster; })};
        if(last == rps.end())
        {
            clusters.emplace_back(it, last);
            break;
            ;
        }

        // Get next-to-last iterator
        auto gap {std::next(last)};

        // Push back and continue
        clusters.emplace_back(it, gap);

        // Prepare for next iteration
        it = gap;
    }
    // Sort by size of cluster
    std::sort(clusters.begin(), clusters.end(),
              [](const std::vector<RPValue>& l, const std::vector<RPValue>& r) { return l.size() > r.size(); });

    // Get mean of clusters
    for(const auto& cluster : clusters)
    {
        std::set<int> set {};
        XYZPoint mean {};
        for(const auto& [rp, idxs] : cluster)
        {
            // std::cout << "Proc RP : " << rp << " with idxs : " << '\n';
            // std::cout << "first : " << idxs.first << " second : " << idxs.second << '\n';
            mean += XYZVector {rp};
            set.insert(idxs.first);
            set.insert(idxs.second);
        }
        // std::cout << "============ new cluster =============" << '\n';
        mean /= cluster.size();
        ret.push_back({mean, set});
    }
    // Validate once again using distance
    auto distvalid = [this](const RPCluster& cluster)
    {
        int count {};
        for(const auto& idx : cluster.second)
        {
            auto it {fClusters->begin() + idx};
            if(it->GetIsBeamLike())
                continue;
            // Sort voxels
            std::sort(it->GetRefToVoxels().begin(), it->GetRefToVoxels().end());
            // Min and max
            auto min {it->GetVoxels().front().GetPosition()};
            auto max {it->GetRefToVoxels().back().GetPosition()};
            for(const auto& p : {min, max})
            {
                auto dist {(p - cluster.first).R()};
                // std::cout << " p    : " << p << '\n';
                // std::cout << " rp   : " << cluster.first << '\n';
                // std::cout << " dist : " << dist << '\n';
                // std::cout << "---------------" << '\n';
                if(dist < fRPDistValidate)
                    count++;
            }
        }
        return count;
    };
    // Call to sort
    std::sort(ret.begin(), ret.end(),
              [this, &distvalid](const RPCluster& l, const RPCluster& r)
              {
                  auto lc {distvalid(l)};
                  auto rc {distvalid(r)};
                  return lc > rc;
              });
    return ret;
}

void ActAlgorithm::MultiStep::FindPreliminaryRP()
{
    // Case in which there is only one track: automatically mark to delete
    if(fClusters->size() == 1)
        fClusters->begin()->SetToDelete(true);

    typedef std::vector<std::pair<XYZPoint, std::pair<int, int>>> RPVector; // includes RP and pair of indexes as values
    // Declare vector of RPs
    RPVector rps;
    // Run
    for(int i = 0, size = fClusters->size(); i < size; i++)
    {
        // Get clusters as iterators
        auto out {fClusters->begin() + i};
        for(int j = i + 1; j < size; j++)
        {
            // Get clusters as iterators
            auto in {fClusters->begin() + j};

            // If both are BL, continue
            if(out->GetIsBeamLike() && in->GetIsBeamLike())
                continue;

            // Run function
            auto [pA, pB, dist] {ComputeRPIn3D(out->GetLine().GetPoint(), out->GetLine().GetDirection(),
                                               in->GetLine().GetPoint(), in->GetLine().GetDirection())};
            if(dist < 0) // just in case lines were parallel
                continue;
            // Build RP as mean of A and B
            XYZPoint rp {(pA.X() + pB.X()) / 2, (pA.Y() + pB.Y()) / 2, (pA.Z() + pB.Z()) / 2};

            // Check that all points are valid
            bool checkA {IsRPValid(pA)};
            bool checkB {IsRPValid(pB)};
            bool checkRP {IsRPValid(rp)};
            auto checkPoints {checkA && checkB && checkRP};
            // And finally that distance AB is bellow threshold
            bool checkDist {dist <= fRPDistThresh};
            // std::cout << " -- RP conditionals --" << '\n';
            // std::cout << "-> checkPoints ? " << std::boolalpha << checkPoints << '\n';
            // std::cout << "-> checkDist ? " << std::boolalpha << checkDist << '\n';
            // std::cout << "-> rp : " << rp << '\n';
            // std::cout << "===========" << '\n';
            if(checkPoints && checkDist)
                rps.push_back({rp, {i, j}});
            else
                continue;
            // we indeed confirmed that distance is correctly calculated in ComputeRPin3D
            // auto manual {TMath::Sqrt(TMath::Power(pA.X() - pB.X(), 2) + TMath::Power(pA.Y() - pB.Y(), 2) +
            // TMath::Power(pA.Z() - pB.Z(), 2))}; std::cout<<"Manual dist = "<<manual<<'\n'; std::cout<<"Auto dist =
            // "<<dist<<'\n';
        }
    }
    // Debug new RPs
    // std::cout << "RPs before processing" << '\n';
    // for(const auto& rp : rps)
    // {
    //     std::cout << "RP : " << rp.first << " at i,j : " << rp.second.first << ", " << rp.second.second << '\n';
    // }
    // std::cout << "---------------------------" << '\n';
    auto proc {ClusterAndSortRPs(rps)};
    // for(const auto& [rp, set] : proc)
    // {
    //     std::cout << "RP : " << rp << " with set size : " << set.size() << '\n';
    // }
    std::set<int> toKeep {};
    fRPs->clear();
    if(proc.size() > 0)
    {
        // Set RP as the one with the biggest number
        // of cluster within distance
        fRPs->push_back(proc.front().first);
        // Marks its tracks to be kept
        toKeep = proc.front().second;
    }
    // // Process RP: delete outliers
    // if(rps.size() > 1)
    // {
    //     for(auto out = rps.begin(); out != rps.end();)
    //     {
    //         bool isInlier {false};
    //         for(auto in = rps.begin(); in != rps.end(); in++)
    //         {
    //             if(in == out)
    //                 continue;
    //             auto outRP {out->first};
    //             auto inRP {in->first};
    //             auto dist {(outRP - inRP).R()};
    //             std::cout << "dist : " << dist << '\n';
    //             if(dist < fRPDistValidate)
    //             {
    //                 isInlier = true;
    //                 break;
    //             }
    //         }
    //         if(!isInlier && rps.size() > 1)
    //             out = rps.erase(out);
    //         else
    //             out++;
    //     }
    // }
    //
    // // Add indexes of clusters to keep
    // std::set<int, std::greater<int>> toKeep;
    // for(const auto& pair : rps)
    // {
    //     auto [_, idxs] {pair};
    //     toKeep.insert(idxs.first);
    //     toKeep.insert(idxs.second);
    // }
    // This method ensures always a RP vector with size > 1 always!
    for(int i = 0, size = fClusters->size(); i < size; i++)
    {
        auto it {fClusters->begin() + i};
        auto isToKeep {toKeep.find(i) != toKeep.end()};
        if(isToKeep)
            it->SetToDelete(false);
        else
            it->SetToDelete(true);
    }
    // // Compute mean of RPs
    // XYZPoint rp {};
    // for(const auto& pair : rps)
    // {
    //     auto [p, _] {pair};
    //     rp += XYZVector {p};
    // }
    // rp /= rps.size();
    // fRPs->clear();
    // fRPs->push_back(rp);
}

void ActAlgorithm::MultiStep::DeleteInvalidClusters()
{
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        if(it->GetToDelete())
            it = fClusters->erase(it);
        else
            it++;
    }
}

void ActAlgorithm::MultiStep::PerformFinerFits()
{
    // Must be executed after cleaning of invalid clusters
    // 1-> Get the unique RP (for legacy reasons it is still kept as a vector)
    if(fRPs->size() == 0)
        return;
    const auto& rp {fRPs->front()};

    // 2-> Break BL starting on RP
    std::vector<ActRoot::Cluster> toAppend {};
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        if(it->GetIsBeamLike())
        {
            auto& refVoxels {it->GetRefToVoxels()};
            auto rpBreak {std::partition(refVoxels.begin(), refVoxels.end(),
                                         [&](const ActRoot::Voxel& voxel)
                                         {
                                             auto pos {voxel.GetPosition()};
                                             pos += XYZVector {0.5, 0.5, 0.5};
                                             return (pos.X() < rp.X());
                                         })};
            // Move
            std::vector<ActRoot::Voxel> newVoxels;
            newVoxels.insert(newVoxels.end(), std::make_move_iterator(rpBreak),
                             std::make_move_iterator(refVoxels.end()));
            refVoxels.erase(rpBreak, refVoxels.end());
            // Add to cluster
            ActRoot::Cluster newCluster {(int)fClusters->size()};
            newCluster.SetVoxels(std::move(newVoxels));
            newCluster.ReFit();
            newCluster.ReFillSets();
            newCluster.SetIsSplitRP(true);
            toAppend.push_back(std::move(newCluster));

            if(fIsVerbose)
            {
                std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose ----" << '\n';
                std::cout << "What: Splitting BL cluster after RP" << '\n';
                std::cout << "-----------------------------" << RESET << '\n';
            }

            // Refit remanining voxels
            it->ReFit();
            it->ReFillSets();
        }
    }
    fClusters->insert(fClusters->end(), std::make_move_iterator(toAppend.begin()),
                      std::make_move_iterator(toAppend.end()));

    // 3-> Delete clusters with less than ClIMB min points or with fBLMinVoxels if BL
    for(auto it = fClusters->begin(); it != fClusters->end();)
    {
        if(it->GetIsBeamLike())
        {
            if(it->GetSizeOfVoxels() <= fBeamLikeMinVoxels)
            {
                if(fIsVerbose)
                {
                    std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose ----" << '\n';
                    std::cout << "What: Deleting RP with fVoxels.size() < fBLMinVoxels" << '\n';
                    std::cout << "-----------------------------" << RESET << '\n';
                }
                it = fClusters->erase(it);
            }
            else
                it++;
        }
        else
        {
            if(it->GetSizeOfVoxels() <= fAlgo->GetMinPoints())
                it = fClusters->erase(it);
            else
                it++;
        }
    }

    // 4 ->Mask region around RP
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        auto& refVoxels {it->GetRefToVoxels()};
        auto toKeep {std::partition(refVoxels.begin(), refVoxels.end(),
                                    [&](const ActRoot::Voxel& voxel)
                                    {
                                        auto pos {voxel.GetPosition()};
                                        pos += XYZVector {0.5, 0.5, 0.5};
                                        bool condX {std::abs(pos.X() - rp.X()) <= fRPMaskXY};
                                        bool condY {std::abs(pos.Y() - rp.Y()) <= fRPMaskXY};
                                        bool condZ {std::abs(pos.Z() - rp.Z()) <= fRPMaskZ};
                                        return !(condX && condY && condZ);
                                    })};
        // Compare sizes
        auto remainSize {std::distance(refVoxels.begin(), toKeep)};
        // Indeed delete region and refit
        if(remainSize > fAlgo->GetMinPoints())
        {
            refVoxels.erase(toKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets();
            if(fIsVerbose)
            {
                std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose for ID: " << it->GetClusterID() << '\n';
                std::cout << "What: masking region around RP" << '\n';
                std::cout << "------------------------------" << RESET << '\n';
            }
        }
    }

    // 5-> Clean voxels outside cylinder
    if(fEnableCylinder)
    {
        for(auto it = fClusters->begin(); it != fClusters->end(); it++)
        {
            auto& refVoxels {it->GetRefToVoxels()};
            auto oldSize {refVoxels.size()};
            auto itKeep {std::partition(refVoxels.begin(), refVoxels.end(),
                                        [&](const ActRoot::Voxel& voxel)
                                        {
                                            auto pos {voxel.GetPosition()};
                                            pos += XYZVector {0.5, 0.5, 0.5};
                                            auto dist {it->GetLine().DistanceLineToPoint(pos)};
                                            // auto proj {it->GetLine().ProjectionPointOnLine(pos)};
                                            // auto distX {std::abs(proj.X() - pos.X())};
                                            // auto distY {std::abs(proj.Y() - pos.Y())};
                                            // auto distZ {std::abs(proj.Z() - pos.Z())};
                                            // std::cout << "Proj : " << proj << '\n';
                                            // std::cout << "Pos  : " << pos << '\n';
                                            // std::cout << "dX : " << distX << " dY : " << distY << " dZ : " << distZ
                                            //           << '\n';
                                            // std::cout << "Overall dist : " << dist << '\n';
                                            // std::cout << "--------------------" << '\n';
                                            return dist <= fCylinderRadius;
                                        })};
            // if enough voxels remain
            auto remain {std::distance(refVoxels.begin(), itKeep)};
            if(remain > fAlgo->GetMinPoints())
            {
                refVoxels.erase(itKeep, refVoxels.end());
                it->ReFit();
                it->ReFillSets();
                if(fIsVerbose)
                {
                    std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose for ID: " << it->GetClusterID() << '\n';
                    std::cout << "What: cleaning cylinder" << '\n';
                    std::cout << "(Old - New) sizes : " << (oldSize - remain) << '\n';
                    std::cout << "------------------------------" << RESET << '\n';
                }
            }
        }
    }


    // 6-> Mask region at begining and end of tracks
    for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    {
        // Declare variables
        const auto& line {it->GetLine()};
        const auto& gp {line.GetPoint()};
        auto& refVoxels {it->GetRefToVoxels()};
        auto oldSize {refVoxels.size()};
        // Sort them
        std::sort(refVoxels.begin(), refVoxels.end());
        // Set same sign as rp
        it->GetRefToLine().AlignUsingPoint(rp);
        // Get init point
        auto init {refVoxels.front()};
        // Get end point
        auto end {refVoxels.back()};
        //// PROJECTIONS ON LINE, relative to GP of line (correct by +0.5 offset)
        auto projInit {line.ProjectionPointOnLine(init.GetPosition() + XYZVector {0.5, 0.5, 0.5})};
        auto projEnd {line.ProjectionPointOnLine(end.GetPosition() + XYZVector {0.5, 0.5, 0.5})};
        // Get pivot points, according to position respect to RP
        // auto pivotInit {projInit + fRPPivotDist * it->GetLine().GetDirection().Unit()};
        // auto pivotEnd {projEnd - fRPPivotDist * it->GetLine().GetDirection().Unit()};
        // Get iterator to last element to be kept
        auto itKeep {
            std::partition(refVoxels.begin(), refVoxels.end(),
                           [&](const ActRoot::Voxel& voxel)
                           {
                               auto pos {voxel.GetPosition()};
                               pos += XYZVector {0.5, 0.5, 0.5};
                               auto proj {line.ProjectionPointOnLine(pos)};
                               // delete all points over projInit/end
                               // bc due to ordering and angle, some voxel could have a proj larger than
                               // the one of the last/first voxel
                               // TODO: check a better way to mask outling voxels (proj.X() < projInit.X() could)
                               // be troublesome depending on track angle
                               bool isInCapInit {(proj - projInit).R() <= fRPPivotDist || (proj.X() < projInit.X())};
                               bool isInCapEnd {(proj - projEnd).R() <= fRPPivotDist || (proj.X() > projEnd.X())};
                               // if(it->GetIsBeamLike())
                               // {
                               //     std::cout << "Proj : " << proj << '\n';
                               //     std::cout << "isInCapInit : " << std::boolalpha << isInCapInit << '\n';
                               //     std::cout << "isInCapEnd : " << std::boolalpha << isInCapEnd << '\n';
                               //     std::cout << "--------------------" << '\n';
                               // }
                               return !(isInCapInit || isInCapEnd);
                           })};
        auto newSize {std::distance(refVoxels.begin(), itKeep)};
        // Refit if enough voxels remain
        if(newSize > fAlgo->GetMinPoints())
        {
            refVoxels.erase(itKeep, refVoxels.end());
            it->ReFit();
            it->ReFillSets();
        }
        //  Print
        if(fIsVerbose)
        {
            std::cout << BOLDMAGENTA << "--- FindPreciseRP verbose for ID : " << it->GetClusterID() << " ----" << '\n';
            std::cout << "Init : " << init.GetPosition() << '\n';
            std::cout << "Proj Init : " << projInit << '\n';
            std::cout << "End : " << end.GetPosition() << '\n';
            std::cout << "Proj End : " << projEnd << '\n';
            std::cout << "(Old - New) sizes : " << (oldSize - refVoxels.size()) << '\n';
            std::cout << "Gravity point : " << it->GetLine().GetPoint() << '\n';
            std::cout << "------------------------------" << RESET << '\n';
            // it->GetLine().Print();
        }
    }

    // 7-> Set default fit for BLs
    if(fEnableRPDefaultBeam)
    {
        for(auto it = fClusters->begin(); it != fClusters->end(); it++)
        {
            if(it->GetIsBeamLike())
            {
                auto [xmin, xmax] {it->GetXRange()};
                bool isShort {(xmax - xmin) <= fRPDefaultMinX};
                if(isShort)
                {
                    it->GetRefToLine().SetDirection({1, 0, 0});
                    // std::cout << "Before dir : " << it->GetLine().GetDirection().Unit() << '\n';
                    // it->GetRefToLine().FitVoxels(it->GetVoxels(), false);
                    // std::cout << "After disabling charge : " << it->GetLine().GetDirection().Unit() << '\n';
                    if(fIsVerbose)
                    {
                        std::cout << BOLDMAGENTA << "---- FindPreciseRP verbose for ID: " << it->GetClusterID() << '\n';
                        std::cout << "What: short beam : setting (1, 0, 0) direction" << '\n';
                        std::cout << "------------------------------" << RESET << '\n';
                    }
                }
            }
        }
    }

    // // 7-> Merge clusters which got split due to fineRP
    // for(auto in = fClusters->begin(); in != fClusters->end(); in++)
    // {
    //     if(in->GetIsSplitRP())
    //     {
    //         for(auto other = fClusters->begin(); other != fClusters->end(); other++)
    //         {
    //             if(in == other || other->GetIsBeamLike())
    //                 continue;
    //             ClustersOverlap(other, in);
    //         }
    //         break;
    //     }
    // }
    // 7 -> Delete split heavy if there are more than 1 recoil track
    // bool hasBL {};
    // bool hasSplitHeavy {};
    // ItType itSplitHeavy {};
    // for(auto it = fClusters->begin(); it != fClusters->end(); it++)
    // {
    //     if(it->GetIsBeamLike())
    //         hasBL = true;
    //     if(it->GetIsSplitRP())
    //     {
    //         hasSplitHeavy = true;
    //         itSplitHeavy = it;
    //     }
    // }
    // if(hasSplitHeavy && hasBL)
    // {
    //     int count {(int)fClusters->size()};
    //     if(std::abs(count - 2) != 1)
    //         fClusters->erase(itSplitHeavy);
    // }
}

bool ActAlgorithm::MultiStep::ClustersOverlap(ItType out, ItType in)
{
    // Interval xout {out->GetXRange()};
    // std::cout << "Xout : " << xout << '\n';
    // Interval xin {in->GetXRange()};
    // std::cout << "Xin : " << xin << '\n';
    // bool condX {xout.Overlaps(xin)};
    //
    // Interval yout {out->GetYRange()};
    // Interval yin {in->GetYRange()};
    // bool condY {yout.Overlaps(yin)};
    //
    // Interval zout {out->GetZRange()};
    // Interval zin {in->GetZRange()};
    // bool condZ {zout.Overlaps(zin)};
    //
    // return condX && (condY && condZ);
    // Sort voxels
    auto& outVoxels {out->GetRefToVoxels()};
    std::sort(outVoxels.begin(), outVoxels.end());
    auto& inVoxels {in->GetRefToVoxels()};
    std::sort(inVoxels.begin(), inVoxels.end());
    // Compare in.back() with out.front()
    auto back {inVoxels.back().GetPosition()};
    auto front {outVoxels.front().GetPosition()};

    auto dist {(front - back).R()};
    std::cout << "Back : " << back << '\n';
    std::cout << "Front : " << front << '\n';
    std::cout << "dist : " << dist << '\n';
    std::cout << "----------------" << '\n';
    return false;
}


void ActAlgorithm::MultiStep::FindPreciseRP()
{
    if(fRPs->size() == 0)
        return;
    // Precise RP is found by intersection of a BL cluster with the track with larger angle
    // We sort them in this way using a set
    typedef std::pair<double, XYZPoint> SetValue;
    auto lambda {[](const SetValue& l, const SetValue& r) { return l.first > r.first; }};
    std::set<SetValue, decltype(lambda)> set {lambda};
    for(auto out = fClusters->begin(); out != fClusters->end(); out++)
    {
        // Find a BL cluster
        if(out->GetIsBeamLike())
        {
            // Ensure direction of BL is always positive along X
            auto& outLine {out->GetRefToLine()};
            const auto& oldDir {outLine.GetDirection()};
            outLine.SetDirection({std::abs(oldDir.X()), oldDir.Y(), oldDir.Z()});

            for(auto in = fClusters->begin(); in != fClusters->end(); in++)
            {
                if(in == out)
                    continue;
                // Ensure direction signs are defined from preliminary RP
                in->GetRefToLine().AlignUsingPoint(fRPs->front());
                // Compute angle theta
                auto theta {GetThetaAngle(out->GetLine().GetDirection(), in->GetLine().GetDirection())};
                // Compute RPs
                auto [a, b, dist] {ComputeRPIn3D(out->GetLine().GetPoint(), out->GetLine().GetDirection(),
                                                 in->GetLine().GetPoint(), in->GetLine().GetDirection())};
                if(dist < 0) // just in case lines were parallel
                    continue;
                XYZPoint rp {(a.X() + b.X()) / 2, (a.Y() + b.Y()) / 2, (a.Z() + b.Z()) / 2};
                // Check that all points are valid
                bool checkA {IsRPValid(a)};
                bool checkB {IsRPValid(b)};
                bool checkRP {IsRPValid(rp)};
                auto checkPoints {checkA && checkB && checkRP};
                // std::cout << "RP of " << out->GetClusterID() << " with " << in->GetClusterID() << '\n';
                // std::cout << "at angle : " << theta << " is : " << rp << '\n';
                // And finally that distance AB is bellow threshold
                bool checkDist {dist <= fRPDistThresh};
                if(checkPoints)
                    set.insert({std::abs(theta), rp});
            }
        }
    }
    // // Print
    // for(const auto& pair : set)
    //     std::cout << "FineRP : " << pair.second << " theta : " << pair.first << '\n';
    // Write
    if(set.size() > 0)
    {
        fRPs->clear();
        fRPs->push_back(set.begin()->second);
    }
    else
        ; // keep preliminary RP just in case this finer method fails
}

void ActAlgorithm::MultiStep::Print() const
{
    std::cout << BOLDCYAN << "==== MultiStep settings ====" << '\n';
    std::cout << "-> IsEnabled        : " << std::boolalpha << fIsEnabled << '\n';
    std::cout << "-> IsVerbose        : " << std::boolalpha << fIsVerbose << '\n';
    std::cout << "-----------------------" << '\n';
    if(fIsEnabled)
    {
        if(fEnableBreakMultiBeam)
        {
            std::cout << "-> FitNotBeam       : " << std::boolalpha << fFitNotBeam << '\n';
            std::cout << "-> Chi2 thresh      : " << fChi2Threshold << '\n';
            std::cout << "-> MinSpanX         : " << fMinSpanX << '\n';
            std::cout << "-> LengthXToBreak   : " << fLengthXToBreak << '\n';
            std::cout << "-> BeamWindowY      : " << fBeamWindowY << '\n';
            std::cout << "-> BeamWindowZ      : " << fBeamWindowZ << '\n';
            std::cout << "-> BreakLenThresh   : " << fBreakLengthThres << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableBreakMultiTracks)
        {
            std::cout << "-> TrackChi2Thresh  : " << fTrackChi2Threshold << '\n';
            std::cout << "-> BeamWindowScale  : " << fBeamWindowScaling << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableMerge)
        {
            std::cout << "-> MergeMinParallel : " << fMergeMinParallelFactor << '\n';
            std::cout << "-> MergeChi2CoverF  : " << fMergeChi2CoverageFactor << '\n';
            std::cout << "-> MergeDistThresh  : " << fMergeDistThreshold << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableCleanPileUp)
        {
            std::cout << "-> PileUpChangeZ    : " << fPileUpXPercent << '\n';
            std::cout << "-> BeamLowerZ       : " << fBeamLowerZ << '\n';
            std::cout << "-> BeamUpperZ       : " << fBeamUpperZ << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableCleanZs)
        {
            std::cout << "-> ZDirectionThresh : " << fZDirectionThreshold << '\n';
            std::cout << "-> ZMinSpanInPlane  : " << fZMinSpanInPlane << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableCleanDeltas)
        {
            std::cout << "-> DeltaChi2Thresh  : " << fDeltaChi2Threshold << '\n';
            std::cout << "-> DeltaMaxVoxels   : " << fDeltaMaxVoxels << '\n';
            std::cout << "-----------------------" << '\n';
        }
        if(fEnableRPRoutine)
        {
            std::cout << "-> BeamLikeXMin     : " << fBeamLikeXMinThresh << '\n';
            std::cout << "-> BeamLikeParall   : " << fBeamLikeParallelF << '\n';
            std::cout << "-> BeamLikeMinVox   : " << fBeamLikeMinVoxels << '\n';
            std::cout << "-> EnableRPDelete   ? " << std::boolalpha << fEnableRPDelete << '\n';
            std::cout << "-> RPDistThresh     : " << fRPDistThresh << '\n';
            std::cout << "-> RPDistCluster    : " << fRPDistCluster << '\n';
            std::cout << "-> RPDistValidate   : " << fRPDistValidate << '\n';
            std::cout << "-> EnableFineRP     ? " << std::boolalpha << fEnableFineRP << '\n';
            std::cout << "-> RPMaskXY         : " << fRPMaskXY << '\n';
            std::cout << "-> RPMaskZ          : " << fRPMaskZ << '\n';
            std::cout << "-> RPPivotDist      : " << fRPPivotDist << '\n';
            std::cout << "-> EnableDefaultBL  : " << std::boolalpha << fEnableRPDefaultBeam << '\n';
            std::cout << "-> RPDefaultMinX    : " << fRPDefaultMinX << '\n';
            std::cout << "-> EnableCylinder   : " << std::boolalpha << fEnableCylinder << '\n';
            std::cout << "-> CylinderRadius   : " << fCylinderRadius << '\n';
        }
    }
    std::cout << "=======================" << RESET << '\n';
}
