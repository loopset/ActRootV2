#include "ActSilSpecs.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActSilMatrix.h"
#include "ActUtils.h"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

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
        fNormal = fNormal.Unit(); // store as normal!
    }
    // 3-> Thresholds
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
    // 4-> Sil side to be stored as enum
    if(block->CheckTokenExists("Side"))
    {
        auto str {block->GetString("Side")};
        str = ActRoot::StripSpaces(str);
        str = ActRoot::ToLower(str);
        if(str == "left")
            fSide = SilSide::ELeft;
        else if(str == "right")
            fSide = SilSide::ERight;
        else if(str == "front")
            fSide = SilSide::EFront;
        else if(str == "back")
            fSide = SilSide::EBack;
        else
            throw std::runtime_error("Side spec : " + str +
                                     " could not be interpreted in terms of left, right, front or back!");
    }
    // 5-> Build the silicon matrix with the info gathered here
    fMatrix = BuildSilMatrix();
}

template <typename T>
std::pair<ActPhysics::SilLayer::Point<T>, bool>
ActPhysics::SilLayer::GetSiliconPointOfTrack(const Point<T>& otherPoint, const Vector<T>& otherVec, bool scale) const
{
    Point<T> ref {fPoint};
    if(scale)
    {
        // Convert XY to mm
        ref.SetX(ref.X() * 2.);
        ref.SetY(ref.Y() * 2.);
    }
    auto unitVec {otherVec.Unit()};
    auto d {((ref - otherPoint).Dot(fNormal)) / (unitVec.Dot(fNormal))};
    bool isOk {(d > 0) ? true : false};
    return std::make_pair(otherPoint + unitVec * d, isOk);
}

template <typename T>
ActPhysics::SilLayer::Point<T>
ActPhysics::SilLayer::GetBoundaryPointOfTrack(int padx, int pady, const Point<T>& otherPoint,
                                              const Vector<T>& otherVec) const
{
    // Just move point to ACTAR's flanges
    Point<T> newPoint {};
    if(fSide == SilSide::EBack || fSide == SilSide::EFront)
        newPoint = {(T)padx, 0, 0};
    else
        newPoint = {0, (T)pady, 0};
    auto unitVec {otherVec.Unit()};
    auto d {((newPoint - otherPoint).Dot(fNormal)) / (unitVec.Dot(fNormal))};
    return otherPoint + unitVec * d;
}


template <typename T>
bool ActPhysics::SilLayer::MatchesRealPlacement(int i, const Point<T>& sp, bool useZ) const
{
    auto [xy, z] {fPlacements.at(i)};
    auto xyMin {xy - fUnit.GetWidth() / 2};
    auto xyMax {xy + fUnit.GetWidth() / 2};
    auto zMin {z - fUnit.GetHeight() / 2};
    auto zMax {z + fUnit.GetHeight() / 2};
    // Por XY plane we have to determine whether it is along X or Y!
    double plane {};
    if(fSide == SilSide::EBack || fSide == SilSide::EFront)
        plane = sp.Y();
    else
        plane = sp.X();
    bool condXY {xyMin <= plane && plane <= xyMax};
    bool condZ {true};
    if(useZ)
        condZ = zMin <= sp.Z() && sp.Z() <= zMax;
    return condXY && condZ;
}

std::shared_ptr<ActPhysics::SilMatrix> ActPhysics::SilLayer::BuildSilMatrix() const
{
    auto w {fUnit.GetWidth()};
    auto h {fUnit.GetHeight()};
    auto sm {std::make_shared<SilMatrix>()};
    for(const auto& [idx, pair] : fPlacements)
    {
        std::pair x {pair.first - w / 2, pair.first + w / 2};
        std::pair y {pair.second - h / 2, pair.second + h / 2};
        sm->AddSil(idx, x, y);
    }
    return sm;
}

template <typename T>
int ActPhysics::SilLayer::GetIndexOfMatch(const Point<T>& p) const
{
    int idx {-1};
    for(const auto& [i, g] : fMatrix->GetGraphs())
    {
        double xy {};
        if(fSide == SilSide::ELeft || fSide == SilSide::ERight)
            xy = p.X();
        else
            xy = p.Y();
        if(g->IsInside(xy, p.Z()))
        {
            idx = i;
            break;
        }
    }
    return idx;
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

ActPhysics::SilSpecs::SearchRet
ActPhysics::SilSpecs::FindLayerAndIdx(const XYZPoint& p, const XYZVector& v, bool verbose)
{
    SearchRet ret;
    for(const auto& [name, layer] : fLayers)
    {
        // Find silicon point for this layer
        auto [sp, _] {layer.GetSiliconPointOfTrack(p, v, true)};
        // And see if it matches with any silicon
        auto idx {layer.GetIndexOfMatch(sp)};
        if(verbose)
        {
            std::cout << "-> Layer : " << name << '\n';
            std::cout << "   SP    : " << sp << '\n';
            std::cout << "   Idx   : " << idx << '\n';
        }
        if(idx != -1)
            return {name, idx, sp};
    }
    return {"", -1, {-1, -1, -1}};
}

void ActPhysics::SilSpecs::EraseLayer(const std::string& name)
{
    auto it {fLayers.find(name)};
    if(it != fLayers.end())
        fLayers.erase(it);
}

// Explicit instantiations
template std::pair<ActPhysics::SilLayer::Point<float>, bool>
ActPhysics::SilLayer::GetSiliconPointOfTrack(const Point<float>& point, const Vector<float>& vector,
                                             bool scale = false) const;
template std::pair<ActPhysics::SilLayer::Point<double>, bool>
ActPhysics::SilLayer::GetSiliconPointOfTrack(const Point<double>& point, const Vector<double>& vector,
                                             bool scale = false) const;
///////////////////////////////////
template ActPhysics::SilLayer::Point<float>
ActPhysics::SilLayer::GetBoundaryPointOfTrack(int padx, int pady, const Point<float>& point,
                                              const Vector<float>& vector) const;
template ActPhysics::SilLayer::Point<double>
ActPhysics::SilLayer::GetBoundaryPointOfTrack(int padx, int pady, const Point<double>& point,
                                              const Vector<double>& vector) const;
////////////////////////////////////
template bool ActPhysics::SilLayer::MatchesRealPlacement(int i, const Point<float>& sp, bool useZ = true) const;
template bool ActPhysics::SilLayer::MatchesRealPlacement(int i, const Point<double>& sp, bool useZ = true) const;
////////////////////////////////////
template int ActPhysics::SilLayer::GetIndexOfMatch(const Point<float>& p) const;
template int ActPhysics::SilLayer::GetIndexOfMatch(const Point<double>& p) const;
