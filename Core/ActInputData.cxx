#include "ActInputData.h"

#include "ActColors.h"
#include "ActInputParser.h"

#include "TFile.h"
#include "TString.h"
#include "TTree.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

ActRoot::InputData::InputData(const std::string& file, bool outputAsFriend)
{
    ReadConfiguration(file, outputAsFriend);
}

void ActRoot::InputData::AddFile(int run, const std::string& file)
{
    if(fTreeName.empty())
        throw std::runtime_error("No TreeName given to ActRoot::InputData!");

    std::cout << "Adding TTree " << fTreeName << " for run " << run << " in file " << '\n';
    std::cout << "  " << file << '\n';
    fFiles[run] = std::make_shared<TFile>(file.data());
    if(fFiles[run]->IsZombie())
        throw std::runtime_error("This file isZombie!");
    fTrees[run] = std::shared_ptr<TTree>(fFiles[run]->Get<TTree>(fTreeName.data()));
}

void ActRoot::InputData::ReadConfiguration(const std::string& file, bool outputAsFriend)
{
    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("InputData")};
    // Keys available in file
    std::vector<std::string> keys {"TreeName", "FilePath",  "FileBegin",    "FileList",
                                   "FileEnd",  "HasFriend", "ManualEntries"};
    // Set input data
    if(block->CheckTokenExists("TreeName"))
        fTreeName = block->GetString("TreeName");
    std::string path {};
    if(block->CheckTokenExists("FilePath"))
        path = block->GetString("FilePath");
    std::string begin {};
    if(block->CheckTokenExists("FileBegin"))
        begin = block->GetString("FileBegin");
    // If run list was set manually, ignore the one in file
    if(fRuns.empty())
    {
        if(block->CheckTokenExists("FileList"))
            fRuns = block->GetIntVector("FileList");
    }
    std::string end {};
    if(block->CheckTokenExists("FileEnd", true))
        end = block->GetString("FileEnd");
    if(block->CheckTokenExists("HasFriend", true))
        fHasFriend = block->GetBool("HasFriend");
    // Add option to exclude some runs from run list
    if(block->CheckTokenExists("ExcludeList", true))
    {
        auto ex {block->GetIntVector("ExcludeList")};
        // Delete them from fRuns
        for(const auto& e : ex)
        {
            auto it {std::find(fRuns.begin(), fRuns.end(), e)};
            if(it != fRuns.end())
                fRuns.erase(it);
        }
    }
    // Add files!
    for(const auto& run : fRuns)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), begin.c_str(), run, end.c_str())};
        AddFile(run, fullname.Data());
    }
    // Add FriendData object if desired
    if(fHasFriend)
        AddFriend(parser.GetBlock("FriendData"));
    // Use output olso as a friend (only accesible in macro; no file key to do this)
    if(outputAsFriend)
        AddFriend(parser.GetBlock("OutputData"));
    // Use manual entries mode
    if(block->CheckTokenExists("ManualEntries", true))
        AddManualEntries(block->GetString("ManualEntries"));
}

void ActRoot::InputData::GetEntry(int run, int entry)
{
    fTrees[run]->GetEntry(entry);
}

void ActRoot::InputData::AddFriend(std::shared_ptr<InputBlock> fb)
{
    // Process keys in the same way as InputData
    if(fb->CheckTokenExists("TreeName"))
        fFriendName = fb->GetString("TreeName");
    std::string path {};
    if(fb->CheckTokenExists("FilePath"))
        path = fb->GetString("FilePath");
    std::string begin {};
    if(fb->CheckTokenExists("FileBegin"))
        begin = fb->GetString("FileBegin");
    std::string end {};
    if(fb->CheckTokenExists("FileEnd", true))
        end = fb->GetString("FileEnd");
    for(const auto& run : fRuns)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), begin.c_str(), run, end.c_str())};
        fTrees[run]->AddFriend(fFriendName.c_str(), fullname.Data());
        std::cout << "Adding Friend TTree " << fFriendName << " in " << fullname << '\n';
    }
}

void ActRoot::InputData::AddManualEntries(const std::string& file)
{
    // Read file
    std::ifstream streamer {file.c_str()};
    if(!streamer)
        throw std::runtime_error("Could not open InputData::ManualEntries file");
    int run {};
    int entry {};
    while(streamer >> run >> entry)
    {
        // Check if run is in list
        bool isKnown {std::find(fRuns.begin(), fRuns.end(), run) != fRuns.end()};
        if(!isKnown)
            continue;
        // Add to map
        fManualEntries[run].push_back(entry);
    }
    streamer.close();
}
void ActRoot::InputData::Close(int run)
{
    fTrees[run].reset();
    fFiles[run].reset();
}

void ActRoot::InputData::SetRunListFrom(const std::string& listfile)
{
    // If fRuns is already populated, raise a warning
    if(fRuns.size() > 0)
    {
        std::cout << BOLDRED
                  << "InputData::SetRunListFrom: fRuns is already populated, so this will not take any effect" << '\n';
        return;
    }
    InputParser parser {listfile};
    auto block {parser.GetBlock("InputData")};
    if(block->CheckTokenExists("FileList"))
        fRuns = block->GetIntVector("FileList");
    if(block->CheckTokenExists("ExcludeList", true))
    {
        auto ex {block->GetIntVector("ExcludeList")};
        // Delete them from fRuns
        for(const auto& e : ex)
        {
            auto it {std::find(fRuns.begin(), fRuns.end(), e)};
            if(it != fRuns.end())
                fRuns.erase(it);
        }
    }
}
