#include "ActClIMB.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActTPCDetector.h"
#include "ActLine.h"

#include "TEnv.h"
#include "TRandom.h"
#include "TSystem.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

ActCluster::ClIMB::ClIMB(ActRoot::TPCParameters* tpc, int minPoints)
    : fTPC(tpc), fMinPoints(minPoints)
{
    InitMatrix();
}

void ActCluster::ClIMB::Print() const
{
    std::cout<<BOLDMAGENTA<<".... ClIMB configuration ...."<<'\n';
    std::cout<<"-> MinPoints : "<<fMinPoints<<'\n';
    std::cout<<"............................."<<RESET<<'\n';
}

void ActCluster::ClIMB::ReadConfigurationFile(const std::string& infile)
{
    //automatically get project path from gEnv
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/cluster.climb";
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
    auto cb {parser.GetBlock("Climb")};
    if(cb->CheckTokenExists("MinPoints"))
        fMinPoints = cb->GetInt("MinPoints");
}

void ActCluster::ClIMB::InitMatrix()
{
    fMatrix = std::vector<std::vector<std::vector<int>>>(fTPC->GetNPADSX(),
                                                         std::vector<std::vector<int>>(fTPC->GetNPADSY(),
                                                                                       std::vector<int>(fTPC->GetNPADSZ() / fTPC->GetREBINZ(), -1)));
}

void ActCluster::ClIMB::FillMatrix()
{
    for(int i = 0, size = fVoxels.size(); i < size; i++)
    {
        auto [x, y, z] {GetCoordinates(i)};
        fMatrix[x][y][z] = i;
    }
}

std::tuple<int, int, int> ActCluster::ClIMB::GetCoordinates(int index)
{
    const auto& pos {fVoxels[index].GetPosition()};
    auto x {(int)pos.X()};
    auto y {(int)pos.Y()};
    auto z {(int)(pos.Z() / fTPC->GetREBINZ())};
    return {x, y, z};
}

void ActCluster::ClIMB::MaskVoxelsInMatrix(int index)
{
    auto [x, y, z] {GetCoordinates(index)};
    fMatrix[x][y][z] = -1;
}

int ActCluster::ClIMB::SampleSeed()
{
    auto seed {static_cast<int>(gRandom->Uniform() * fVoxels.size())};
    MaskVoxelsInMatrix(seed);
    return seed;
}

std::vector<int> ActCluster::ClIMB::ScanNeighborhood(const std::vector<int>& gen0)
{
    std::vector<int> gen1;
    for(const auto& ivoxel : gen0)
    {
        auto [x, y, z] {GetCoordinates(ivoxel)};
        for(int ix = -1; ix <= 1; ix++)//x positions
        {
            for(int iy = -1; iy <= 1; iy++)//y positions
            {
                for(int iz = -1; iz <= 1; iz++)//z positions
                {
                    if(ix == 0 && iy == 0 && iz == 0)//skip self point
                        continue;                    
                    int index {};
                    try
                    {
                        index = fMatrix.at(x + ix).at(y + iy).at(z +iz);
                    }
                    catch(std::exception& e)
                    {
                        continue;
                    }
                    if(index != -1)
                    {
                        gen1.push_back(index);
                        MaskVoxelsInMatrix(index);
                    }
                }
            }
        }
    }
    return gen1;
}

std::vector<ActCluster::Cluster> ActCluster::ClIMB::Run(const std::vector<ActRoot::Voxel> &voxels)
{
    fVoxels = voxels;//copy in internal variable to avoid modifications
    std::vector<ActCluster::Cluster> ret;
    while(fVoxels.size() > 0)
    {
        //1->Prepare 3D matrix for each iteration, with new indexes
        FillMatrix();
        //2->Set cluster seed point
        auto seed {SampleSeed()};
        //Major structures
        //--- Current cluster class
        ActCluster::Cluster currentCluster {static_cast<int>(ret.size())};
        currentCluster.AddVoxel(std::move(fVoxels[seed]));
        //--- Indexes processed during construction of this cluster
        std::set<int, std::greater<int>> currentIndexes {seed};
        //3-> Initialize generation 0
        std::vector<int> gen0 {seed};
        //4-> Loop until no new neighbors are found!
        while(gen0.size() > 0)
        {
            auto gen1 {ScanNeighborhood(gen0)};
            //Check no indexes are repeated
            // for(auto& g0 : gen0)
            //     for(auto& g1 : gen1)
            //         if(g1 == g0)
            //             throw std::runtime_error("gen0 == gen1 at some point");
            //Push back voxels and indexes
            for(const auto& index : gen1)
            {
                currentCluster.AddVoxel(std::move(fVoxels[index]));
                currentIndexes.insert(index);
            }
            //Set gen0 to new iteration!
            gen0 = gen1;
        }
        //Delete voxels in just formed cluster, despite being moved
        for(const auto index : currentIndexes)
            fVoxels.erase(fVoxels.begin() + index);
        //Check whether to validate cluster or not
        //Just if threshold is overcome
        if(currentCluster.GetSizeOfVoxels() > fMinPoints)
        {
            //Of course, fit it before pushing
            currentCluster.ReFit();
            ret.push_back(std::move(currentCluster));
        }
    }
    fVoxels.clear();
    return ret;
}
