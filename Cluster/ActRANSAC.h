#ifndef ActRANSAC_h
#define ActRANSAC_h

#include "ActLine.h"
#include "ActTPCData.h"
#include <vector>

//! A namespace with all clustering utilities
namespace ActCluster
{
    class RANSAC
    {
    private:
        double fDistThreshold {15};
        int fIterations {150};
        int fMinPoints {20};
        int fNPointsToSample {2};//2 always for a line

    public:
        RANSAC(int iterations, int minPoints, double distThres);

        //Getters and setters
        double GetDistThreshold() const {return fDistThreshold;}
        int GetIterations() const {return fIterations;}
        int GetMinPoints() const {return fMinPoints;}

        void SetDistThreshold(double thresh){fDistThreshold = thresh;}
        void SetIterations(int iter){fIterations = iter;}
        void SetMinPoints(int minPoints){fMinPoints = minPoints;}

        //Main method
        std::vector<ActPhysics::Line> Run(const std::vector<ActRoot::Voxel>& voxels);

        //Read configuration file
        void ReadConfigurationFile(const std::string& infile = "");

    private:
        int GetNInliers(const std::vector<ActRoot::Voxel>& voxels, const ActPhysics::Line& line);
        std::vector<ActRoot::Voxel> RankLines(const std::vector<ActRoot::Voxel>& remain, const ActPhysics::Line& line);
    };
}

#endif
