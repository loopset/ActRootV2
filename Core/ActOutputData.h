#ifndef ActOutputData_h
#define ActOutputData_h

#include "TFile.h"
#include "TTree.h"

#include <map>
#include <memory>
#include <string>
#include <vector>
namespace ActRoot
{
    // forward declaration
    class InputData;

    class OutputData
    {
    private:
        std::vector<int> fRuns;
        std::map<int, std::shared_ptr<TFile>> fFiles;
        std::map<int, std::shared_ptr<TTree>> fTrees;
        std::string fTreeName;

    public:
        OutputData() = default;
        OutputData(const InputData& input);
        OutputData(const std::string& file);
        ~OutputData() = default;
        OutputData(const OutputData&) = default;
        OutputData& operator=(const OutputData&) = default;

        void ReadConfiguration(const std::string& file);
        void SetSameTreesAsInput(const InputData& input);
        void SetTreeName(const std::string& treeName) { fTreeName = treeName; }
        void InitFile(int run, const std::string& file);
        std::map<int, std::shared_ptr<TTree>> GetTrees() const { return fTrees; }
        std::shared_ptr<TTree> GetTree(int run) const { return fTrees.at(run); }
        void Fill(int run);
        // Write TTree and close TFile
        void Close(int run);
        // Function to write analysis parameters next to TTree
        void WriteMetadata(const std::string& file, const std::string& description = "");
    };
} // namespace ActRoot

#endif
