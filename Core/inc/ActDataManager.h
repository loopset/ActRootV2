#ifndef ActDataManager_h
#define ActDataManager_h

#include "ActInputData.h"
#include "ActInputParser.h"
#include "ActOutputData.h"
#include "ActTypes.h"

#include "TChain.h"

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

namespace ActRoot
{
class DataManager
{
private:
    std::unordered_map<std::string, BlockPtr> fBlocks {};
    std::set<int> fRuns {};
    std::set<int> fExludeList {};
    std::string fManual {};
    ModeType fMode {ModeType::ENone};

public:
    DataManager() = default;
    DataManager(ModeType mode) : fMode(mode) {}
    DataManager(const std::string& file, ModeType mode = ModeType::ECorrect);

    void ReadDataFile(const std::string& file);

    // Main methods
    InputData GetInput() { return GetInput(fMode); }
    InputData GetInput(ModeType mode);
    InputData GetInputForThread(const std::set<int>& runs);

    OutputData GetOuput() { return GetOuput(fMode); };
    OutputData GetOuput(ModeType mode);
    OutputData GetOutputForThread(const std::set<int>& runs);

    std::shared_ptr<TChain> GetJoinedData() { return GetJoinedData(fMode); };
    std::shared_ptr<TChain> GetJoinedData(ModeType mode);

    // Getters
    const std::set<int>& GetRunList() const { return fRuns; }
    const std::set<int>& GetExcludeList() const { return fExludeList; }
    const std::string& GetManualFile() const { return fManual; }

private:
    void ParseManagerBlock(BlockPtr block);
    BlockPtr CheckAndGet(const std::string& name);
    void SetInputData(InputData& in, ModeType mode);
    void SetOutputData(OutputData& out, ModeType mode);
};
} // namespace ActRoot

#endif
