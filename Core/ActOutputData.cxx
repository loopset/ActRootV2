#include "ActOutputData.h"

#include "ActInputData.h"
#include "ActInputParser.h"

#include "TFile.h"
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

void ActRoot::OutputData::InitFile(int run, const std::string& file)
{
    if(fTreeName.empty())
        throw std::runtime_error("No TreeName given to ActRoot::OutputData!");
    
    std::cout<<"Initializing TTree "<<fTreeName<<" for run "<<run<<" in file "<<'\n';
    std::cout<<"  "<<file<<'\n';
    fFiles[run] = std::make_shared<TFile>(file.c_str(), "recreate");
    fTrees[run] = std::make_shared<TTree>(fTreeName.c_str(), "ActRoot event data");
}

void ActRoot::OutputData::ReadConfiguration(const std::string &file)
{
    if(fRuns.empty())
        throw std::runtime_error("Run list is empty. Pass InputData object to get same output data as input!");
    
    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("OutputData")};
    //Keys available in file
    std::vector<std::string> keys {"TreeName", "FilePath", "FileBegin", "FileEnd"};
    //Set output
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
