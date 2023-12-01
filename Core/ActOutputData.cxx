#include "ActOutputData.h"

#include "ActInputData.h"
#include "ActInputParser.h"

#include "TDirectory.h"
#include "TFile.h"
#include "TMacro.h"
#include "TObject.h"
#include "TSystem.h"
#include "TTree.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

ActRoot::OutputData::OutputData(const InputData& input)
{
    SetSameTreesAsInput(input);
}

ActRoot::OutputData::OutputData(const std::string& file)
{
    ReadConfiguration(file);
}

void ActRoot::OutputData::InitFile(int run, const std::string& file)
{
    if(fTreeName.empty())
        throw std::runtime_error("No TreeName given to ActRoot::OutputData!");

    std::cout << "Initializing TTree " << fTreeName << " for run " << run << " in file " << '\n';
    std::cout << "  " << file << '\n';
    fFiles[run] = std::make_shared<TFile>(file.c_str(), "recreate");
    fTrees[run] = std::make_shared<TTree>(fTreeName.c_str(), "ActRoot event data");
}

void ActRoot::OutputData::ReadConfiguration(const std::string& file)
{
    if(fRuns.empty())
        throw std::runtime_error("Run list is empty. Pass InputData object to get same output data as input!");

    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("OutputData")};
    // Keys available in file
    std::vector<std::string> keys {"TreeName", "FilePath", "FileBegin", "FileEnd"};
    // Set output
    SetTreeName(block->GetString("TreeName"));
    auto path {block->GetString("FilePath")};
    auto begin {block->GetString("FileBegin")};
    std::string end {};
    if(block->CheckTokenExists("FileEnd", true))
        end = block->GetString("FileEnd");

    for(const auto& run : fRuns)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), begin.c_str(), run, end.c_str())};
        InitFile(run, fullname.Data());
    }
}

void ActRoot::OutputData::SetSameTreesAsInput(const InputData& input)
{
    fRuns = input.GetTreeList();
}

void ActRoot::OutputData::Fill(int run)
{
    fTrees[run]->Fill();
}

void ActRoot::OutputData::Close(int run)
{
    // Write to file assigned to tree in run
    // option kWriteDelete erases previous cycle metadata
    // keeping only the highest
    fFiles[run]->Write();
    // Delete pointers (automatically calls destructor)
    fFiles[run]->Close();
    fTrees[run].reset();
    fFiles[run].reset();
}

void ActRoot::OutputData::WriteMetadata(const std::string& file, const std::string& description)
{
    // Store metadata as TMacro class, which has all the functionality required
    TMacro mcr {file.c_str(), description.c_str()};
    // mcr.ReadFile(file.c_str()); // not necessary since this constructor already reads the filter
    // But we need to reset name (NOT with the new name convention)
    // auto idx {file.find_last_of("/")};
    // mcr.SetTitle(file.substr(idx + 1).c_str());
    // Write
    for(auto& [_, file] : fFiles)
        file->WriteObject(&mcr, mcr.GetName());
}
