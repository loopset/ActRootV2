#ifndef ActVFilter_h
#define ActVFilter_h

#include "ActMergerData.h"
#include "ActTPCData.h"
#include "ActTPCParameters.h"
#include "ActVCluster.h"

#include <memory>

namespace ActAlgorithm
{
class VFilter
{
protected:
    ActRoot::TPCData* fData {};
    ActRoot::TPCParameters* fTPC {};
    ActRoot::MergerData* fMergerData {};
    std::shared_ptr<ActAlgorithm::VCluster> fAlgo {};
    bool fIsVerbose {};

public:
    VFilter() = default;
    virtual ~VFilter() = default;

    virtual void SetTPCParameters(ActRoot::TPCParameters* tpc) { fTPC = tpc; }
    ActRoot::TPCParameters* GetTPCParameters() const { return fTPC; }

    virtual void SetTPCData(ActRoot::TPCData* data) { fData = data; }
    ActRoot::TPCData* GetTPCData() const { return fData; }

    virtual void SetMergerData(ActRoot::MergerData* data) { fMergerData = data; }
    ActRoot::MergerData* GetMergerData() const { return fMergerData; }

    virtual void SetClusterPtr(std::shared_ptr<ActAlgorithm::VCluster> ptr) { fAlgo = ptr; }

    void SetIsVerbose(bool verb = true) { fIsVerbose = verb; }
    bool GetIsVerbose() const { return fIsVerbose; }

    virtual void ReadConfiguration() = 0;
    virtual void Run() = 0;
    virtual void Print() const = 0;
    virtual void PrintReports() const = 0;
};
} // namespace ActAlgorithm

#endif
