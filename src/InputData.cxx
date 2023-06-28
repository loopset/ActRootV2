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

ActRoot::InputIterator::InputIterator(const InputData* input)
{
    //Init entries log
    for(const auto& [run, tree] : input->GetTrees())
    {
        fEntries[run] = tree->GetEntries();
    }
    //Init iterators
    fRunIt = fEntries.begin();
    fCurrentRun = -1;
    fCurrentEntry = -1;
}

bool ActRoot::InputIterator::Previous()
{
    //I dont want to mess up thins mixing both forward and reverse iterators
    //So I implement Previous() manually assuming we will use more Next()
    //Works fine but for sure we could have improved its implementation
    if(fCurrentRun == -1)//this would happen if we at the very first iteration previous instead of next
        return false;
    if(CheckEntryIsInRange(fCurrentRun, fCurrentEntry - 1))
    {
        fCurrentEntry -= 1;
    }
    else
    {
        auto it = fEntries.find(fCurrentRun - 1);
        if(it != fEntries.end())
        {
            fRunIt = it;
            fCurrentRun = fCurrentRun - 1;
            fCurrentEntry = fEntries[fCurrentRun] - 1;
        }
        else
            return false;//reached end of database
    }
    //std::cout<<"Run = "<<fRunIt->first<<" entry = "<<fCurrentEntry<<'\n';
    return true;
}

bool ActRoot::InputIterator::Next()
{
    //Increase entry
    for(; fRunIt != fEntries.end(); fRunIt++)
    {
        fCurrentRun = fRunIt->first;
        if(CheckEntryIsInRange(fRunIt->first, fCurrentEntry + 1))
        {
            fCurrentEntry += 1;
            break;
        }
        fCurrentEntry = -1;
    }
    //Check if it is last entry
    bool reachedEnd {fRunIt == fEntries.end()};
    if(reachedEnd)
        return false;
    //std::cout<<"Run = "<<fRunIt->first<<" entry = "<<fCurrentEntry<<'\n';
    return true;
}

bool ActRoot::InputIterator::CheckEntryIsInRange(int run, int entry)
{
    return (entry > -1 && entry < fEntries[run]);
}

bool ActRoot::InputIterator::GoTo(int run, int entry)
{
    auto it = fEntries.find(run);
    if(it != fEntries.end() && CheckEntryIsInRange(run, entry))
    {
        fRunIt = it;
        fCurrentRun = run;
        fCurrentEntry = entry;
        return true;
    }
    else
    {
        std::cout<<"InpuIterator::GoTo received wrong run or entry"<<'\n';
        return false;
    }
}
