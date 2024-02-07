#include "ActDataManager.h"

#include "ActInputData.h"
#include "ActInputParser.h"
#include "ActOutputData.h"
#include "ActTypes.h"

#include <stdexcept>
#include <string>
#include <utility>

ActRoot::DataManager::DataManager(const std::string& file)
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
    auto runs {block->GetIntVector("RunList")};
    fRuns.insert(runs.begin(), runs.end());
    // 2
    if(block->CheckTokenExists("ExcludeList", true))
    {
        auto exclude {block->GetIntVector("ExcludeList")};
        std::set<int> excludeSet {exclude.begin(), exclude.end()};
        for(const int& run : excludeSet)
            if(auto it {fRuns.find(run)}; it != fRuns.end())
                fRuns.erase(it);
    }
    // 3
    if(block->CheckTokenExists("ManualEntries", true))
        fManual = block->GetString("ManualEntries");
}

ActRoot::BlockPtr ActRoot::DataManager::CheckAndGet(const std::string& name)
{
    if(fBlocks.count(name))
        return fBlocks[name];
    else
        throw std::runtime_error("DataManager::CheckAndGet(): could not locate " + name + " block");
}

ActRoot::InputData ActRoot::DataManager::GetInput(ActRoot::ModeType mode)
{
    InputData in;
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
    else if(mode == ModeType::EGui)
    {
        in.AddInput(CheckAndGet("Cluster"));
        in.AddInput(CheckAndGet("Data"));
    }
    else
        throw std::invalid_argument("DataManager::GetInput(): mode not implemented yet");
    in.Init(fRuns);
    in.AddManualEntries(fManual);
    return std::move(in);
}

ActRoot::OutputData ActRoot::DataManager::GetOuput(ActRoot::ModeType mode)
{
    OutputData out;
    if(mode == ModeType::EReadTPC)
        out.AddOuput(CheckAndGet("Cluster"));
    else if(mode == ModeType::EReadSilMod)
        out.AddOuput(CheckAndGet("Data"));
    else if(mode == ModeType::EFilter)
        out.AddOuput(CheckAndGet("Filter"));
    else if(mode == ModeType::EMerge)
        out.AddOuput(CheckAndGet("Merger"));
    else if(mode == ModeType::EGui)
        ;
    else
        throw std::invalid_argument("DataManager::GetOutput(): mode not implemented yet");
    out.Init(fRuns);
    return std::move(out);
}
