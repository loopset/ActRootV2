#ifndef ActInputData_h
#define ActInputData_h

#include "ActInputParser.h"

#include "TChain.h"
#include "TTree.h"
#include "TUUID.h"

#include <map>
#include <memory>
#include <string>
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
        std::map<int, std::vector<int>> fManualEntries;
        std::string fTreeName;
        std::string fFriendName;
        bool fHasFriend {};

    public:
        InputData() = default;
        InputData(const std::string& file, bool outputAsFriend = false);
        ~InputData() = default;
        InputData(const InputData&) = default;
        InputData& operator=(const InputData&) = default;

        void ReadConfiguration(const std::string& file, bool outputAsFriend = false);

        // Setters
        void SetTreeName(const std::string& treeName) { fTreeName = treeName; };
        void SetFriendName(const std::string& friendName) { fFriendName = friendName; }
        void SetRunListFrom(const std::string& listfile);

        // Getters
        std::map<int, std::shared_ptr<TTree>> GetTrees() const { return fTrees; }
        std::shared_ptr<TTree> GetTree(int run) const { return fTrees.at(run); }
        std::vector<int> GetTreeList() const { return fRuns; }
        void GetEntry(int run, int entry);
        int GetNEntries(int run) const { return fTrees.at(run)->GetEntries(); }
        const std::map<int, std::vector<int>>& GetManualEntries() const { return fManualEntries; }

        // Close files once processed
        void Close(int run);

    private:
        void AddFile(int run, const std::string& file);
        void AddFriend(std::shared_ptr<InputBlock> fb);
        void AddManualEntries(const std::string& file);
    };
} // namespace ActRoot
#endif
