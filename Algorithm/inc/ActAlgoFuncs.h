#ifndef ActAlgoFuncs_h
#define ActAlgoFuncs_h

#include "ActCluster.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <tuple>
#include <vector>

// forward declararions
namespace ActRoot
{
class TPCParameters;
}

namespace ActAlgorithm
{
typedef ROOT::Math::XYZPointF XYZPoint;
typedef ROOT::Math::XYZVectorF XYZVector;

// RP computation in 3D
std::tuple<XYZPoint, XYZPoint, double> ComputeRPIn3D(XYZPoint pA, XYZVector vA, XYZPoint pB, XYZVector vB);

// Check its validity
bool IsRPValid(const XYZPoint& rp, ActRoot::TPCParameters* tpc);

// Merge similar tracks is now a commom function!
void MergeSimilarClusters(std::vector<ActRoot::Cluster>* clusters, double distThresh, double minParallelFactor,
                          double chi2Factor, bool isVerbose = false);

} // namespace ActAlgorithm

#endif
