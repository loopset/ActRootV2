#include "ActSilSpecs.h"

#include "ActColors.h"
#include "ActInputParser.h"

#include <iostream>
#include <memory>
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
    std::cout << "-> idx : <X || Y, Z>" << '\n';
    for(const auto& [idx, pair] : fPlacements)
        std::cout << "   " << idx << " : <" << pair.first << ", " << pair.second << ">" << '\n';
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
    auto allKeys {block->GetAllReadValues()};
    for(const auto& [key, vals] : allKeys)
    {
        if(!(key.rfind("i", 0) == 0)) // check this is an i-like command to add placements
            continue;
        auto idx {std::stoi(key.substr(1))};
        auto pair {block->GetDoubleVector(key)};
        fPlacements[idx] = {pair[0], pair[1]};
    }
    // Add offset and normal vector
    // 1-> Point aka offset
    if(block->CheckTokenExists("Point"))
    {
        auto aux {block->GetDoubleVector("Point")};
        fPoint = {aux[0], aux[1], aux[2]};
    }
    // 2-> Plane normal
    if(block->CheckTokenExists("Normal"))
    {
        auto aux {block->GetDoubleVector("Normal")};
        fNormal = {aux[0], aux[1], aux[2]};
    }
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
