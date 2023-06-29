#include "InputIterator.h"

#include "InputData.h"
#include "TPCData.h"

#include <map>
#include <utility>
#include <iostream>

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

////////////////////////////////////////////////////////////////////////////////////////////////////////

ActRoot::InputWrapper::InputWrapper(ActRoot::InputData* input)
    : fInput(input), fIt(ActRoot::InputIterator(input)), fData(new ActRoot::TPCData)
{
}

bool ActRoot::InputWrapper::GoNext()
{
    auto [runBef, _] = fIt.GetCurrentRunEntry();
    auto ok = fIt.Next();
    auto [run, entry] = fIt.GetCurrentRunEntry();
    if(!ok)
        return ok;
    if(runBef != run)
    {
        fInput->GetTree(run)->SetBranchAddress("data", &fData);
    }
    fInput->GetEntry(run, entry);
    return ok;
}

bool ActRoot::InputWrapper::GoPrevious()
{
    auto [runBef, _] = fIt.GetCurrentRunEntry();
    auto ok = fIt.Previous();
    auto [run, entry] = fIt.GetCurrentRunEntry();
    if(!ok)
        return ok;
    if(runBef != run)
    {
        fInput->GetTree(run)->SetBranchAddress("data", &fData);
    }
    fInput->GetEntry(run, entry);
    return ok;
}

bool ActRoot::InputWrapper::GoTo(int run, int entry)
{
    auto [runBef, _] = fIt.GetCurrentRunEntry();
    auto ok = fIt.GoTo(run, entry);
    if(!ok)
        return ok;
    if(runBef != run)
    {
        fInput->GetTree(run)->SetBranchAddress("data", &fData);
    }
    fInput->GetEntry(run, entry);
    return ok;
}
