#ifndef ActCutsManager_h
#define ActCutsManager_h

#include <Rtypes.h>
#include <RtypesCore.h>

#include "TMath.h"
#include <TCutG.h>
#include <TFile.h>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ActRoot
{
template <typename T>
class CutsManager
{
private:
    std::unordered_map<T, TCutG*> fCuts {};

public:
    bool ReadCut(const T& key, std::string_view file, std::string_view name = "CUTG");
    void DrawCut(const T& key);
    void DrawAll();
    bool IsInside(const T& key, double x, double y);
    std::optional<T> GetKeyIsInside(double x, double y);
    TCutG* GetCut(const T& key);
    void SetLineAttributes(const T& key, Color_t color, Width_t width = 1);
    std::vector<T> GetListOfKeys() const;
    std::pair<double, double> GetXRange(const T& key) const { return GetRange(key, "x"); };
    std::pair<double, double> GetYRange(const T& key) const { return GetRange(key, "y"); };

private:
    std::pair<double, double> GetRange(const T& key, const std::string& dim) const;
};

template <typename T>
inline std::pair<double, double> CutsManager<T>::GetRange(const T& key, const std::string& dim) const
{
    int n {fCuts.at(key)->GetN()};
    double* ptr {};
    if(dim == "x")
        ptr = fCuts.at(key)->GetX();
    else
        ptr = fCuts.at(key)->GetY();
    return {TMath::MinElement(n, ptr), TMath::MaxElement(n, ptr)};
}

template <typename T>
inline bool CutsManager<T>::ReadCut(const T& key, std::string_view file, std::string_view name)
{
    auto* infile {new TFile(file.data())};
    if(infile->IsZombie())
    {
        std::cout << "CutsManager::ReadCut(): file " << file << " doesn't exist!" << '\n';
        fCuts[key] = nullptr;
        return false;
    }
    auto* cut {infile->Get<TCutG>(name.data())};
    if(!cut)
        throw std::runtime_error("CutsManager::ReadCut(): file " + std::string(file) +
                                 " doesn't contain a TCutG named " + std::string(name));
    fCuts[key] = cut;
    delete infile;
    return true;
}

template <typename T>
inline void CutsManager<T>::DrawCut(const T& key)
{
    if(fCuts[key])
    {
        fCuts[key]->SetLineColor(kRed);
        fCuts[key]->Draw("same");
    }
}

template <typename T>
inline void CutsManager<T>::DrawAll()
{
    for(auto& [key, cut] : fCuts)
    {
        if(cut)
            cut->Draw("same");
    }
}

template <typename T>
inline bool CutsManager<T>::IsInside(const T& key, double x, double y)
{
    if(fCuts[key])
        return fCuts[key]->IsInside(x, y);
    else
        return false;
}

template <typename T>
inline std::optional<T> CutsManager<T>::GetKeyIsInside(double x, double y)
{
    for(const auto& [key, cut] : fCuts)
    {
        if(cut)
            if(cut->IsInside(x, y))
                return std::optional<T>(key);
    }
    return std::nullopt;
}

template <typename T>
inline TCutG* CutsManager<T>::GetCut(const T& key)
{
    return fCuts[key];
}

template <typename T>
inline void CutsManager<T>::SetLineAttributes(const T& key, Color_t color, Width_t width)
{
    if(fCuts[key])
    {
        fCuts[key]->SetLineColor(color);
        fCuts[key]->SetLineWidth(width);
    }
}

template <typename T>
inline std::vector<T> CutsManager<T>::GetListOfKeys() const
{
    std::vector<T> ret;
    for(const auto& [key, _] : fCuts)
        ret.push_back(key);
    return ret;
}
} // namespace ActRoot
#endif
