#ifndef ActRANSAC_h
#define ActRANSAC_h

#include "ActCluster.h"
#include "ActLine.h"
#include "ActTPCData.h"

#include "Math/Point3D.h"

#include <algorithm>
#include <utility>
#include <vector>

//! A namespace with all clustering utilities
namespace ActCluster
{
    class RANSAC
    {
    public:
        using XYZPoint = ROOT::Math::XYZPoint;

    private:
        double fDistThreshold {15};
        int fIterations {150};
        int fMinPoints {20};
        int fNPointsToSample {2}; // 2 always for a line
        bool fUseLmeds {false};

    public:
        RANSAC() = default;
        RANSAC(int iterations, int minPoints, double distThres);
        ~RANSAC() = default;

        // Getters and setters
        double GetDistThreshold() const { return fDistThreshold; }
        int GetIterations() const { return fIterations; }
        int GetMinPoints() const { return fMinPoints; }

        void SetDistThreshold(double thresh) { fDistThreshold = thresh; }
        void SetIterations(int iter) { fIterations = iter; }
        void SetMinPoints(int minPoints) { fMinPoints = minPoints; }

        // Main method
        std::vector<ActCluster::Cluster> Run(const std::vector<ActRoot::Voxel>& voxels);

        // Read configuration file
        void ReadConfigurationFile(const std::string& infile = "");

        void Print() const;

    private:
        int GetNInliers(const std::vector<ActRoot::Voxel>& voxels, ActPhysics::Line& line);
        std::vector<ActRoot::Voxel> ProcessCloud(std::vector<ActRoot::Voxel>& remain, const ActPhysics::Line& line);
        ActPhysics::Line SampleLine(const std::vector<ActRoot::Voxel>& voxels);
        template <typename T>
        inline bool IsInVector(T val, const std::vector<T>& vec)
        {
            if (vec.size())
                return false;
            return std::find(vec.begin(), vec.end(), val) != vec.end();
        }
    };
} // namespace ActCluster

#endif
