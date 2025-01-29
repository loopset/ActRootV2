#include "ActSilMatrix.h"

#include "ActColors.h"

#include "TCutG.h"
#include "TFile.h"
#include "TLatex.h"
#include "TList.h"
#include "TMath.h"
#include "TMultiGraph.h"
#include "TString.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

ActPhysics::SilMatrix::~SilMatrix()
{
    if(fMulti)
    {
        // If MultiGraph has been generated, deleting it
        // will automatically delete the TGraphs cause it owns them
        delete fMulti;
        return;
    }
    for(auto& [i, g] : fMatrix)
        delete g;
}

void ActPhysics::SilMatrix::AddSil(int idx, const std::pair<double, double>& x, const std::pair<double, double>& y)
{
    if(!fMatrix.count(idx))
        InitGraph(idx);
    // 1-> xlow, ylow
    AddPoint(idx, x.first, y.first);
    // 2-> xlow,yup
    AddPoint(idx, x.first, y.second);
    // 3-> xup, yup
    AddPoint(idx, x.second, y.second);
    // 4-> xup, ylow
    AddPoint(idx, x.second, y.first);
    // 5-> xlow, ylow (close TGraph)
    AddPoint(idx, x.first, y.first);
}

void ActPhysics::SilMatrix::AddPoint(int idx, double x, double y)
{
    fMatrix[idx]->SetPoint(fMatrix[idx]->GetN(), x, y);
}

void ActPhysics::SilMatrix::InitGraph(int idx)
{
    fMatrix[idx] = new TCutG {};
    fMatrix[idx]->SetTitle(TString::Format("Surface matrix for sil %d", idx));
    fMatrix[idx]->GetListOfFunctions()->SetOwner(); // list owns objects
}

bool ActPhysics::SilMatrix::IsInside(int idx, double x, double y)
{
    if(!fMatrix.count(idx))
        return false;
    else
        return fMatrix[idx]->IsInside(x, y);
}

std::optional<int> ActPhysics::SilMatrix::IsInside(double x, double y)
{
    for(const auto& [i, g] : fMatrix)
        if(g->IsInside(x, y))
            return i;
    return {};
}

std::set<int> ActPhysics::SilMatrix::GetSilIndexes() const
{
    std::set<int> ret;
    for(auto& [s, _] : fMatrix)
        ret.insert(s);
    return std::move(ret);
}

void ActPhysics::SilMatrix::SetSyle(bool enableLabel, Style_t ls, Width_t lw, Style_t fs)
{
    fIsStyleSet = true; // style was manually set by user
    for(auto& [i, g] : fMatrix)
    {
        if(enableLabel)
        {
            // Set text label
            double xc {};
            double yc {};
            g->Center(xc, yc);
            auto* latex {new TLatex(xc, yc, std::to_string(i).c_str())};
            latex->SetTextAlign(22);
            g->GetListOfFunctions()->Add(latex);
        }
        // Set style
        g->SetLineStyle(ls);
        g->SetLineWidth(lw);
        g->SetFillStyle(fs);
    }
}

void ActPhysics::SilMatrix::MoveZTo(double ztarget, const std::set<int>& idxs)
{
    // Get centres of given indexes
    std::vector<double> centres;
    for(auto& idx : idxs)
    {
        if(!fMatrix.count(idx))
            throw std::runtime_error("SilMatrix::MoveZTo(): idx " + std::to_string(idx) + " not listed in fMatrix");
        auto* g {fMatrix[idx]};
        double xy {};
        double z {};
        g->Center(xy, z);
        centres.push_back(z);
    }
    // Get mean
    auto zmean {TMath::Mean(centres.begin(), centres.end())};
    // Compute difference
    auto diff {ztarget - zmean};
    // And move
    for(auto& [_, g] : fMatrix)
    {
        for(int p = 0, size = g->GetN(); p < size; p++)
        {
            g->SetPointY(p, g->GetPointY(p) + diff);
        }
        // Modify text also
        auto* text {(TLatex*)g->GetListOfFunctions()->FindObject("Text")};
        if(text)
            text->SetY(text->GetY() + diff);
    }
}

void ActPhysics::SilMatrix::MoveXYTo(double xRef, const std::pair<double, double>& yzCentre, double xTarget)
{
    // Silicon matrix is obtained at a given distance from the origin
    // This function allows scalling and moving that distance
    // by simply using triangle equivalence
    auto scale {xTarget / xRef};
    for(auto& [idx, g] : fMatrix)
    {
        for(int i = 0, size = g->GetN(); i < size; i++)
        {
            auto y {g->GetPointX(i)};
            auto z {g->GetPointY(i)};
            // But reference is not the origin but the given pair!
            auto diffy {y - yzCentre.first};
            auto diffz {z - yzCentre.second};
            // And we scale THAT DIFFERENCE
            auto scaledy {diffy * scale};
            auto scaledz {diffz * scale};
            // And set new points
            // Y = Y + (scaledDiffY - diffY);
            g->SetPointX(i, y + (scaledy - diffy));
            g->SetPointY(i, z + (scaledz - diffz));
        }
    }
}

