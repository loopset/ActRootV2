#ifndef ActAlgoFuncs_h
#define ActAlgoFuncs_h

#include "ActCluster.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <tuple>
#include <utility>
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
typedef std::pair<XYZPoint, std::pair<int, int>> RPElement;
typedef std::vector<RPElement> RPVector;
typedef std::pair<XYZPoint, std::set<int>> RPSet;

// RP computation in 3D
std::tuple<XYZPoint, XYZPoint, double> ComputeRPIn3D(XYZPoint pA, XYZVector vA, XYZPoint pB, XYZVector vB);

// Check its validity
bool IsRPValid(const XYZPoint& rp, ActRoot::TPCParameters* tpc);

// Merge similar tracks is now a commom function!
void MergeSimilarClusters(std::vector<ActRoot::Cluster>* clusters, double distThresh, double minParallelFactor,
                          double chi2Factor, bool isVerbose = false);

// Clean voxels in cylinder
void CylinderCleaning(std::vector<ActRoot::Cluster>* cluster, double cylinderR, int minVoxels, bool isVerbose = false);

// Clean bad fit or with large chi2
void Chi2AndSizeCleaning(std::vector<ActRoot::Cluster>* cluster, double chi2Threh, int minVoxels = -1,
                         bool isVerbose = false);

// Simplified version of a cluster of RP points. Returns mean of RPs close enough (given a threshold)
RPSet SimplifyRPs(const RPVector& rps, double distThresh);

// Break BL after RP is found
void BreakBeamToHeavy(std::vector<ActRoot::Cluster>* clusters, const XYZPoint& rp, int minVoxels, bool keepSplit = true,
                      bool isVerbose = false);

// Mask beginning and end of tracks to get a better fit
void MaskBeginEnd(std::vector<ActRoot::Cluster>* clusters, const XYZPoint& rp, double pivotDist, int minVoxels,
                  bool isVerbose = false);

// Compute angle between two clusters
double GetClusterAngle(const XYZVector& beam, const XYZVector& recoil);

} // namespace ActAlgorithm

#endif
