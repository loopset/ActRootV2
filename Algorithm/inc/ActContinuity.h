#ifndef ActClIMB_h
#define ActClIMB_h

#include "ActVCluster.h"
#include "ActVoxel.h"

#include "TStopwatch.h"

#include <tuple>
#include <vector>

// forward declaration to avoid circular dependencies
namespace ActRoot
{
class TPCParameters;
}

namespace ActAlgorithm
{
//! Implementation of a continuity-based cluster algorithm by J. Lois-Fuentes
class Continuity : public VCluster
{
private:
    // Timer
    TStopwatch fClock {};
    std::vector<std::vector<std::vector<int>>> fMatrix; //!< 3D matrix to locate clusters in space
    std::vector<ActRoot::Voxel> fVoxels;                //!< Local copy of vector to be treated
    std::vector<int> fIndexes;
    ActRoot::TPCParameters* fTPC {}; //!< Pointer to TPC parameters needed to define algorithm parameters
public:
    Continuity() = default;
    Continuity(ActRoot::TPCParameters* tpc, int minPoints);
    ~Continuity() override = default;

    // Read config file
    void ReadConfiguration() override;

    // Setters and getters
    void SetTPCParameters(ActRoot::TPCParameters* tpc)
    {
        fTPC = tpc;
        InitMatrix();
    }

    // Main method
    ClusterRet Run(const std::vector<ActRoot::Voxel>& voxels, bool addNoise = false) override;

    // Print
    void Print() const override;
    void PrintReports() const override;

private:
    void InitMatrix();
    void InitIndexes();
    void FillMatrix();
    std::vector<int> ScanNeighborhood(const std::vector<int>& gen0);
    std::tuple<int, int, int> GetCoordinates(int index);
    void MaskVoxelsInMatrix(int index);
    void MaskVoxelsInIndex(int index);
    template <typename T>
    bool IsInCage(T x, T y, T z);
};
} // namespace ActAlgorithm

#endif
