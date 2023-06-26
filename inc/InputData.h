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
    class InputData
    {
    private:
        std::vector<int> fRuns;
        std::map<int, std::shared_ptr<TFile>> fFiles;
        std::map<int, std::shared_ptr<TTree>> fTrees;
        //std::shared_ptr<TChain> fChain;
        std::string fTreeName;
        
    public:
        InputData() = default;
        InputData(const std::string& treeName);
        ~InputData() = default;

        void ReadConfiguration(const std::string& file);
        void SetTreeName(const std::string& treeName);
        void AddFile(int run, const std::string& file);
        std::map<int, std::shared_ptr<TTree>> GetTrees() const {return fTrees;}
        //std::shared_ptr<TChain> GetChain() { return fChain; }
    };
}
#endif
