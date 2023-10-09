#include "ActRANSAC.h"

#include <string>

ActCluster::RANSAC::RANSAC(int iterations, int minPoints, double distThres)
    : fIterations(iterations), fMinPoints(minPoints), fDistThreshold(distThres)
{}

void ActCluster::RANSAC::ReadConfigurationFile(const std::string& infile)
{
    //in progress
}