double ActPhysics::SilMatrix::GetMeanZ(const std::set<int>& idxs)
{
    std::vector<double> zs;
    for(const auto& idx : idxs)
    {
        if(fMatrix.count(idx))
        {
            double xy {};
            double z {};
            fMatrix[idx]->Center(xy, z);
            zs.push_back(z);
        }
    }
    return std::accumulate(zs.begin(), zs.end(), 0.) / zs.size();
}

TMultiGraph* ActPhysics::SilMatrix::Draw(bool same, const std::string& xlabel, const std::string& ylabel)
{
    if(fMulti)
    {
        delete fMulti;
        fMulti = nullptr;
    }
    fMulti = new TMultiGraph;
    fMulti->SetTitle(TString::Format("Sil matrix %s;%s;%s", fName.c_str(), xlabel.c_str(), ylabel.c_str()));

    // If style was not set by user, set default
    if(!fIsStyleSet)
        SetSyle();

    // Add to multigraph
    for(auto& [i, g] : fMatrix)
        fMulti->Add(g, "lf");

    // Draw
    if(!same)
        fMulti->Draw("alf plc pfc");
    else
        fMulti->Draw("lf plc pfc");
    return fMulti;
}

void ActPhysics::SilMatrix::Write(const std::string& file)
{
    auto fout {std::make_unique<TFile>(file.c_str(), "recreate")};
    fout->WriteObject(this, "silMatrix");
}

void ActPhysics::SilMatrix::Write()
{
    gFile->WriteObject(this, fName.c_str());
}

void ActPhysics::SilMatrix::Read(const std::string& file)
{
    auto fin {std::make_unique<TFile>(file.c_str())};
    auto* copy {fin->Get<SilMatrix>("silMatrix")};
    if(!copy)
        throw std::runtime_error("SilMatrix::Read(): could not find silMatrix in file " + file);
    *this = *copy;
}

void ActPhysics::SilMatrix::Print() const
{
    std::cout << BOLDGREEN << ":::: SilMatrix ::::" << '\n';
    std::cout << "-> Name : " << fName << '\n';
    std::cout << std::fixed << std::setprecision(2);
    for(const auto& [i, g] : fMatrix)
    {
        auto width {g->GetPointX(3) - g->GetPointX(0)};
        auto height {g->GetPointY(1) - g->GetPointY(0)};
        std::cout << "-> Idx : " << i << '\n';
        std::cout << "···· Width  : " << width << '\n';
        std::cout << "···· Height : " << height << '\n';
    }
    std::cout << "::::::::::::::::::::::::::::::" << RESET << '\n';
}

void ActPhysics::SilMatrix::Erase(int idx)
{
    auto it {fMatrix.find(idx)};
    if(it != fMatrix.end())
    {
        delete fMatrix[idx];
        fMatrix.erase(it);
    }
}

ActPhysics::SilMatrix* ActPhysics::SilMatrix::Clone() const
{
    auto* sm {new SilMatrix};
    sm->SetName(fName);
    sm->SetPadIdx(fPadIdx);
    for(const auto& [idx, g] : fMatrix)
    {
        auto* cl {(TCutG*)g->Clone()};
        cl->GetListOfFunctions()->Clear();
        sm->AddSil(idx, cl);
    }
    return sm;
}

TMultiGraph* ActPhysics::SilMatrix::DrawClone(bool same)
{
    auto* sm {this->Clone()};
    return sm->Draw(same);
}

std::pair<double, double> ActPhysics::SilMatrix::GetCentre(int idx) const
{
    std::pair<double, double> ret {-1, -1};
    if(fMatrix.count(idx))
    {
        auto& g {fMatrix.at(idx)};
        double xy {};
        double z {};
        g->Center(xy, z);
        ret = {xy, z};
    }
    return ret;
}

double ActPhysics::SilMatrix::GetWidth(int idx) const
{
    double ret {-1};
    if(fMatrix.count(idx))
    {
        auto& g {fMatrix.at(idx)};
        ret = std::abs(g->GetPointX(0) - g->GetPointX(3));
    }
    return ret;
}

double ActPhysics::SilMatrix::GetHeight(int idx) const
{
    double ret {-1};
    if(fMatrix.count(idx))
    {
        auto& g {fMatrix.at(idx)};
        ret = std::abs(g->GetPointY(0) - g->GetPointY(1));
    }
    return ret;
}

void ActPhysics::SilMatrix::CreateMultiGraphForPainter()
{
    if(fMulti)
    {
        delete fMulti;
        fMulti = nullptr;
    }
    fMulti = new TMultiGraph;
    fMulti->SetTitle(TString::Format("%s;X or Y [mm];Z [mm]", fName.c_str()));
    // Set style for painter: no index
    SetSyle();
    // Add to multigraph
    for(auto& [i, g] : fMatrix)
        fMulti->Add(g, "lf");
}

void ActPhysics::SilMatrix::DrawForPainter()
{
    fMulti->Draw("al plc pfc");
}
