#include "ActSilSpecs.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActSilMatrix.h"
#include "ActUtils.h"
#include "Rtypes.h"

#include "TCanvas.h"
#include "TH3.h"
#include "TPolyLine3D.h"
#include "TVirtualPad.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
    std::cout << "-> Margin : " << fMargin << " mm" << '\n';
    std::cout << "-> PadIdx : " << fPadIdx << '\n';
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
    // 5-> Read pad index
    if(block->CheckTokenExists("PadIdx", true))
        fPadIdx = block->GetInt("PadIdx");
    // 6-> Margin to match
    if(block->CheckTokenExists("MatchMargin", true))
        fMargin = block->GetDouble("MatchMargin");

    // 7-> Build the silicon matrix with the info gathered here
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
    auto xyMin {xy - fUnit.GetWidth() / 2 - fMargin};
    auto xyMax {xy + fUnit.GetWidth() / 2 + fMargin};
    auto zMin {z - fUnit.GetHeight() / 2 - fMargin};
    auto zMax {z + fUnit.GetHeight() / 2 + fMargin};
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
    sm->SetPadIdx(fPadIdx);
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

void ActPhysics::SilLayer::UpdatePlacementsFromMatrix()
{
    fPlacements.clear();
    for(const auto& [idx, _] : fMatrix->GetGraphs())
    {
        auto [xy, z] {fMatrix->GetCentre(idx)};
        fPlacements[idx] = {xy, z};
    }
}

void ActPhysics::SilLayer::ReplaceWithMatrix(ActPhysics::SilMatrix* sm)
{
    fMatrix.reset();
    fMatrix = std::shared_ptr<SilMatrix>(sm->Clone());
    UpdatePlacementsFromMatrix();
}

void ActPhysics::SilLayer::MoveZTo(double z, const std::set<int>& idxs)
{
    fMatrix->MoveZTo(z, idxs);
    UpdatePlacementsFromMatrix();
}

double ActPhysics::SilLayer::MeanZ(const std::set<int>& idxs)
{
    return fMatrix->GetMeanZ(idxs);
}

void ActPhysics::SilSpecs::ReadFile(const std::string& file)
{
    ActRoot::InputParser parser {file};
    for(const auto& name : parser.GetBlockHeaders())
    {
        SilLayer layer;
        layer.ReadConfiguration(parser.GetBlock(name));
        fLayers[name] = layer;
        fLayers[name].GetSilMatrix()->SetName(name);
    }
}

ActPhysics::SilSpecs::SearchTuple
ActPhysics::SilSpecs::FindLayerAndIdx(const XYZPoint& p, const XYZVector& v, bool verbose)
{
    SearchTuple ret;
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

ActPhysics::SilSpecs::SearchPair
ActPhysics::SilSpecs::FindSPInLayer(const std::string& name, const XYZPoint& p, const XYZVector& v)
{
    if(fLayers.count(name))
    {
        auto [sp, _] {fLayers[name].GetSiliconPointOfTrack(p, v, true)};
        auto idx {fLayers[name].GetIndexOfMatch(sp)};
        return {idx, sp};
    }
    else
        throw std::invalid_argument("SilSpecs::FindSPInLayer: received wrong layer name");
}

void ActPhysics::SilSpecs::EraseLayer(const std::string& name)
{
    auto it {fLayers.find(name)};
    if(it != fLayers.end())
        fLayers.erase(it);
}

void ActPhysics::SilSpecs::ReplaceWithMatrix(const std::string& name, SilMatrix* sm)
{
    if(fLayers.count(name))
        fLayers[name].ReplaceWithMatrix(sm);
}

TVirtualPad* ActPhysics::SilSpecs::DrawGeo(double zoffset, bool withActar)
{
    // Colors
    std::vector<int> colors {kBlue + 2,  kOrange + 1, kCyan + 2, kMagenta - 3,
                             kGreen + 3, kYellow + 2, kBlack,    kGray + 1};
    std::unordered_map<std::string, std::vector<TPolyLine3D*>> polys;
    int nl {};
    for(const auto& [name, layer] : fLayers)
    {
        const auto& unit {layer.GetUnit()};
        auto point {layer.GetPoint()};
        // Convert to mm
        point *= 2;
        const auto& side {layer.GetSilSide()};
        for(const auto& [idx, g] : layer.GetSilMatrix()->GetGraphs())
        {
            // Instantiate polyline
            auto* poly {new TPolyLine3D};
            for(int i = 0; i < g->GetN(); i++)
            {
                auto a {g->GetPointX(i)};
                auto b {g->GetPointY(i)};
                if(side == SilSide::EBack || side == SilSide::EFront)
                    poly->SetNextPoint(point.X(), a, b + zoffset);
                else
                    poly->SetNextPoint(a, point.Y(), b + zoffset);
            }
            // Add Thickness
            for(int i = 0; i < g->GetN(); i++)
            {
                auto a {g->GetPointX(i)};
                auto b {g->GetPointY(i)};
                if(side == SilSide::EBack || side == SilSide::EFront)
                    poly->SetNextPoint(point.X() + unit.GetThickness(), a, b + zoffset);
                else
                    poly->SetNextPoint(a, point.Y() + unit.GetThickness(), b + zoffset);
            }
            // Set style options
            poly->SetLineWidth(2);
            if(nl < colors.size())
                poly->SetLineColor(colors[nl]);
            polys[name].push_back(poly);
        }
        nl++;
    }

    // Create a histo3d to serve as view
    auto* h3d {new TH3I {"h3D", "Geometry view;X [mm];Y [mm];Z [mm]", 50, 0, 450, 50, 0, 450, 50, 0, 300}};
    h3d->SetStats(false);

    // Canvas
    auto* c {new TCanvas {"cSilSpecs", "Geomtry drawing"}};
    h3d->Draw();
    // And draw polylines
    for(const auto& [name, vec] : polys)
        for(const auto& poly : vec)
            poly->Draw("same");
    // If ACTAR (prototype not available) is requested
    if(withActar)
    {
        std::vector<std::vector<double>> points {{0, 0, 0},   {256, 0, 0},   {256, 256, 0},   {0, 256, 0},
                                                 {0, 0, 256}, {256, 0, 256}, {256, 256, 256}, {0, 256, 256}};
        std::vector<std::pair<int, int>> edges {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                                {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for(auto& [a, b] : edges)
        {
            double x[2] = {points[a][0], points[b][0]};
            double y[2] = {points[a][1], points[b][1]};
            double z[2] = {points[a][2], points[b][2]};
            auto* edge = new TPolyLine3D(2, x, y, z);
            edge->SetLineColor(kAzure + 10);
            edge->SetLineWidth(2);
            edge->Draw();
        }
    }
    return c;
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
