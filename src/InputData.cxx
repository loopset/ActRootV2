#include "InputData.h"
#include "TChain.h"
#include <memory>
#include <string>

ActRoot::InputData::InputData(const std::string& treeName)
    : fTreeName(treeName)
{
    fChain.reset();
    fChain = std::make_shared<TChain>(fTreeName.c_str());
}

void ActRoot::InputData::SetTreeName(const std::string &treeName)
{
    fTreeName = treeName;
    fChain.reset();
    fChain = std::make_shared<TChain>(fTreeName.c_str());
}

void ActRoot::InputData::AddFile(const std::string &file)
{
    fChain->Add(file.data());
}
