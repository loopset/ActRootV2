#ifndef ActMultiAction_h
#define ActMultiAction_h

#include "ActInputParser.h"
#include "ActVAction.h"
#include "ActVCluster.h"
#include "ActVFilter.h"

#include "TStopwatch.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// forward declarations
namespace ActRoot
{
class TPCData;
class TPCParameters;
} // namespace ActRoot

namespace ActAlgorithm
{
class MultiAction : public VFilter
{
public:
    typedef std::shared_ptr<VAction> Ptr;
    typedef std::unordered_map<std::string, Ptr (*)()> MapActions;

private:
    MapActions fMap {};           //!< Known actions to instantiate
    std::vector<Ptr> fActions {}; //!< Actions by order in file

    TStopwatch fTimer {}; //!< To control time spent in executing the actions

public:
    MultiAction();
    ~MultiAction() override = default;

    void SetTPCParameters(ActRoot::TPCParameters* pars) override;
    void SetTPCData(ActRoot::TPCData* data) override;
    void SetClusterPtr(std::shared_ptr<ActAlgorithm::VCluster> ptr) override;
    void ReadConfiguration() override;
    void Run() override;
    void Print() const override;
    void PrintReports() const override;

    MapActions& GetActionsMap() { return fMap; }
    Ptr ConstructAction(const std::string& actionID);

private:
    void LoadUserAction(std::shared_ptr<ActRoot::InputBlock> block);
    void ResetClusterID();
};

template <typename T>
inline MultiAction::Ptr RegisterAction()
{
    return std::make_shared<T>();
}
} // namespace ActAlgorithm

#endif
