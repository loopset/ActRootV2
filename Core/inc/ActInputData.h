#ifndef ActInputData_h
#define ActInputData_h

#include "ActInputParser.h"

#include "TChain.h"
#include "TFile.h"
#include "TString.h"
#include "TTree.h"
#include "TUUID.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ActRoot
{
class InputData
{
private:
    std::vector<std::string> fTreeNames {};
    std::vector<std::string> fPaths {};
    std::vector<std::string> fBegins {};
    std::vector<std::string> fEnds {};
    std::map<int, std::shared_ptr<TFile>> fFiles {};
    std::map<int, std::shared_ptr<TTree>> fTrees {};
    std::map<int, std::vector<int>> fManualEntries {};
    // Local copy of runs from DataManager
    std::set<int> fRuns {};

public:
    InputData() = default;
    ~InputData() = default;
    InputData(const InputData&) = default;
    InputData& operator=(const InputData&) = default;

    void AddInput(BlockPtr block) { ParseBlock(block); };

    void AddManualEntries(const std::string& file);

    void Init(const std::set<int>& runs);

    // Getters
    std::map<int, std::shared_ptr<TTree>> GetTrees() const { return fTrees; }
    std::shared_ptr<TTree> GetTree(int run) const { return fTrees.at(run); }
    void GetEntry(int run, int entry);
    int GetNEntries(int run) const { return fTrees.at(run)->GetEntries(); }
    const std::map<int, std::vector<int>>& GetManualEntries() const { return fManualEntries; }
    const std::set<int>& GetRunList() const { return fRuns; }

    // Close files once processed
    void Close(int run);

private:
    void ParseBlock(BlockPtr block);
    void CheckFileExists(const std::string& file);
    void CheckTreeExists(std::shared_ptr<TTree> tree, int i);
};
} // namespace ActRoot
#endif
