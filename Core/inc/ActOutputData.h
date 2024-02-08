#ifndef ActOutputData_h
#define ActOutputData_h

#include "ActInputParser.h"

#include "TFile.h"
#include "TTree.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace ActRoot
{
class OutputData
{
private:
    std::string fTreeName {};
    std::string fPath {};
    std::string fBegin {};
    std::string fEnd {};
    std::map<int, std::shared_ptr<TFile>> fFiles {};
    std::map<int, std::shared_ptr<TTree>> fTrees {};
    std::set<int> fRuns {};

public:
    OutputData() = default;
    ~OutputData() = default;
    OutputData(const OutputData&) = default;
    OutputData& operator=(const OutputData&) = default;

    void AddOuput(BlockPtr block) { ParseBlock(block); }

    void Init(const std::set<int>& runs, bool print = true);

    void Fill(int run);

    // Write TTree and close TFile
    void Close(int run);

    // Function to write analysis parameters next to TTree
    void WriteMetadata(const std::string& file, const std::string& description = "");

    // Getters
    std::map<int, std::shared_ptr<TTree>> GetTrees() const { return fTrees; }
    std::shared_ptr<TTree> GetTree(int run) const { return fTrees.at(run); }
    const std::set<int>& GetRunList() const { return fRuns; }

private:
    void ParseBlock(BlockPtr block);
};
} // namespace ActRoot

#endif
