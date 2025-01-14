#include "ActCrossSection.h"

#include "ActUtils.h"

#include "TCanvas.h"
#include "TF1.h"
#include "TGraph.h"
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

double ActSim::CrossSection::xsIntervalcm(const TString& file, double minAngle, double maxAngle)
{
    double xsIntervalValue {};
    for(int i = 0; i < fX.size(); i++)
    {
        // Only process the data if the angle is within the specified range
        if(minAngle <= fX[i] && fX[i] <= maxAngle)
        {
            xsIntervalValue += fY[i] * TMath::TwoPi() * (fStep * TMath::DegToRad());
        }
    }
    return xsIntervalValue; // Return in mb
}

void ActSim::CrossSection::DrawCDF() const
{
    auto c0 {new TCanvas("cCDF", "CDF Canvas")};
    fCDF->SetLineWidth(2);
    fCDF->SetLineColor(kRed);
    fCDF->SetLineWidth(3);
    fCDF->SetNpx(1000);
    fCDF->Draw();
}

double ActSim::CrossSection::Sample(double r)
{
    return fCDF->Eval(r);
}

double ActSim::CrossSection::Sample()
{
    return Sample(gRandom->Uniform());
}

void ActSim::CrossSection::DrawTheo()
{
    auto* c1 {new TCanvas("cTheo")};
    fTheoXSGraph->Draw();
}
