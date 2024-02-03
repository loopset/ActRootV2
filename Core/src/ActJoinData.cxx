#include "ActJoinData.h"

#include "ActColors.h"
#include "ActInputParser.h"

#include "TChain.h"
#include "TString.h"

#include <algorithm>
#include <memory>
#include <string>

ActRoot::JoinData::JoinData(const std::string& file)
{
    ReadFile(file);
}

ActRoot::JoinData::JoinData(const std::string& runfile, const std::string& pathfile)
{
    // 1-> Set run list from specified file
    SetRunListFrom(runfile);
    // 2-> Get file name template from the other
    ReadFile(pathfile);
}

void ActRoot::JoinData::ReadFile(const std::string& file)
{
    ActRoot::InputParser parser {file};

    // Get output data
    auto out {parser.GetBlock("OutputData")};
    auto tree {out->GetString("TreeName")};
    auto path {out->GetString("FilePath")};
    auto name {out->GetString("FileBegin")};
    std::string end {};
    if(out->CheckTokenExists("FileEnd", true))
        end = out->GetString("FileEnd");

    // Get input data to get run list
    auto in {parser.GetBlock("InputData")};
    if(fRuns.empty())
        fRuns = in->GetIntVector("FileList");

    // Delete any excluded run from list
    if(in->CheckTokenExists("ExcludeList", true))
    {
        auto toDelete {in->GetIntVector("ExcludeList")};
        for(const auto& run : toDelete)
        {
            auto it {std::find(fRuns.begin(), fRuns.end(), run)};
            if(it != fRuns.end())
                fRuns.erase(it);
        }
    }

    // Init classes
    fChain = std::make_shared<TChain>(tree.c_str());
    // Append files
    for(const auto& run : fRuns)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), name.c_str(), run, end.c_str())};
        fChain->Add(fullname);
    }
}

void ActRoot::JoinData::SetRunListFrom(const std::string& file)
{
    // If fRuns is already populated, raise a warning
    if(fRuns.size() > 0)
    {
        std::cout << BOLDRED
                  << "InputData::SetRunListFrom: fRuns is already populated, so this will not take any effect" << '\n';
        return;
    }
    InputParser parser {file};
    auto block {parser.GetBlock("InputData")};
    if(block->CheckTokenExists("FileList"))
        fRuns = block->GetIntVector("FileList");
    if(block->CheckTokenExists("ExcludeList", true))
    {
        auto ex {block->GetIntVector("ExcludeList")};
        // Delete them from fRuns
        for(const auto& e : ex)
        {
            auto it {std::find(fRuns.begin(), fRuns.end(), e)};
            if(it != fRuns.end())
                fRuns.erase(it);
        }
    }
}
