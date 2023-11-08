#include "ActInputData.h"

#include "ActInputParser.h"

#include "TFile.h"
#include "TString.h"
#include "TTree.h"

#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

ActRoot::InputData::InputData(const std::string& file)
{
    ReadConfiguration(file);
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

void ActRoot::InputData::ReadConfiguration(const std::string& file)
{
    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("InputData")};
    // Keys available in file
    std::vector<std::string> keys {"TreeName", "FilePath", "FileBegin", "FileList", "FileEnd", "HasFriend"};
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