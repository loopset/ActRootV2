#include "ActRANSAC.h"

#include "ActCluster.h"
#include "ActLine.h"
#include "ActTPCData.h"
#include "TEnv.h"
#include "TRandom.h"
#include "TSystemDirectory.h"
#include "TSystemFile.h"
#include "TList.h"
#include "TString.h"

#include <algorithm>
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
    std::string project {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    if(project.length() == 0 && infile.length() == 0)
    {
        std::cout<<"No found RANSAC config file -> Using built-in parameters"<<'\n';
        return;
    }
    auto dirname {project + "configs/"};
    TSystemDirectory dir {dirname.c_str(), dirname.c_str()};
    std::string ransacfile {dirname};
    //list files, get only the one with extension .ransac
    for(auto* obj : *dir.GetListOfFiles())
    {
        if(!obj)
            continue;
        TSystemFile* file {(TSystemFile*)obj};
        TString name {file->GetName()};
        if(name.EndsWith(".ransac"))
        {
            ransacfile += name;
            break;
        }
    }
    std::cout<<"Found file = "<<ransacfile<<'\n';
    
}

int ActCluster::RANSAC::GetNInliers(const std::vector<ActRoot::Voxel>& voxels, ActPhysics::Line& line)
{
    int ninliers {};
    for(const auto& voxel : voxels)
    {
        const auto& pos = voxel.GetPosition();
        double err = line.DistanceLineToPoint(pos);
        err *= err;
        if(err < (fDistThreshold * fDistThreshold))
            ninliers++;
    }
    line.SetChi2(1.0 / ninliers);
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
