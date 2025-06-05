#include "ActCrossSection.h"

#include "ActUtils.h"

#include "Rtypes.h"

#include "TCanvas.h"
#include "TF1.h"
#include "TGraph.h"
#include "TH1.h"
#include "TMath.h"
#include "TRandom.h"
#include "TString.h"

#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

void ActSim::CrossSection::Init(int n, const double* x, const double* y)
{
    // Init vectors
    fX.assign(x, x + n);
    fY.assign(y, y + n);

    // Define a graph to visualize the data
    fTheoXSGraph = new TGraph {static_cast<int>(fX.size()), fX.data(), fY.data()};
    fTheoXSGraph->SetTitle(TString::Format("Theoretical XS;%s;d#sigma / d#Omega [mb/%s]",
                                           fIsAngle ? "#theta_{CM} [#circ]" : "E_{x} [MeV]", fIsAngle ? "sr" : "MeV"));

    // Define the step in X
    fStep = fX[1] - fX[0];

    // Multiply by solid angle factors if necessary
    if(fIsAngle)
    {
        for(int i = 0; i < fX.size(); i++)
            fY[i] = fY[i] * TMath::Sin(fX[i] * TMath::DegToRad());
        fTotalXS = std::accumulate(fY.begin(), fY.end(), 0.0) * fStep * TMath::TwoPi();
    }
    else // For xs in energy is just the sum
        fTotalXS = std::accumulate(fY.begin(), fY.end(), 0.0);

    // Compute the CDF
    double sumXS {};
    for(const auto& xs : fY)
        sumXS += xs;
    // Now the CDF
    std::vector<double> CDFData {};
    for(int i = 0; i < fX.size(); i++)
    {
        double termCDF {};
        for(int j = 0; j <= i; j++)
            termCDF += fY[j];
        termCDF /= sumXS;
        CDFData.push_back(termCDF);
    }

    // Get the Spline
    fCDF = new TSpline3 {"fCDF", &CDFData[0], &fX[0], (int)CDFData.size(), "b2,e2", 0, 0};
    fCDF->SetTitle(TString::Format("CDF;r;%s", fIsAngle ? "#theta_{CM} [#circ]" : "E_{x} [MeV]"));

    // Compute the histogram
    TGraph ghist {static_cast<Int_t>(fX.size()), fX.data(), fY.data()};
    TF1 fhist {"fhist", [&](double* x, double* p) { return ghist.Eval(x[0], nullptr, "S"); }, fX.front(), fX.back(), 0};
    // Get bin info
    auto min {fX.front() - fStep / 2};
    auto max {fX.back() + fStep / 2};
    auto nbins {static_cast<int>((max - min) / 0.5)};
    fHist = new TH1D {"hXS", "Count histo;#theta_{CM} [#circ];#counts", nbins, min, max};
    fHist->Add(&fhist);
}

void ActSim::CrossSection::ReadFile(const std::string& file)
{
    std::ifstream streamer {file};
    if(!streamer)
        throw std::runtime_error("CrossSection::ReadFile(): could not open input file named " + file);

    std::vector<double> x, y;
    // Outter loop: lines
    std::string line {};
    while(std::getline(streamer, line))
    {
        // Inner loop: columns
        std::istringstream lineStreamer {line};
        std::string col {};
        int colIndex {};
        while(std::getline(lineStreamer, col, ' '))
        {
            // Clean whitespaces
            col = ActRoot::StripSpaces(col);
            // If empty, skip
            if(col.length() == 0)
                continue;
            // Save col as double
            if(colIndex == 0)
            {
                auto val {std::stod(col)};
                if(fIsAngle && val == 180)
                    x.push_back(val - 0.001); // Avoid the point 180 deg (sin180 = 0), problems in CDF calculation
                else
                    x.push_back(std::stod(col));
            }
            else if(colIndex == 1)
                y.push_back(std::stod(col));
            colIndex++;
        }
    }

    // Call init method!
    Init(x.size(), x.data(), y.data());
}

void ActSim::CrossSection::ReadGraph(TGraph* g)
{
    // Delete 180 deg point to avoid error with Spline
    int lastPoint {g->GetN() - 1};
    if(g->GetPointX(lastPoint) == 180)
        g->RemovePoint(lastPoint);
    Init(g->GetN(), g->GetX(), g->GetY());
}

double ActSim::CrossSection::GetIntervalXS(double minAngle, double maxAngle)
{
    double xsIntervalValue {};
    for(int i = 0; i < fX.size(); i++)
    {
        if(minAngle <= fX[i] && fX[i] <= maxAngle)
        {
            xsIntervalValue += fY[i] * TMath::TwoPi() * (fStep * TMath::DegToRad()); // already multiplied by sin factor
        }
    }
    return xsIntervalValue; // Return in mb
}

void ActSim::CrossSection::Draw() const
{
    auto* c {new TCanvas {"cXS", "Cross section canvas"}};
    c->DivideSquare(4);
    c->cd(1);
    fTheoXSGraph->Draw("apl");
    c->cd(2);
    fCDF->SetLineWidth(2);
    fCDF->SetLineColor(kRed);
    fCDF->SetLineWidth(3);
    fCDF->SetNpx(1000);
    fCDF->Draw();
    c->cd(3);
    fHist->Draw();
}

double ActSim::CrossSection::SampleCDF(double r)
{
    return fCDF->Eval(r);
}

double ActSim::CrossSection::SampleCDF()
{
    return SampleCDF(gRandom->Uniform());
}

double ActSim::CrossSection::SampleHist(TRandom* rand)
{
    return fHist->GetRandom(rand);
}
