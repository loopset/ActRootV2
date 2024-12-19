#include "ActDataManager.h"

#include "ActInputData.h"
#include "ActInputParser.h"
#include "ActOutputData.h"
#include "ActTypes.h"

#include "TChain.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

ActRoot::DataManager::DataManager(const std::string& file, ModeType mode) : fMode(mode)
{
    ReadDataFile(file);
}

void ActRoot::DataManager::ReadDataFile(const std::string& file)
{
    InputParser parser {file};
    for(auto& bstr : parser.GetBlockHeaders())
    {
        auto block {parser.GetBlock(bstr)};
        if(bstr == "DataManager")
            ParseManagerBlock(block);
        else
            fBlocks[bstr] = block;
    }
}

void ActRoot::DataManager::ParseManagerBlock(ActRoot::BlockPtr block)
{
    // This block contains
    // 1-> Run list
    // 2-> Exclude list of runs, to skip certains runs in ... expansion
    // 3-> Manual entries file to InputData
    //
    // 1
    auto runs {block->GetIntVector("Runs")};
    fRuns.insert(runs.begin(), runs.end());
    // 2
    if(block->CheckTokenExists("Exclude", true))
    {
        auto exclude {block->GetIntVector("Exclude")};
        fExludeList.insert(exclude.begin(), exclude.end());
        for(const int& run : fExludeList)
            if(auto it {fRuns.find(run)}; it != fRuns.end())
                fRuns.erase(it);
    }
    // 3
    if(block->CheckTokenExists("Manual", true))
        fManual = block->GetString("Manual");
}

void ActRoot::DataManager::SetRuns(int low, int up)
{
    // Clean file runs list
    fRuns.clear();
    // Generate
    for(int i = low; i <= up; i++)
    {
        // Check if is in excluded list
        if(fExludeList.find(i) != fExludeList.end())
            continue;
        else
            fRuns.insert(i);
    }
}

ActRoot::BlockPtr ActRoot::DataManager::CheckAndGet(const std::string& name)
{
    if(fBlocks.count(name))
        return fBlocks[name];
    else
        throw std::runtime_error("DataManager::CheckAndGet(): could not locate " + name + " block");
}

void ActRoot::DataManager::SetInputData(InputData& in, ModeType mode)
{
    if(mode == ModeType::EReadTPC)
        in.AddInput(CheckAndGet("Raw"));
    else if(mode == ModeType::EReadSilMod)
        in.AddInput(CheckAndGet("Raw"));
    else if(mode == ModeType::EFilter)
        in.AddInput(CheckAndGet("Cluster"));
    else if(mode == ModeType::EMerge)
    {
        in.AddInput(CheckAndGet("Filter"));
        in.AddInput(CheckAndGet("Data"));
    }
    else if(mode == ModeType::EFilterMerge)
    {
        in.AddInput(CheckAndGet("Cluster"));
        in.AddInput(CheckAndGet("Data"));
    }
    else if(mode == ModeType::EGui)
    {
        in.AddInput(CheckAndGet("Cluster"));
        in.AddInput(CheckAndGet("Data"));
    }
    else if(mode == ModeType::ECorrect)
        in.AddInput(CheckAndGet("Merger"));
    else
        throw std::invalid_argument("DataManager::SetInputData(): mode not implemented yet");
}

void ActRoot::DataManager::SetOutputData(OutputData& out, ModeType mode)
{
    if(mode == ModeType::EReadTPC)
        out.AddOuput(CheckAndGet("Cluster"));
    else if(mode == ModeType::EReadSilMod)
        out.AddOuput(CheckAndGet("Data"));
    else if(mode == ModeType::EFilter)
        out.AddOuput(CheckAndGet("Filter"));
    else if(mode == ModeType::EMerge || mode == ModeType::EFilterMerge)
        out.AddOuput(CheckAndGet("Merger"));
    else if(mode == ModeType::EGui)
        ;
    else if(mode == ModeType::ECorrect)
        out.AddOuput(CheckAndGet("Corrector"));
    else
        throw std::invalid_argument("DataManager::GetOutput(): mode not implemented yet");
}

ActRoot::InputData ActRoot::DataManager::GetInput(ActRoot::ModeType mode)
{
    InputData in;
    SetInputData(in, mode);
    in.Init(fRuns);
    in.AddManualEntries(fManual);
    return std::move(in);
}

ActRoot::InputData ActRoot::DataManager::GetInputForThread(const std::set<int>& runs)
{
    InputData in;
    SetInputData(in, fMode);
    in.Init(runs, false);
    return std::move(in);
}

ActRoot::OutputData ActRoot::DataManager::GetOutputForThread(const std::set<int>& runs)
{
    OutputData out;
    SetOutputData(out, fMode);
    out.Init(runs, false);
    return std::move(out);
}

ActRoot::OutputData ActRoot::DataManager::GetOuput(ActRoot::ModeType mode)
{
    OutputData out;
    SetOutputData(out, mode);
    out.Init(fRuns);
    return std::move(out);
}

std::shared_ptr<TChain> ActRoot::DataManager::GetJoinedData(ActRoot::ModeType mode)
{
    InputData in;
    if(mode == ModeType::EReadTPC)
        in.AddInput(CheckAndGet("Cluster"));
    else if(mode == ModeType::EReadSilMod)
        in.AddInput(CheckAndGet("Data"));
    else if(mode == ModeType::EFilter)
        in.AddInput(CheckAndGet("Filter"));
    else if(mode == ModeType::EMerge)
        in.AddInput(CheckAndGet("Merger"));
    else if(mode == ModeType::ECorrect)
        in.AddInput(CheckAndGet("Corrector"));
    else
        throw std::runtime_error("DataManager::GetJoinedData(): no conversion out -> in for that mode");
    in.InitChain(fRuns);
    return std::move(in.GetChain());
}
