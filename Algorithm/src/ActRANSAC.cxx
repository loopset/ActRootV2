#include "ActRANSAC.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActLine.h"
#include "ActOptions.h"
#include "ActVCluster.h"
#include "ActVoxel.h"

#include "TMath.h"
#include "TRandom.h"

#include <ios>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

ActAlgorithm::RANSAC::RANSAC(int iterations, int minPoints, double distThres)
    : VCluster(minPoints),
      fIterations(iterations),
      fDistThreshold(distThres)
{
}

void ActAlgorithm::RANSAC::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "ransac.conf";
    // Parse!
    ActRoot::InputParser parser {conf};
    auto rb {parser.GetBlock("Ransac")};
    if(rb->CheckTokenExists("DistThreshold"))
        fDistThreshold = rb->GetDouble("DistThreshold");
    if(rb->CheckTokenExists("MinPoints"))
        fMinPoints = rb->GetInt("MinPoints");
    if(rb->CheckTokenExists("Iterations"))
        fIterations = rb->GetInt("Iterations");
    if(rb->CheckTokenExists("UseLmeds", true))
        fUseLmeds = rb->GetBool("UseLmeds");
}

int ActAlgorithm::RANSAC::GetNInliers(const std::vector<ActRoot::Voxel>& voxels, ActPhysics::Line& line)
{
    int ninliers {};
    std::vector<double> verrors;
    for(const auto& voxel : voxels)
    {
        const auto& pos = voxel.GetPosition();
        double err = line.DistanceLineToPoint(pos);
        err *= err;
        if(err < (fDistThreshold * fDistThreshold))
        {
            verrors.push_back(err);
            ninliers++;
        }
    }
    // Naive implementation of other estimators simply changing the test value
    if(fUseLmeds)
    {
        double weight {TMath::Median(verrors.size(), &(verrors[0]))};
        line.SetChi2(weight / ninliers);
    }
    else
        line.SetChi2(1. / ninliers);
    return ninliers;
}

std::vector<ActRoot::Voxel>
ActAlgorithm::RANSAC::ProcessCloud(std::vector<ActRoot::Voxel>& remain, const ActPhysics::Line& line)
{
    std::vector<ActRoot::Voxel> ret {};
    // clear cloud according to line: delete from it voxel belonging to this line
    auto itrange {remain.end()}; // to work with ranges of voxels, using insert range of iterator capabilities!
    for(auto it = remain.begin(); it != remain.end(); ++it)
    {
        // Compute error
        double err {line.DistanceLineToPoint(it->GetPosition())};
        bool isInLine {(err * err) < (fDistThreshold * fDistThreshold)};

        // In line
        if(isInLine && (itrange == remain.end()))
        {
            itrange = it;
            continue;
        }
        // Not in line
        if(!isInLine && (itrange != remain.end()))
        {
            ret.insert(ret.end(), std::make_move_iterator(itrange), std::make_move_iterator(it)); // move voxels
            // and now delete positions
            remain.erase(itrange, it);
            // set new positions
            it = itrange;
            itrange = remain.end();
            continue;
        }
    }
    // treat last range case since we are using itrange == remain.end() as a tag
    if(itrange != remain.end())
    {
        auto it = remain.end();
        ret.insert(ret.end(), std::make_move_iterator(itrange), std::make_move_iterator(it));
        remain.erase(itrange, it);
    }
    return ret;
}

ActPhysics::Line ActAlgorithm::RANSAC::SampleLine(const std::vector<ActRoot::Voxel>& voxels)
{
    // Sample two points uniformly
    std::vector<int> idxs;
    std::vector<ActRoot::Voxel> picked;
    while(idxs.size() < 2) // Line = 2 points
    {
        int i {static_cast<int>(gRandom->Uniform() * voxels.size())};
        if(!IsInVector(i, idxs))
        {
            idxs.push_back(i);
            picked.push_back(voxels[i]);
        }
    }
    // Build Line
    return ActPhysics::Line(picked[0].GetPosition(), picked[1].GetPosition());
}

ActAlgorithm::VCluster::ClusterRet ActAlgorithm::RANSAC::Run(const std::vector<ActRoot::Voxel>& voxels, bool addNoise)
{
    // inner timing tool
    fClock.Start(false);
    std::vector<ActRoot::Cluster> clusters;
    if(voxels.size() < fMinPoints)
    {
        return {};
    }
    // Build set to compare lines
    auto lambdaCompare = [](const ActPhysics::Line& a, const ActPhysics::Line& b) { return a.GetChi2() < b.GetChi2(); };
    std::set<ActPhysics::Line, decltype(lambdaCompare)> sortedLines(lambdaCompare);
    // 1-> Run for fIterations
    for(int i = 0; i < fIterations; i++)
    {
        // 1->Sample line
        auto sampled {SampleLine(voxels)};
        // 2->Get inliers of line
        auto inliers {GetNInliers(voxels, sampled)};
        // 3-> If ninliers greater than minimum, push to set of lines
        if(inliers > fMinPoints)
            sortedLines.insert(sampled);
    }
    // 2-> Extract points from cloud belonging to the best scored lines
    auto remain = voxels; // copy to avoid modification of init vector
    for(const auto& line : sortedLines)
    {
        if(remain.size() < fMinPoints)
            break;
        auto inlierVoxels {ProcessCloud(remain, line)};
        if(inlierVoxels.size() > fMinPoints) // assert again this condition
        {
            // Create copy of line
            auto copy = line;
            // Fit it
            copy.FitVoxels(inlierVoxels);
            // And push back to vector of clustering results!
            clusters.push_back(ActRoot::Cluster(clusters.size(), copy, inlierVoxels));
        }
    }
    fClock.Stop();
    return std::make_pair(std::move(clusters), std::move(remain)); // remain voxels are noise
}

void ActAlgorithm::RANSAC::Print() const
{
    std::cout << BOLDGREEN << "==== RANSAC configuration ====" << '\n';
    std::cout << "-> DistThreshold: " << fDistThreshold << '\n';
    std::cout << "-> MinPoints    : " << fMinPoints << '\n';
    std::cout << "-> Iterations   : " << fIterations << '\n';
    std::cout << "-> UseLmeds     : " << std::boolalpha << fUseLmeds << '\n';
    std::cout << "===========================" << RESET << '\n';
}

void ActAlgorithm::RANSAC::PrintReports() const
{
    std::cout << BOLDYELLOW << "==== RANSAC time report ====" << '\n';
    fClock.Print();
    std::cout << "==============================" << RESET << '\n';
}
