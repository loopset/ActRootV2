#include "ActInputData.h"

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
    auto tree {block->GetString("TreeName")};
    fTreeName = tree;
    auto path {block->GetString("FilePath")};
    auto name {block->GetString("FileBegin")};
    fRuns = block->GetIntVector("FileList");
    std::string end {};
    if(block->CheckTokenExists("FileEnd", true))
        end = block->GetString("FileEnd");
    if(block->CheckTokenExists("HasFriend", true))
        fHasFriend = block->GetBool("HasFriend");
    for(const auto& run : fRuns)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), name.c_str(), run, end.c_str())};
        AddFile(run, fullname.Data());
    }
    if(fHasFriend)
        AddFriend(parser.GetBlock("FriendData"));
    if(outputAsFriend)
        AddFriend(parser.GetBlock("OutputData"));
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
    auto tree {fb->GetString("TreeName")};
    fFriendName = tree;
    auto path {fb->GetString("FilePath")};
    auto name {fb->GetString("FileBegin")};
    std::string end {};
    if(fb->CheckTokenExists("FileEnd", true))
        end = fb->GetString("FileEnd");
    for(const auto& run : fRuns)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), name.c_str(), run, end.c_str())};
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
