#ifndef ActMultiAction_h
#define ActMultiAction_h

#include "ActVAction.h"
#include "ActVFilter.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// forward declarations
namespace ActRoot
{
class TPCData;
}

namespace ActAlgorithm
{
class MultiAction : public VFilter
{
public:
    typedef std::shared_ptr<VAction> Ptr;
    typedef std::unordered_map<std::string, Ptr (*)()> MapActions;

private:
    MapActions fMap {};           // know actions to instantiate
    std::vector<Ptr> fActions {}; // Actions by order in file

public:
    MultiAction();
    ~MultiAction() override = default;

    void SetTPCData(ActRoot::TPCData* data) override;
    void ReadConfiguration() override;
    void Run() override;
    void Print() const override;
    void PrintReports() const override;

    MapActions& GetActionsMap() { return fMap; }
    Ptr ConstructAction(const std::string& actionID);
};

template <typename T>
inline MultiAction::Ptr RegisterAction()
{
    return std::make_shared<T>();
}
} // namespace ActAlgorithm

#endif
