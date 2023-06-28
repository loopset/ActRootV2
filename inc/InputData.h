#ifndef ActInputData_h
#define ActInputData_h

#include "TChain.h"
#include "TTree.h"
#include "TUUID.h"
#include <memory>
#include <string>
#include <map>
#include <utility>
#include <vector>

namespace ActRoot
{
    //forward declaration
    class InputData;
    class InputIterator
    {
    private:
        std::map<int, int>::iterator fRunIt;
        int fCurrentRun;
        int fCurrentEntry;
        std::map<int, int> fEntries;

    public:
        InputIterator() = default;
        InputIterator(const InputData* input);
        ~InputIterator() = default;

        bool Previous();
        bool Next();
        bool GoTo(int run, int entry);
        std::pair<int, int> GetCurrentRunEntry() const {return {fCurrentRun, fCurrentEntry};}

    private:
        bool CheckEntryIsInRange(int run, int entry);
    };
    
    class InputData
    {
    private:
        std::vector<int> fRuns;
        std::map<int, std::shared_ptr<TFile>> fFiles;
        std::map<int, std::shared_ptr<TTree>> fTrees;
        std::string fTreeName;
        
    public:
        InputData() = default;
        ~InputData() = default;
        InputData(const InputData& ) = default;
        InputData& operator=(const InputData& ) = default;        

        void ReadConfiguration(const std::string& file);
        void SetTreeName(const std::string& treeName){fTreeName = treeName; };
        void AddFile(int run, const std::string& file);
        std::map<int, std::shared_ptr<TTree>> GetTrees() const {return fTrees;}
        std::shared_ptr<TTree> GetTree(int run) const {return fTrees.at(run);}
        std::vector<int> GetTreeList() const {return fRuns;}
        void GetEntry(int run, int entry);
        int GetNEntries(int run) const {return fTrees.at(run)->GetEntries();}
    };
}
#endif
