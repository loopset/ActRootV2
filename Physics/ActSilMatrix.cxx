#include "ActSilMatrix.h"

#include "ActColors.h"

#include "TCutG.h"
#include "TFile.h"
#include "TLatex.h"
#include "TList.h"
#include "TMultiGraph.h"
#include "TString.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

void ActPhysics::SilMatrix::AddSil(int idx, const std::pair<double, double>& x, const std::pair<double, double>& y)
{
    if(!fMatrix.count(idx))
        InitGraph(idx);
    // 1-> xlow, ylow
    AddPoint(idx, x.first, y.first);
    // 2-> xlow,map .h to hpp in nvim lsp yup
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

void ActPhysics::SilMatrix::SetSyle(bool enableLabel, Style_t ls, Width_t lw, Style_t fs)
{
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


void ActPhysics::SilMatrix::Draw(bool same, const std::string& xlabel, const std::string& ylabel) const
{
    auto mg {new TMultiGraph()};
    mg->SetTitle(TString::Format("Sil matrix %s;%s;%s", fName.c_str(), xlabel.c_str(), ylabel.c_str()));

    // Add to multigraph
    for(auto& [i, g] : fMatrix)
        mg->Add(g, "lf");

    // Draw
    if(!same)
        mg->Draw("alf plc pfc");
    else
        mg->Draw("lf plc pfc");
}

void ActPhysics::SilMatrix::Write(const std::string& file)
{
    auto fout {std::make_unique<TFile>(file.c_str(), "recreate")};
    fout->WriteObject(this, "silMatrix");
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
