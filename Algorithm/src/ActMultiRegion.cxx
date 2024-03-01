#include "ActMultiRegion.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActRegion.h"

#include <iostream>
#include <stdexcept>
#include <string>

ActAlgorithm::MultiRegion::MultiRegion()
{
    fIsVerbose = ActRoot::Options::GetInstance()->GetIsVerbose();
}

void ActAlgorithm::MultiRegion::AddRegion(unsigned int r, const std::vector<double>& vec)
{
    // Assert right dimension
    if(vec.size() != 4)
        throw std::runtime_error("MultiRegion::AddRegion(): vec in config file for idx " + std::to_string(r) +
                                 " has size != 4 required for 2D");
    RegionType type;
    if(r == 0)
        type = RegionType::EBeam;
    else if(r == 1)
        type = RegionType::ELight;
    else if(r == 2)
        type = RegionType::EHeavy;
    else
        type = RegionType::ENone;
    fRegions[type] = {vec[0], vec[1], vec[2], vec[3]};
}

void ActAlgorithm::MultiRegion::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "multiregion.conf";
    // Parse
    ActRoot::InputParser parser {conf};
    auto mr {parser.GetBlock("MultiRegion")};
    // Add regions
    auto regions {mr->GetMappedValuesVectorOf<double>("r")};
    for(const auto& [idx, vec] : regions)
        AddRegion(idx, vec);
}

void ActAlgorithm::MultiRegion::Run() {}

std::string ActAlgorithm::MultiRegion::RegionTypeToStr(const RegionType& r) const
{
    if(r == RegionType::EBeam)
        return "Beam";
    if(r == RegionType::ELight)
        return "Light";
    if(r == RegionType::EHeavy)
        return "Heavy";
    if(r == RegionType::ENone)
        return "None";
    return "";
}

void ActAlgorithm::MultiRegion::Print() const
{
    std::cout << BOLDCYAN << "**** MultiRegion ****" << '\n';
    for(const auto& [type, r] : fRegions)
    {
        std::cout << "-> Region : " << RegionTypeToStr(type) << '\n';
        std::cout << "   ";
        r.Print();
    }
    std::cout << "******************************" << RESET << '\n';
}

void ActAlgorithm::MultiRegion::PrintReports() const {}
