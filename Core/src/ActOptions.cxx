#include "ActOptions.h"

#include "ActColors.h"
#include "ActTypes.h"

#include "TSystem.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

std::unordered_map<ActRoot::ModeType, std::string> ActRoot::Options::fModeTable = {
    {ModeType::ENone, "None"},     {ModeType::ECluster, "Cluster"}, {ModeType::EData, "Data"},
    {ModeType::EFilter, "Filter"}, {ModeType::EMerge, "Merger"},    {ModeType::ECorrect, "Correct"}};

std::shared_ptr<ActRoot::Options> ActRoot::Options::fInstance = nullptr;

ActRoot::Options::Options(int argc, char** argv)
{
    GetProjectDir();
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

void ActRoot::Options::Parse(int argc, char** argv)
{
    for(int i = 1; i < argc; i++)
    {
        std::string arg {argv[i]};
        if(arg == "-h")
            Help();
        else if(arg == "-m" && argc >= i + 1)
            fMode = ConvertToMode(argv[++i]);
        else if(arg == "-st")
            fIsMT = false;
        else if(arg == "-mt")
            fIsMT = true;
        else if(arg == "-d" && argc >= i + 1)
            fDetFile = argv[++i];
        else if(arg == "-c" && argc >= i + 1)
            fCalFile = argv[++i];
        else if((arg == "-in" || arg == "-r") && argc >= i + 1)
            fInFile = argv[++i];
        else if(arg == "-out" && argc >= i + 1)
            fOutFile = argv[++i];
        else
            throw std::invalid_argument("Options::Parse(): invalid argument : " + arg);
    }
}

void ActRoot::Options::Help() const
{
    std::cout << BOLDCYAN << "---- ActRoot::Options help ----" << '\n';
    std::cout << "List of valid arguments is : " << '\n';
    std::cout << "-h : Displays this help" << '\n';
    std::cout << "-d file.detector : Sets detector config file" << '\n';
    std::cout << "-c file.calibrations : Sets calibrations for detectors" << '\n';
    std::cout << "-mt or -st : Enables MT or ST mode" << '\n';
    std::cout << "-in input.runs : Sets input data" << '\n';
    std::cout << "-out output.runs : Sets output data" << '\n';
    std::cout << "--------------------" << RESET << '\n';
}

void ActRoot::Options::Print() const
{
    std::cout << BOLDCYAN << "---- ActRoot::Options ----" << '\n';
    std::cout << "Loaded options are : " << '\n';
    std::cout << "-> Mode         : " << GetModeStr() << '\n';
    std::cout << "-> Detector     : " << fDetFile << '\n';
    std::cout << "-> Calibrations : " << fCalFile << '\n';
    if(fIsMT)
        std::cout << "-> Threads      : multi" << '\n';
    else
        std::cout << "-> Threads      : single" << '\n';
    std::cout << "-> Input        : " << fInFile << '\n';
    std::cout << "-> Output       : " << fOutFile << '\n';
    std::cout << "--------------------" << RESET << '\n';
}

std::string ActRoot::Options::GetProjectDir() const
{
    std::string pwd {gSystem->pwd()};
    if(gSystem->AccessPathName((pwd + "/configs").c_str()))
        throw std::runtime_error("ActRoot::Options::GetProjectDir() : " + pwd + " does not contain a configs/ dir");
    return pwd;
}
