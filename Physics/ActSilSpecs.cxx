#include "ActSilSpecs.h"

#include "ActColors.h"
#include "ActInputParser.h"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

void ActPhysics::SilUnit::Print() const
{
    std::cout << "...................." << '\n';
    std::cout << "-> Height : " << fHeight << " mm" << '\n';
    std::cout << "-> Width  : " << fWidth << " mm" << '\n';
    std::cout << "-> Thick. : " << fThick << " mm" << '\n';
    std::cout << "...................." << '\n';
}

void ActPhysics::SilLayer::Print() const
{
    std::cout << "--------------------" << '\n';
    std::cout << "-> idx : <X || Y, Z> [mm] ; thresh [MeV]" << '\n';
    for(const auto& [idx, pair] : fPlacements)
        std::cout << "   " << idx << " : <" << pair.first << ", " << pair.second << "> ; " << fThresholds.at(idx)
                  << '\n';
    std::cout << "-> Point  : " << fPoint << " [pads]" << '\n';
    std::cout << "-> Normal : " << fNormal << '\n';
    std::cout << "-> Unit   : " << '\n';
    fUnit.Print();
    std::cout << "--------------------" << '\n';
}

void ActPhysics::SilSpecs::Print() const
{
    std::cout << BOLDYELLOW << "++++++++ SilGeo ++++++++" << '\n';
    for(const auto& [name, layer] : fLayers)
    {
        std::cout << "-> Layer : " << name << '\n';
        layer.Print();
        std::cout << std::endl;
    }
    std::cout << RESET;
}

void ActPhysics::SilLayer::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    std::vector<std::string> unitKeys {"Height", "Width", "Thickness"};
    std::vector<double> unitVals(unitKeys.size());
    for(int i = 0; i < unitKeys.size(); i++)
    {
        block->CheckTokenExists(unitKeys[i]);
        unitVals[i] = block->GetDouble(unitKeys[i]);
    }
    fUnit = SilUnit {unitVals[0], unitVals[1], unitVals[2]};
    // Get placements
    auto placements {block->GetMappedValuesVectorOf<double>("i")};
    for(const auto& [idx, vals] : placements)
    {
        fPlacements[idx] = {vals[0], vals[1]};
        fThresholds[idx] = 0.;
    }
    // Add offset and normal vector
    // 1-> Point aka offset
    if(block->CheckTokenExists("Point"))
    {
        auto aux {block->GetDoubleVector("Point")};
        fPoint = {(float)aux[0], (float)aux[1], (float)aux[2]};
    }
    // 2-> Plane normal
    if(block->CheckTokenExists("Normal"))
    {
        auto aux {block->GetDoubleVector("Normal")};
        fNormal = {(float)aux[0], (float)aux[1], (float)aux[2]};
    }
    // Thresholds
    if(block->CheckTokenExists("CommomThresh", true))
    {
        auto thresh {block->GetDouble("CommomThresh")};
        for(const auto& [idx, _] : fPlacements)
            fThresholds[idx] = thresh;
    }
    // Set thresholds
    auto threshs {block->GetMappedValuesAs<double>("t")};
    if(threshs.size() > 0)
    {
        for(const auto& [idx, _] : fPlacements)
        {
            try
            {
                fThresholds[idx] = threshs.at(idx);
            }
            catch(std::exception& e)
            {
                throw std::runtime_error("While reading independent threshs, missing t command for idx : " +
                                         std::to_string(idx));
            }
        }
    }
}

ActPhysics::SilLayer::XYZPoint
ActPhysics::SilLayer::GetSiliconPointOfTrack(const XYZPoint& otherPoint, const XYZVector& otherVec) const
{
    return otherPoint + (((fPoint - otherPoint).Dot(fNormal)) / otherVec.Dot(fNormal)) * otherVec;
}


void ActPhysics::SilSpecs::ReadFile(const std::string& file)
{
    ActRoot::InputParser parser {file};
    for(const auto& name : parser.GetBlockHeaders())
    {
        SilLayer layer;
        layer.ReadConfiguration(parser.GetBlock(name));
        fLayers[name] = layer;
    }
}
