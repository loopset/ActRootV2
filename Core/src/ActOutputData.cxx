#include "ActOutputData.h"

#include "ActColors.h"
#include "ActInputParser.h"

#include "TDirectory.h"
#include "TFile.h"
#include "TMacro.h"
#include "TSystem.h"
#include "TTree.h"

#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

void ActRoot::OutputData::ParseBlock(ActRoot::BlockPtr block)
{
    // Each block should consist of:
    // 1-> TTree name
    // 2-> Path to folder (abs or relative)
    // 3-> File name BEGIN
    // 4-> File name END (optional)
    // The input will read a file: /path/to/folder/BEGIN{%.4d of run}END.root

    // 1
    fTreeName = block->GetString("TreeName");
    // 2
    auto path {block->GetString("Path")};
    // Expand it with ROOT
    fPath = gSystem->ExpandPathName(path.c_str());
    // 3
    fBegin = block->GetString("Begin");
    // 4
    if(block->CheckTokenExists("End", true))
        fEnd = block->GetString("End");
}

void ActRoot::OutputData::Init(const std::set<int>& runs, bool print)
{
    fRuns = runs;
    // Assert AddOutput was called before
    if(fTreeName.length() < 1)
        throw std::runtime_error("OutputData::Init(): called without inner parameters set");

    for(const auto& run : runs)
    {
        std::string filename {fPath + fBegin + TString::Format("%04d", run) + fEnd + ".root"};
        // Print
        if(print)
        {
            std::cout << BOLDCYAN << "OutputData: saving " << fTreeName << " tree in file" << '\n';
            std::cout << "  " << filename << RESET << '\n';
        }
        // Init
        fFiles[run] = std::make_shared<TFile>(filename.c_str(), "recreate", "", 505); // RECREATE for output
        // INFO: Explanation for compression factor: 505
        // 5 (new ZTSD algorithm, recommended by ROOT)05(level 5, also recommended in the forum)
        // ZSTD requires ROOT > 6.20 to work but it creates much more reduced files. The decompression speed
        // is faster than the ZLIB algorithm used by default (101 option)
        fTrees[run] = std::make_shared<TTree>(fTreeName.c_str(), "An ACTAR TPC tree created with ActRoot");
    }
}

void ActRoot::OutputData::Fill(int run)
{
    fTrees[run]->Fill();
}

void ActRoot::OutputData::Close(int run)
{
    // Write to file assigned to tree in run
    // option kWriteDelete erases previous cycle metadata
    // keeping only the highest
    fFiles[run]->Write();
    // Close is implicitily called at reset
    fTrees[run].reset();
    fFiles[run].reset();
}

void ActRoot::OutputData::WriteMetadata(const std::string& file, const std::string& description)
{
    // Store metadata as TMacro class, which has all the functionality required
    TMacro mcr {file.c_str(), description.c_str()};
    // mcr.ReadFile(file.c_str()); // not necessary since this constructor already reads the filter
    // But we need to reset name (NOT with the new name convention)
    // auto idx {file.find_last_of("/")};
    // mcr.SetTitle(file.substr(idx + 1).c_str());
    // Write
    for(auto& [_, file] : fFiles)
        file->WriteObject(&mcr, mcr.GetName());
}
