#include "ActPIDCorrector.h"

#include "ActColors.h"

#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TMath.h"
#include "TProfile.h"
#include "TString.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

ActPhysics::PIDCorrection::PIDCorrection(const std::string& name, double off, double slope)
    : fName(name),
      fOffset(off),
      fSlope(slope)
{
}

void ActPhysics::PIDCorrection::Print() const
{
    std::cout << BOLDYELLOW << ":::: PIDCorrection for " << fName << '\n';
    std::cout << "-> Offset : " << fOffset << '\n';
    std::cout << "-> Slope  : " << fSlope << '\n';
    std::cout << "::::::::::::::::::::::::::::::" << RESET << '\n';
}

double ActPhysics::PIDCorrection::Apply(double q, double spz)
{
    auto eval {fOffset + fSlope * spz};
    auto diff {eval - fOffset};
    if(diff <= 0)
        return q + std::abs(diff);
    else
        return q - std::abs(diff);
}

void ActPhysics::PIDCorrection::Write(const std::string& file)
{
    auto f {std::make_unique<TFile>(file.c_str(), "recreate")};
    f->WriteObject(this, "PIDCorrection");
    std::cout << BOLDMAGENTA << "Saving PIDCorrection in " << file << '\n';
}

ActPhysics::PIDCorrector::PIDCorrector(const std::string& name, const std::vector<std::string> keys, TH2* hModel)
    : fName(name)
{
    for(const auto& key : keys)
    {
        fHistos[key] = (TH2D*)hModel->Clone(TString::Format("hPID%s_%s", key.c_str(), fName.c_str()));
        fHistos[key]->SetTitle(
            TString::Format("PID Corr for %s at %s;SP.Z() [mm];Q_{ave} [mm^{-1}]", key.c_str(), fName.c_str()));
    }
}

void ActPhysics::PIDCorrector::FillHisto(const std::string& key, double z, double q, double silE, double minE,
                                         double maxE)
{
    if(fHistos.count(key))
    {
        if(minE <= silE && silE <= maxE)
            fHistos[key]->Fill(z, q);
    }
}

void ActPhysics::PIDCorrector::GetProfiles()
{
    for(const auto& [key, h] : fHistos)
    {
        fProfs[key] = h->ProfileX(TString::Format("hProf%s_%s", key.c_str(), fName.c_str()));
        fProfs[key]->SetTitle(
            TString::Format("ProfileX for %s at %s;SP.Z() [mm];Q_{ave} [mm^{-1}]", key.c_str(), fName.c_str()));
    }
}

void ActPhysics::PIDCorrector::FitProfiles(double xmin, double xmax, const std::vector<std::string> keys)
{
    for(auto& [key, p] : fProfs)
    {
        if(keys.size() != 0 && (std::find(keys.begin(), keys.end(), key) == keys.end()))
            continue;
        p->Fit("pol1", "M0Q", "", xmin, xmax);
        // Print results
        auto* f {p->GetFunction("pol1")};
        if(!f)
        {
            throw std::runtime_error("Fitting func does not exist for " + key +
                                     ": check range of fitting, maybe ProfileX is empty there");
        }
        std::cout << BOLDGREEN << ":::: PIDCorrector fit for " << key << " at " << fName << '\n';
        std::cout << "-> p0 : " << f->GetParameter(0) << " +/- " << f->GetParError(0) << '\n';
        std::cout << "-> p1 : " << f->GetParameter(1) << " +/- " << f->GetParError(1) << '\n';
        std::cout << "::::::::::::::::::::::::::::::" << RESET << '\n';
    }
}

void ActPhysics::PIDCorrector::Draw()
{
    auto* c {
        new TCanvas(TString::Format("c%s", fName.c_str()), TString::Format("PID Corrector for %s", fName.c_str()))};
    c->DivideSquare(fHistos.size() + fProfs.size());
    int idx {0};
    for(const auto& [key, h] : fHistos)
    {
        idx++;
        c->cd(idx);
        h->Draw("col");
        if(fProfs.count(key))
        {
            idx++;
            c->cd(idx);
            fProfs[key]->Draw();
            for(auto* o : *fProfs[key]->GetListOfFunctions())
                if(o)
                    o->Draw("same");
        }
    }
}

ActPhysics::PIDCorrection ActPhysics::PIDCorrector::GetCorrection(const std::string& key)
{
    PIDCorrection corr {};
    if(key.length() > 0)
    {
        auto* f {fProfs[key]->GetFunction("pol1")};
        corr = {key, f->GetParameter(0), f->GetParameter(1)};
    }
    else
    {
        std::vector<double> offsets, slopes;
        for(const auto& [key, p] : fProfs)
        {
            auto* f {p->GetFunction("pol1")};
            if(!f)
            {
                std::cout << BOLDRED << "PIDCorrector::GetCorrection(): pol1 func does not exist in profile named "
                          << key << '\n';
                continue;
            }
            offsets.push_back(f->GetParameter(0));
            slopes.push_back(f->GetParameter(1));
        }
        auto moffset {TMath::Mean(offsets.begin(), offsets.end())};
        auto mslope {TMath::Mean(slopes.begin(), slopes.end())};
        corr = {fName, moffset, mslope};
    }
    corr.Print();
    return corr;
}
