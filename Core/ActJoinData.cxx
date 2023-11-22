#include "ActJoinData.h"

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
    auto runs {in->GetIntVector("FileList")};

    // Delete any excluded run from list
    if(in->CheckTokenExists("ExcludeList"))
    {
        auto toDelete {in->GetIntVector("ExcludeList")};
        for(const auto& run : toDelete)
        {
            auto it {std::find(runs.begin(), runs.end(), run)};
            if(it != runs.end())
                runs.erase(it);
        }
    }

    // Init classes
    fChain = std::make_shared<TChain>(tree.c_str());
    // Append files
    for(const auto& run : runs)
    {
        auto fullname {TString::Format("%s%s%04d%s.root", path.c_str(), name.c_str(), run, end.c_str())};
        fChain->Add(fullname);
    }
}
