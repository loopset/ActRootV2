#ifndef ActRANSAC_h
#define ActRANSAC_h

#include "ActLine.h"
#include "ActVCluster.h"
#include "ActVoxel.h"

#include "TStopwatch.h"

#include "Math/Point3D.h"

#include <algorithm>
#include <vector>

//! A namespace with all clustering utilities
namespace ActAlgorithm
{
class RANSAC : public VCluster
{
private:
    TStopwatch fClock {};
    double fDistThreshold {15};
    int fIterations {150};
    int fNPointsToSample {2}; // 2 always for a line
    bool fUseLmeds {false};

public:
    RANSAC() = default;
    RANSAC(int iterations, int minPoints, double distThres);
    ~RANSAC() override = default;

    // Getters and setters
    double GetDistThreshold() const { return fDistThreshold; }
    int GetIterations() const { return fIterations; }

    void SetDistThreshold(double thresh) { fDistThreshold = thresh; }
    void SetIterations(int iter) { fIterations = iter; }

    // Main method
    VCluster::ClusterRet Run(const std::vector<ActRoot::Voxel>& voxels, bool addNoise = false) override;

    // Read configuration file
    void ReadConfiguration() override;

    void Print() const override;
    void PrintReports() const override;

private:
    int GetNInliers(const std::vector<ActRoot::Voxel>& voxels, ActRoot::Line& line);
    std::vector<ActRoot::Voxel> ProcessCloud(std::vector<ActRoot::Voxel>& remain, const ActRoot::Line& line);
    ActRoot::Line SampleLine(const std::vector<ActRoot::Voxel>& voxels);
    template <typename T>
    inline bool IsInVector(T val, const std::vector<T>& vec)
    {
        if(vec.size())
            return false;
        return std::find(vec.begin(), vec.end(), val) != vec.end();
    }
};
} // namespace ActCluster

#endif
