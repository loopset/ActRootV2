#ifndef ActVAction_h
#define ActVAction_h

#include "ActInputParser.h"
#include "ActVCluster.h"

#include "Math/Point3Dfwd.h"
#include "Math/Vector3Dfwd.h"

#include <memory>
#include <string>

namespace ActRoot
{
class TPCData;
class TPCParameters;
} // namespace ActRoot

namespace ActAlgorithm
{
class VAction
{
public:
    using XYZVector = ROOT::Math::XYZVector;
    using XYZVectorF = ROOT::Math::XYZVectorF;
    using XYZPoint = ROOT::Math::XYZPoint;
    using XYZPointF = ROOT::Math::XYZPointF;

protected:
    std::string fActionID {};
    // Basic pointers to hold
    ActRoot::TPCData* fTPCData {};
    ActRoot::TPCParameters* fTPCPars {};
    std::shared_ptr<ActAlgorithm::VCluster> fAlgo {};

    bool fIsEnabled {true};
    bool fIsVerbose {};

public:
    VAction(const std::string& actionID = "Base");
    virtual ~VAction() = default;

    virtual void SetTPCParameters(ActRoot::TPCParameters* pars) { fTPCPars = pars; }
    ActRoot::TPCParameters* GetTPCParameters() const { return fTPCPars; }

    virtual void SetTPCData(ActRoot::TPCData* data) { fTPCData = data; }
    ActRoot::TPCData* GetTPCData() const { return fTPCData; }

    virtual void SetClusterPtr(std::shared_ptr<ActAlgorithm::VCluster> ptr) { fAlgo = ptr; }

    const std::string& GetActionID() const { return fActionID; }

    void SetIsEnabled(bool enabled) { fIsEnabled = enabled; }
    bool GetIsEnabled() const { return fIsEnabled; }

    void SetIsVerbose(bool verbose) { fIsVerbose = verbose; }
    bool GetIsVerbose() const { return fIsVerbose; }

    // Configuration
    virtual void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) = 0;

    // Main method
    virtual void Run() = 0;

    // Print methods
    virtual void Print() const = 0;
};
} // namespace ActAlgorithm

#endif
