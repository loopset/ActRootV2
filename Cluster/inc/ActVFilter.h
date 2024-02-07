#ifndef ActVFilter_h
#define ActVFilter_h

#include "ActTPCData.h"
#include "ActVCluster.h"

#include <memory>

namespace ActCluster
{
class VFilter
{
protected:
    ActRoot::TPCData* fData {};
    std::shared_ptr<ActCluster::VCluster> fAlgo {};
    bool fIsVerbose {};

public:
    VFilter() = default;
    virtual ~VFilter() = default;

    virtual void SetTPCData(ActRoot::TPCData* data) { fData = data; }
    ActRoot::TPCData* GetTPCData() const { return fData; }

    virtual void SetClusterPtr(std::shared_ptr<ActCluster::VCluster> ptr) { fAlgo = ptr; }

    void SetIsVerbose(bool verb = true) { fIsVerbose = verb; }
    bool GetIsVerbose() const { return fIsVerbose; }

    virtual void ReadConfiguration() = 0;
    virtual void Run() = 0;
    virtual void Print() const = 0;
    virtual void PrintReports() const = 0;
};
} // namespace ActCluster

#endif
