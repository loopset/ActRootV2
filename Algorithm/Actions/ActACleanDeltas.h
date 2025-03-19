#ifndef ActACleanDeltas_h
#define ActACleanDeltas_h
#include "ActVAction.h"

// forward declarations
namespace ActRoot
{
class Cluster;
}

namespace ActAlgorithm
{
namespace Actions
{
class CleanDeltas : public VAction
{
private:
    double fChi2Thresh {};      //!< Chi2 threshold (bad fit)
    double fMaxVoxels {};       //!< Max amount of voxeles for a delta e- trace  or noise
    double fSigmaGap {};        //!< Diff between leading sigma and NLO sigma to consider cluster as linear
    double fCylinderR {};       //!< Radius of cylinder used to clean
    bool fUseExtVoxels {false}; //!< Whether to use extended voxel content to determine finer chi2
    bool fUseCylinder {};       //!< Whether to use cylinder cleaning in coordinate Z

public:
    CleanDeltas() : VAction("CleanDeltas") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    std::pair<double, double> ComputeLongTransSigmas(ActRoot::Cluster* cluster);
};
} // namespace Actions
} // namespace ActAlgorithm

#endif
