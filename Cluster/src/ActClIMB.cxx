#include "ActClIMB.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"
#include "ActLine.h"
#include "ActTPCDetector.h"
#include "ActVoxel.h"

#include "TEnv.h"
#include "TRandom.h"
#include "TSystem.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

ActCluster::ClIMB::ClIMB(ActRoot::TPCParameters* tpc, int minPoints) : fTPC(tpc), fMinPoints(minPoints)
{
    InitMatrix();
}

void ActCluster::ClIMB::Print() const
{
    std::cout << BOLDMAGENTA << ".... ClIMB configuration ...." << '\n';
    std::cout << "-> MinPoints : " << fMinPoints << '\n';
    std::cout << "............................." << RESET << '\n';
}

void ActCluster::ClIMB::ReadConfigurationFile(const std::string& infile)
{
    // automatically get project path from gEnv
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/climb.conf";
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
    auto cb {parser.GetBlock("Climb")};
    if(cb->CheckTokenExists("MinPoints"))
        fMinPoints = cb->GetInt("MinPoints");
}

void ActCluster::ClIMB::InitMatrix()
{
    fMatrix = std::vector<std::vector<std::vector<int>>>(
        fTPC->GetNPADSX(),
        std::vector<std::vector<int>>(fTPC->GetNPADSY(), std::vector<int>(fTPC->GetNPADSZ() / fTPC->GetREBINZ(), -1)));
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
    auto z {(int)pos.Z()};
    return {x, y, z};
}

void ActCluster::ClIMB::MaskVoxelsInMatrix(int index)
{
    auto [x, y, z] {GetCoordinates(index)};
    fMatrix[x][y][z] = -1;
}

void ActCluster::ClIMB::MaskVoxelsInIndex(int index)
{
    fIndexes[index] = -1;
}
template <typename T>
bool ActCluster::ClIMB::IsInCage(T x, T y, T z)
{
    bool condX {0 <= x && x < fTPC->GetNPADSX()};
    bool condY {0 <= y && y < fTPC->GetNPADSY()};
    bool condZ {0 <= z && z < (fTPC->GetNPADSZ() / fTPC->GetREBINZ())};
    return condX && condY && condZ;
}

std::vector<int> ActCluster::ClIMB::ScanNeighborhood(const std::vector<int>& gen0)
{
    std::vector<int> gen1;
    for(const auto& ivoxel : gen0)
    {
        auto [x, y, z] {GetCoordinates(ivoxel)};
        for(int ix = -1; ix <= 1; ix++) // x positions
        {
            for(int iy = -1; iy <= 1; iy++) // y positions
            {
                for(int iz = -1; iz <= 1; iz++) // z positions
                {
                    if(ix == 0 && iy == 0 && iz == 0) // skip self point
                        continue;
                    int index {};
                    if(IsInCage(x + ix, y + iy, z + iz))
                        index = fMatrix[x + ix][y + iy][z + iz];
                    else
                        continue;
                    if(index != -1)
                    {
                        gen1.push_back(index);
                        // Mask them both in matrix and in indexes vector
                        MaskVoxelsInMatrix(index);
                        MaskVoxelsInIndex(index);
                    }
                }
            }
        }
    }
    return std::move(gen1);
}

void ActCluster::ClIMB::InitIndexes()
{
    // Clear
    fIndexes.clear();
    // Allocate enough memory
    fIndexes.reserve(fVoxels.size());
    // Set size
    fIndexes.resize(fVoxels.size());
    // Fill
    std::iota(fIndexes.begin(), fIndexes.end(), 0);
}

ActCluster::ClIMB::RetType ActCluster::ClIMB::Run(const std::vector<ActRoot::Voxel>& voxels, bool returnNoise)
{
    fVoxels = voxels; // copy in internal variable to avoid modifications
    // Init Indexes structure
    InitIndexes();
    // Fill matrix
    FillMatrix();
    // Prepare return values:
    // Clusters (c)
    std::vector<ActCluster::Cluster> cret;
    // Noise (n)
    std::vector<ActRoot::Voxel> nret;
    // Getter of seed based on fIndex being masked (=-1)
    auto lambda {[](const int& i) { return i == -1; }};
    auto it {std::find_if_not(fIndexes.begin(), fIndexes.end(), lambda)};
    while(it != fIndexes.end()) //(fVoxels.size() > 0)
    {
        // 1->Set seed of cluster as first non-masked element of fIndexes
        auto seed {*it};
        // Mask it!
        MaskVoxelsInMatrix(seed);
        MaskVoxelsInIndex(seed);
        // 2-> Create current cluster
        ActCluster::Cluster currentCluster {static_cast<int>(cret.size())};
        currentCluster.AddVoxel(std::move(fVoxels[seed]));
        // 3-> Initialize generation 0
        std::vector<int> gen0 {seed};
        // 4-> Loop until no new neighbors are found!
        while(gen0.size() > 0)
        {
            auto gen1 {ScanNeighborhood(gen0)};
            // Check no indexes are repeated
            //  for(auto& g0 : gen0)
            //      for(auto& g1 : gen1)
            //          if(g1 == g0)
            //              throw std::runtime_error("gen0 == gen1 at some point");
            // Push back voxels and indexes
            for(const auto& index : gen1)
                currentCluster.AddVoxel(std::move(fVoxels[index]));
            // Set gen0 to new iteration!
            gen0 = gen1;
        }
        // Check whether to validate cluster or not
        // based on number of voxels
        if(currentCluster.GetSizeOfVoxels() > fMinPoints)
        {
            // Of course, fit it before pushing
            currentCluster.ReFit();
            cret.push_back(std::move(currentCluster));
        }
        else
        {
            if(returnNoise)
            {
                for(auto& voxel : currentCluster.GetRefToVoxels())
                    nret.push_back(std::move(voxel));
            }
        }
        // Prepare iterator for next iteration
        it = std::find_if_not(fIndexes.begin(), fIndexes.end(), lambda);
    }
    fVoxels.clear();
    return std::make_pair(std::move(cret), std::move(nret));
}
