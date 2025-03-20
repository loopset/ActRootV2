#ifndef ActABreakChi2_h
#define ActABreakChi2_h

#include "ActVAction.h"

// forward declarations
namespace ActRoot
{
class Line;
}

namespace ActAlgorithm
{
namespace Actions
{
class BreakChi2 : public VAction
{
private:
    // Parameters of this action
    double fChi2Thresh {};     //!< Min Chi2 to treat cluster (bin units)
    double fMinXRange {};      //!< Min range along X (pad units) to treat cluster
    double fLengthXToBreak {}; //!< Size in X in which find the breaking point from the Xmin
    // double fMaxDistGP {};        //! Max accepted projected distance on fit to compute gravity point
    double fBeamWindowY {};      //!< Width Y around gravity point to mask not-beam voxels
    double fBeamWindowZ {};      //!< Width Z around gravity point to mask not-beam voxels
    bool fDoClusterNotBeam {};   //!< Whether to recluster or not not-beam voxels after separating them
    bool fDoBreakMultiTracks {}; //!< Whether to run or not the second part: breaking the multi-tracks events
    double fTrackChi2Thresh {};  //!< Different Chi2Thresh to be used in this second part
    double fBeamWindowScale {};  //!< Beam window scaling factor to be used in this second part
    // double fChi2Difference {};   //!< Maximum allowed chi2 diff between q-weighted and not q-weighted


public:
    // Default constructor that sets the name of the class
    // by init the base class member fActionID with the passed string
    BreakChi2() : VAction("BreakChi2") {}

    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) override;
    void Run() override;
    void Print() const override;

private:
    // Inner functions of the class
    bool ManualIsInBeam(const XYZPointF& pos, const XYZPointF& gravity, double scale = 1.);
    // bool AutoIsInBeam(const XYZPointF& pos, ActRoot::Line* line);
    void BreakMultiTrack();
};
} // namespace Actions
} // namespace ActAlgorithm

#endif
