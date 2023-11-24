#include "ActInputIterator.h"

#include "ActInputData.h"
#include "ActMergerData.h"
#include "ActModularData.h"
#include "ActSilData.h"
#include "ActTPCData.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>

ActRoot::InputIterator::InputIterator(const InputData* input)
{
    // Init entries log, depeding on whether is manual or not
    fManual = input->GetManualEntries();
    if(fManual.size() > 0)
        fIsManual = true;
    for(const auto& [run, tree] : input->GetTrees())
    {
        if(fIsManual && (fManual.find(run) == fManual.end()))
            continue;
        fEntries[run] = tree->GetEntries();
    }
    // Init iterators
    fCurrentRun = -1;
    fCurrentEntry = -1;
    fLastRun = -1;
    fManIdx = -1;
}

std::pair<int, int> ActRoot::InputIterator::GetManualPrev()
{
    fManIdx -= 1;
    if(fManIdx < 0)
    {
        // find previous run
        auto it {fManual.find(fCurrentRun)};
        if(it == fManual.begin())
            throw std::runtime_error("Could not go past beginning of manual entries");
        else
        {
            it = std::prev(it);
            fCurrentRun = it->first;
            fManIdx = it->second.size() - 1;
        }
    }
    return {fCurrentRun, fManual[fCurrentRun][fManIdx]};
}

bool ActRoot::InputIterator::Previous()
{
    // I dont want to mess up thins mixing both forward and reverse iterators
    // So I implement Previous() manually assuming we will use more Next()
    // Works fine but for sure we could have improved its implementation
    if(fCurrentRun == -1) // this would happen if we at the very first iteration previous instead of next
        return false;
    if(fIsManual)
    {
        auto [run, entry] {GetManualPrev()};
        return GoTo(run, entry);
    }
    if(CheckEntryIsInRange(fCurrentRun, fCurrentEntry - 1))
    {
        fCurrentEntry -= 1;
    }
    else
    {
        auto it = fEntries.find(fCurrentRun - 1);
        if(it != fEntries.end())
        {
            fCurrentRun = fCurrentRun - 1;
            fCurrentEntry = fEntries[fCurrentRun] - 1;
        }
        else
            return false; // reached end of database
    }
    // std::cout<<"Run = "<<fRunIt->first<<" entry = "<<fCurrentEntry<<'\n';
    return true;
}

std::pair<int, int> ActRoot::InputIterator::GetManualNext()
{
    if(fManIdx == -1)
        fManIdx = 0;
    else
        fManIdx += 1;
    if(fManIdx >= fManual[fCurrentRun].size())
    {
        // find next run
        auto it {fManual.find(fCurrentRun)};
        it = std::next(it);
        if(it != fManual.end())
        {
            fCurrentRun = it->first;
            fManIdx = 0;
        }
        else
            throw std::runtime_error("Could not advance: reached end of manual entries");
    }
    return {fCurrentRun, fManual[fCurrentRun][fManIdx]};
}

bool ActRoot::InputIterator::Next()
{
    // Workaround: Idk what is going on with the fRunIt after initialization
    // because it goes undefined behaviour (but only when called in EventPainter)
    if(fCurrentRun == -1)
    {
        fCurrentRun = fEntries.begin()->first;
    }
    if(fIsManual)
    {
        auto [run, entry] {GetManualNext()};
        return GoTo(run, entry);
    }
    if(CheckEntryIsInRange(fCurrentRun, fCurrentEntry + 1))
    {
        fCurrentEntry += 1;
    }
    else
    {
        auto it = fEntries.find(fCurrentRun + 1);
        if(it != fEntries.end())
        {
            fCurrentRun = fCurrentRun + 1;
            fCurrentEntry = fEntries[fCurrentRun] - 1;
        }
        else
            return false;
    }
    return true;
}

bool ActRoot::InputIterator::CheckEntryIsInRange(int run, int entry)
{
    return (entry > -1 && entry < fEntries[run]);
}

bool ActRoot::InputIterator::GoTo(int run, int entry)
{
    // Store iterator status
    fLastRun = fCurrentRun;
    auto it = fEntries.find(run);
    if(it != fEntries.end() && CheckEntryIsInRange(run, entry))
    {
        fCurrentRun = run;
        fCurrentEntry = entry;
        if(fIsManual)
        {
            auto manIt {std::find(fManual[fCurrentRun].begin(), fManual[fCurrentRun].end(), entry)};
            fManIdx = std::distance(fManual[fCurrentRun].begin(), manIt);
        }
        return true;
    }
    else
    {
        std::cout << "InpuIterator::GoTo received wrong run or entry" << '\n';
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

ActRoot::InputWrapper::InputWrapper(ActRoot::InputData* input) : fInput(input), fIt(ActRoot::InputIterator(input)) {}

void ActRoot::InputWrapper::GetEntry(int run, int entry)
{
    fInput->GetEntry(run, entry);
    if(fTPCData)
    {
        fTPCClone.reset();
        fTPCClone = std::make_unique<TPCData>(*fTPCData);
    }
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
        SetBranchAddress(run);
    }
    GetEntry(run, entry);
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
        SetBranchAddress(run);
    }
    GetEntry(run, entry);
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
        SetBranchAddress(run);
    }
    GetEntry(run, entry);
    return ok;
}

void ActRoot::InputWrapper::SetBranchAddress(int run)
{
    auto tree {fInput->GetTree(run)};
    // Set branch addresses if branches exists
    if(tree->FindBranch("TPCData"))
    {
        if(!fTPCData)
            fTPCData = new TPCData;
        tree->SetBranchAddress("TPCData", &fTPCData);
    }
    if(tree->FindBranch("SilData"))
    {
        if(!fSilData)
            fSilData = new SilData;
        tree->SetBranchAddress("SilData", &fSilData);
    }
    if(tree->FindBranch("ModularData"))
    {
        if(!fModularData)
            fModularData = new ModularData;
        tree->SetBranchAddress("ModularData", &fModularData);
    }
    if(tree->FindBranch("MergerData"))
    {
        if(!fMergerData)
            fMergerData = new MergerData;
        tree->SetBranchAddress("MergerData", &fMergerData);
    }
}
