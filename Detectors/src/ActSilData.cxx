#include "ActSilData.h"

#include "ActSilSpecs.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

std::vector<std::string> ActRoot::SilData::GetLayers() const
{
    std::vector<std::string> ret;
    for(const auto& [layer, vals] : fSiE)
        ret.push_back(layer);
    // Unordered map is an unsorted container
    // Here we sort by alphanumeric ordering
    std::sort(ret.begin(), ret.end());
    return ret;
}

void ActRoot::SilData::ApplyFinerThresholds(std::shared_ptr<ActPhysics::SilSpecs> specs)
{
    for(const auto& [key, energies] : fSiE)
    {
        const auto& layer {specs->GetLayer(key)};
        std::set<int, std::greater<int>> toDelete;
        for(int i = 0, size = energies.size(); i < size; i++)
        {
            if(!layer.ApplyThreshold(fSiN[key][i], energies[i]))
                toDelete.insert(i);
        }
        // Delete
        for(const auto& i : toDelete)
        {
            fSiE[key].erase(fSiE[key].begin() + i);
            fSiN[key].erase(fSiN[key].begin() + i);
        }
    }
}

void ActRoot::SilData::Clear()
{
    fSiE.clear();
    fSiN.clear();
}

void ActRoot::SilData::Print() const
{
    for(const auto& [key, vec] : fSiE)
    {
        std::cout << "==== SilData ====" << '\n';
        std::cout << "-- Layer " << key << '\n';
        for(int i = 0; i < vec.size(); i++)
        {
            std::cout << "SilN = " << fSiN.at(key).at(i) << " Val = " << vec[i] << " MeV" << '\n';
        }
    }
}
