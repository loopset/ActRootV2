#include "ActInputIterator.h"

#include "ActColors.h"
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
#include <string>
#include <utility>
#include <vector>

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
        {
            std::cout << BOLDYELLOW
                      << "InpuIterator::GetManualPrev(): previus cannot reverse, reached begin in manual entries"
                      << RESET << '\n';
            fManIdx = -1;
            return {-1, -1};
        }
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
        if(run == -1 && entry == -1)
            return false; // reached beginning of entry list
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
        {
            std::cout << BOLDYELLOW
                      << "InputIterator::GetManualNext(): next cannot advance, reached end of manual entries" << RESET
                      << '\n';
            // Set manual index to end of previous run entry list (-1 will be added by Previous() call)
            fManIdx = std::prev(it)->second.size();
            return {-1, -1};
        }
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
        if(run == -1 && entry == -1) // reached end
            return false;
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

void ActRoot::InputIterator::Print() const
{
    std::cout << "···· InputIterator ····" << '\n';
    std::cout << " -> Run : " << fCurrentRun << '\n';
    std::cout << " -> Entry : " << fCurrentEntry << '\n';
    std::cout << "······························" << '\n';
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
ActRoot::InputWrapper::InputWrapper(InputData* input)
{
    fInput = input;
    fIt = InputIterator {fInput};

    // Init pointers
    fTPCData = new TPCData;
    fTPCClone = new TPCData;
    fTPCClone2 = new TPCData;
    fSilData = new SilData;
    fModularData = new ModularData;
    fMergerData = new MergerData;
}

ActRoot::InputWrapper::~InputWrapper()
{
    if(fTPCData)
        delete fTPCData;
    if(fTPCClone)
        delete fTPCClone;
    if(fTPCClone2)
        delete fTPCClone2;
    if(fSilData)
        delete fSilData;
    if(fModularData)
        delete fModularData;
    if(fMergerData)
        delete fMergerData;
}

void ActRoot::InputWrapper::GetEntry(int run, int entry)
{
    fInput->GetEntry(run, entry);
    // Reset not read from file class members
    std::vector<VData*> datas {fTPCData, fSilData, fModularData, fMergerData};
    for(auto* data : datas)
        if(data)
            data->ClearFilter();
    if(fTPCData && fTPCClone)
        *fTPCClone = *fTPCData;
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

void ActRoot::InputWrapper::ReGet()
{
    auto [run, entry] {fIt.GetCurrentRunEntry()};
    GetEntry(run, entry);
}

void ActRoot::InputWrapper::SetBranchAddress(int run)
{
    auto tree {fInput->GetTree(run)};
    // Set branch addresses if branches exists
    if(tree->FindBranch("TPCData"))
        tree->SetBranchAddress("TPCData", &fTPCData);
    if(tree->FindBranch("SilData"))
        tree->SetBranchAddress("SilData", &fSilData);
    if(tree->FindBranch("ModularData"))
        tree->SetBranchAddress("ModularData", &fModularData);
    if(tree->FindBranch("MergerData"))
        tree->SetBranchAddress("MergerData", &fMergerData);
}

void ActRoot::InputWrapper::CopyToClone2(ActRoot::TPCData* data)
{
    *fTPCClone2 = *data;
}
