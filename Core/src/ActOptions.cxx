#include "ActOptions.h"

#include "ActColors.h"
#include "ActTypes.h"

#include "TString.h"
#include "TSystem.h"

#include <ios>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

std::unordered_map<ActRoot::ModeType, std::string> ActRoot::Options::fModeTable = {
    {ModeType::ENone, "None"},     {ModeType::EReadTPC, "ReadTPC"}, {ModeType::EReadSilMod, "ReadSilMod"},
    {ModeType::EFilter, "Filter"}, {ModeType::EMerge, "Merger"},    {ModeType::ECorrect, "Correct"},
    {ModeType::EGui, "Visual"}};

std::shared_ptr<ActRoot::Options> ActRoot::Options::fInstance = nullptr;

ActRoot::Options::Options(int argc, char** argv)
{
    GetProjectDir();
    CheckConfigDirectory();
    Parse(argc, argv);
}

std::shared_ptr<ActRoot::Options> ActRoot::Options::GetInstance(int argc, char** argv)
{
    if(!fInstance)
        fInstance = std::shared_ptr<Options>(new Options(argc, argv));
    return fInstance;
}

ActRoot::ModeType ActRoot::Options::ConvertToMode(const std::string& mode)
{
    for(const auto& [key, val] : fModeTable)
        if(val == mode)
            return key;
    return ModeType::ENone;
}

void ActRoot::Options::CheckConfigDirectory()
{
    auto conf {GetConfigDir()};
    for(const auto& file : {&fDetFile, &fCalFile, &fDataFile})
    {
        auto full {conf + *file};
        if(gSystem->AccessPathName(full.c_str()))
        {
            std::cout << BOLDRED << "Options::CheckConfigDir(): could not open " + *file + " in config dir" << '\n';
            std::cout << "  It must be set via a Setter method, relative to the config dir" << RESET << '\n';
        }
    }
}

ActRoot::ModeType ActRoot::Options::ReadFlagToMode(TString flag)
{
    flag.ToLower();
    if(flag == "tpc")
        return ModeType::EReadTPC;
    else if(flag == "sil" || flag == "mod" || flag == "silmod")
        return ModeType::EReadSilMod;
    else
        throw std::invalid_argument(
            "Options::ReadFlagToMode(): unrecongnized flag passed in read (-r) mode. Options are TPC and Sil/Mod");
}

void ActRoot::Options::Parse(int argc, char** argv)
{
    for(int i = 1; i < argc; i++)
    {
        TString arg {argv[i]};
        arg.ToLower();
        if(arg == "-h")
            Help();
        // Read mode
        else if(arg == "-r" && argc >= i + 1)
            fMode = ReadFlagToMode(argv[++i]);
        // Filter mode
        else if(arg == "-f")
            fMode = ModeType::EFilter;
        // Merge mode
        else if(arg == "-m")
            fMode = ModeType::EMerge;
        // Correct mode
        else if(arg == "-c")
            fMode = ModeType::ECorrect;
        // GUI mode
        else if(arg == "-gui")
            fMode = ModeType::EGui;
        // MT or ST
        else if(arg == "-st")
            fIsMT = false;
        else if(arg == "-mt")
            fIsMT = true;
        // Detector file inside configs dir
        else if(arg == "-det" && argc >= i + 1)
            fDetFile = argv[++i];
        // Calibration file inside configs dir
        else if(arg == "-cal" && argc >= i + 1)
            fCalFile = argv[++i];
        // Data configuration inside configs dir
        else if(arg == "-runs" && argc >= i + 1)
            fDataFile = argv[++i];
        // Verbose mode
        else if(arg == "-v")
            fIsVerbose = true;
        else
            throw std::invalid_argument("Options::Parse(): invalid argument : " + arg);
    }
}

void ActRoot::Options::Help() const
{
    std::cout << BOLDCYAN << "---- ActRoot::Options help ----" << '\n';
    std::cout << "List of valid arguments is : " << '\n';
    std::cout << "-h : Displays this help" << '\n';
    std::cout << "-r TPC | Sil·Mod : Reads TPC or Sil and Modular data" << '\n';
    std::cout << "-f : Performs filter operation at first stage" << '\n';
    std::cout << "-m : Runs merger detector" << '\n';
    std::cout << "-c : Performs filter operation at second stage: corrects MergerData" << '\n';
    std::cout << "-gui : Set visual mode (only valid for actplot)" << '\n';
    std::cout << "-det file.detector : Sets detector config file" << '\n';
    std::cout << "-cal file.calibrations : Sets calibrations for detectors" << '\n';
    std::cout << "-runs output.runs : Sets data flow configuration" << '\n';
    std::cout << "-mt or -st : Enables MT or ST mode" << '\n';
    std::cout << "-v : Enables verbose mode for algorithms" << '\n';
    std::cout << "--------------------" << RESET << '\n';
}

void ActRoot::Options::Print() const
{
    std::cout << BOLDCYAN << "---- ActRoot::Options ----" << '\n';
    std::cout << "Loaded options are : " << '\n';
    std::cout << "-> Verbose      : " << std::boolalpha << fIsVerbose << '\n';
    std::cout << "-> Multithread  : " << std::boolalpha << fIsMT << '\n';
    std::cout << "-> Mode         : " << GetModeStr() << '\n';
    std::cout << "-> Detector     : " << fDetFile << '\n';
    std::cout << "-> Calibrations : " << fCalFile << '\n';
    std::cout << "-> Data         : " << fDataFile << '\n';
    std::cout << "--------------------" << RESET << '\n';
}

std::string ActRoot::Options::GetProjectDir() const
{
    std::string pwd {gSystem->pwd()};
    if(gSystem->AccessPathName((pwd + "/configs").c_str()))
        throw std::runtime_error("ActRoot::Options::GetProjectDir() : " + pwd + " does not contain a configs/ dir");
    return pwd;
}
