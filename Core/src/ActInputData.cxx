#include "ActInputData.h"

#include "ActColors.h"
#include "ActInputParser.h"

#include "TFile.h"
#include "TString.h"
#include "TSystem.h"
#include "TTree.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


void ActRoot::InputData::ParseBlock(ActRoot::BlockPtr block)
{
    // Each block should consist of:
    // 1-> TTree name
    // 2-> Path to folder (abs or relative)
    // 3-> File name BEGIN
    // 4-> File name END (optional)
    // The input will read a file: /path/to/folder/BEGIN{%.4d of run}END.root

    // 1
    fTreeNames.push_back(block->GetString("TreeName"));
    // 2
    auto path {block->GetString("Path")};
    // Expand it with ROOT
    fPaths.push_back(gSystem->ExpandPathName(path.c_str()));
    // 3
    fBegins.push_back(block->GetString("Begin"));
    // 4
    if(block->CheckTokenExists("End", true))
        fEnds.push_back(block->GetString("End"));
    else
        fEnds.push_back("");
}

void ActRoot::InputData::CheckFileExists(const std::string& file)
{
    if(gSystem->AccessPathName(file.c_str()))
        throw std::runtime_error("InputData: file " + file + " does not exist");
}

void ActRoot::InputData::CheckTreeExists(std::shared_ptr<TTree> tree, int i)
{
    if(!tree)
        throw std::runtime_error("InputData: tree " + fTreeNames[i] + " could not be opened");
}

void ActRoot::InputData::Init(const std::set<int>& runs)
{
    fRuns = runs;
    // Assert that we have at least 1 input
    if(fTreeNames.size() < 1)
        throw std::runtime_error("InputData::Init(): size of internal vectors < 1 -> No inputs to read!");
    for(int in = 0; in < fTreeNames.size(); in++)
    {
        for(const auto& run : runs)
        {
            std::string filename {fPaths[in] + fBegins[in] + TString::Format("%04d", run) + fEnds[in] + ".root"};
            CheckFileExists(filename);
            // Print!
            std::cout << BOLDYELLOW << "InputData: reading " << fTreeNames[in] << " tree in file " << '\n';
            std::cout << "  " << filename << RESET << '\n';
            // Init
            if(in == 0) // Init pointers, this is set as the main input!
            {
                fFiles[run] = std::make_shared<TFile>(filename.c_str()); // READ mode by default
                fTrees[run] = std::shared_ptr<TTree>(fFiles[run]->Get<TTree>(fTreeNames[in].c_str()));
                CheckTreeExists(fTrees[run], in);
            }
            else // add as friend!
                fTrees[run]->AddFriend(fTreeNames[in].c_str(), filename.c_str());
        }
    }
}
void ActRoot::InputData::GetEntry(int run, int entry)
{
    fTrees[run]->GetEntry(entry);
}

void ActRoot::InputData::AddManualEntries(const std::string& file)
{
    if(file.length() == 0)
        return;
    // Read file
    std::ifstream streamer {file.c_str()};
    if(!streamer)
        throw std::runtime_error("InputData::AddManualEntries(): could not open file " + file);
    std::cout << BOLDGREEN << "InputData::AddManualEntries(): from file " + file << RESET << '\n';
    int run {};
    int entry {};
    while(streamer >> run >> entry)
    {
        // Check if run is in list
        bool isKnown {fFiles.count(run) > 0};
        if(!isKnown)
            continue;
        // Add to map
        fManualEntries[run].push_back(entry);
    }
    streamer.close();
    // Sort them in increasing order of entry
    for(auto& [run, vec] : fManualEntries)
        std::sort(vec.begin(), vec.end());
}
void ActRoot::InputData::Close(int run)
{
    fTrees[run].reset();
    fFiles[run].reset();
}
