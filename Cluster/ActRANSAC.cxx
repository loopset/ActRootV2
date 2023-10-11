#include "ActRANSAC.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActLine.h"
#include "ActTPCData.h"
#include "TEnv.h"
#include "TMath.h"
#include "TRandom.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TSystemFile.h"
#include "TList.h"
#include "TString.h"

#include <algorithm>
#include <ios>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <set>

ActCluster::RANSAC::RANSAC(int iterations, int minPoints, double distThres)
    : fIterations(iterations), fMinPoints(minPoints), fDistThreshold(distThres)
{}

void ActCluster::RANSAC::ReadConfigurationFile(const std::string& infile)
{
    //automatically get project path from gEnv
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/cluster.ransac";
    std::string realfile {};
    if(!gSystem->AccessPathName(envfile.c_str()))
        realfile = envfile;
    else if(infile.length() > 0)
        realfile = infile;
    else
    {
        std::cout<<BOLDMAGENTA<<".ransac config file not found -> Using built-in configuration"<<'\n';
        return;
    }
    //Parse!
    ActRoot::InputParser parser {realfile};
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

int ActCluster::RANSAC::GetNInliers(const std::vector<ActRoot::Voxel>& voxels, ActPhysics::Line& line)
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
    //Naive implementation of other estimators simply changing the test value
    if(fUseLmeds)
    {
        double weight {TMath::Median(verrors.size(), &(verrors[0]))};
        line.SetChi2(weight / ninliers);
    }
    else
        line.SetChi2(1. / ninliers);    
    return ninliers;
}

std::vector<ActRoot::Voxel> ActCluster::RANSAC::ProcessCloud(std::vector<ActRoot::Voxel>& remain, const ActPhysics::Line& line)
{
    std::vector<ActRoot::Voxel> ret {};
    //clear cloud according to line: delete from it voxel belonging to this line
    auto itrange {remain.end()};//to work with ranges of voxels, using insert range of iterator capabilities!
    for(auto it = remain.begin(); it != remain.end(); ++it)
    {
        //Compute error
        double err {line.DistanceLineToPoint(it->GetPosition())};
        bool isInLine {(err * err) < (fDistThreshold * fDistThreshold)};

        //In line
        if(isInLine && (itrange == remain.end()))
        {
            itrange = it;
            continue;
        }
        //Not in line
        if(!isInLine && (itrange != remain.end()))
        {
            ret.insert(ret.end(), std::make_move_iterator(itrange), std::make_move_iterator(it));//move voxels
            //and now delete positions
            remain.erase(itrange, it);
            //set new positions
            it = itrange;
            itrange = remain.end();
            continue;
        }
    }
    //treat last range case since we are using itrange == remain.end() as a tag
    if(itrange != remain.end())
    {
        auto it = remain.end();
        ret.insert(ret.end(), std::make_move_iterator(itrange), std::make_move_iterator(it));
        remain.erase(itrange, it);
    }
    return ret;
}

ActPhysics::Line ActCluster::RANSAC::SampleLine(const std::vector<ActRoot::Voxel>& voxels)
{
    //Sample two points uniformly
    std::vector<int> idxs;
    std::vector<ActRoot::Voxel> picked;
    while(idxs.size() < 2)//Line = 2 points
    {
        int i {static_cast<int>(gRandom->Uniform() * voxels.size())};
        if(!IsInVector(i, idxs))
        {
            idxs.push_back(i);
            picked.push_back(voxels[i]);
        }
    }
    //Build Line
    return ActPhysics::Line(picked[0].GetPosition(), picked[1].GetPosition());
}

std::vector<ActCluster::Cluster> ActCluster::RANSAC::Run(const std::vector<ActRoot::Voxel> &voxels)
{
    std::vector<ActCluster::Cluster> ret;
    if(voxels.size() < fMinPoints)
    {
        if(gEnv->GetValue("ActRoot.Verbose", false))
            std::cout<<"RANSAC::Run() Voxel size smaller than fMinPoints = "<<fMinPoints<<'\n';
        return ret;
    }
    //Build set to compare lines
    auto lambdaCompare = [](const ActPhysics::Line& a, const ActPhysics::Line& b){return a.GetChi2() < b.GetChi2();};
    std::set<ActPhysics::Line, decltype(lambdaCompare)> sortedLines(lambdaCompare);
    //1-> Run for fIterations
    for(int i = 0; i < fIterations; i++)
    {
        //1->Sample line
        auto sampled {SampleLine(voxels)};
        //2->Get inliers of line
        auto inliers {GetNInliers(voxels, sampled)};
        //3-> If ninliers greater than minimum, push to set of lines
        if(inliers > fMinPoints)
            sortedLines.insert(sampled);
    }
    //2-> Extract points from cloud belonging to the best scored lines
    auto remain = voxels;//copy to avoid modification of init vector
    for(const auto& line : sortedLines)
    {
        if(remain.size() < fMinPoints)
            break;
        auto inlierVoxels {ProcessCloud(remain, line)};
        if(inlierVoxels.size() > fMinPoints)//assert again this condition
        {
            //Create copy of line
            auto copy = line;
            //Fit it
            copy.FitVoxels(inlierVoxels);
            //And push back to vector of clustering results!
            ret.push_back(ActCluster::Cluster(ret.size(), copy, inlierVoxels));
        }
    }
    return ret;
}

void ActCluster::RANSAC::Print() const
{
    std::cout<<BOLDGREEN<<"==== RANSAC configuration ===="<<'\n';
    std::cout<<"-> DistThreshold: "<<fDistThreshold<<'\n';
    std::cout<<"-> MinPoints    : "<<fMinPoints<<'\n';
    std::cout<<"-> Iterations   : "<<fIterations<<'\n';
    std::cout<<"-> UseLmeds     : "<<std::boolalpha<<fUseLmeds<<'\n';
    std::cout<<"==========================="<<RESET<<'\n';
}
