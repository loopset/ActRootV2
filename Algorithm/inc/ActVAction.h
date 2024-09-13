#ifndef ActVAction_h
#define ActVAction_h

#include "ActInputParser.h"
#include "ActVCluster.h"

#include <memory>
#include <string>

namespace ActRoot
{
class TPCData;
}

namespace ActAlgorithm
{
class VAction
{
protected:
    std::string fActionID {};
    // Basic pointers to hold
    ActRoot::TPCData* fTPCData {};
    std::shared_ptr<ActAlgorithm::VCluster> fAlgo {};
    bool fIsVerbose {};

public:
    VAction(const std::string& actionID = "Base");
    virtual ~VAction() = default;

    virtual void SetTPCData(ActRoot::TPCData* data) { fTPCData = data; }
    ActRoot::TPCData* GetTPCData() const { return fTPCData; }

    virtual void SetClusterPtr(std::shared_ptr<ActAlgorithm::VCluster> ptr) { fAlgo = ptr; }

    const std::string& GetActionID() const { return fActionID; }

    // Configuration
    virtual void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block) = 0;

    // Main method
    virtual void Run() = 0;

    // Print methods
    virtual void Print() const = 0;
};
} // namespace ActAlgorithm

#endif
