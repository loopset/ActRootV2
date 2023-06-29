#include "InputData.h"
#include "InputParser.h"
#include "TChain.h"
#include "TString.h"
#include "TTree.h"
#include "TFile.h"
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void ActRoot::InputData::AddFile(int run, const std::string &file)
{
    if(fTreeName.empty())
        throw std::runtime_error("No TreeName given to ActRoot::InputData!");
    
    std::cout<<"Adding TTree "<<fTreeName<<" for run "<<run<<" in file "<<'\n';
    std::cout<<"  "<<file<<'\n';
    fFiles[run] = std::make_shared<TFile>(file.data());
    if(fFiles[run]->IsZombie())
        throw std::runtime_error("This file isZombie!");
    fTrees[run] = std::shared_ptr<TTree>(fFiles[run]->Get<TTree>(fTreeName.data()));
}

void ActRoot::InputData::ReadConfiguration(const std::string &file)
{
    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("InputData")};
    //Keys available in file
    std::vector<std::string> keys {"TreeName", "FilePath", "FileBegin", "FileList", "FileEnd"};
    //Set input data
    auto tree {block->GetString("TreeName")};
    SetTreeName(tree);
    auto path {block->GetString("FilePath")};
    auto name {block->GetString("FileBegin")};
    fRuns = block->GetIntVector("FileList");
    std::string end {};
    if(block->CheckTokenExists("FileEnd", true))
        end = block->GetString("FileEnd");
    
    for(const auto& run : fRuns)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), name.c_str(), run, end.c_str())};
        AddFile(run, fullname.Data());
    }
}

void ActRoot::InputData::GetEntry(int run, int entry)
{
    fTrees[run]->GetEntry(entry);
}
