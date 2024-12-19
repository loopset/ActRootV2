#include "ActCrossSection.h"


#include "ActUtils.h"

#include "TAxis.h"
#include "TMath.h"

#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

void ActSim::CrossSection::ReadData(const std::string& file, const bool isAngle)
{
    fIsAngle = isAngle;
    std::ifstream streamer {file};
    if(!streamer)
        throw std::runtime_error("CrossSection::ReadData(): could not open input file named " + file);


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
                if(fIsAngle && std::stod(col) == 180)
                    fX.push_back(std::stod(col)-0.001); // Avoid the point 180 deg (sin180 = 0), problems in CDF calculation
                else
                    fX.push_back(std::stod(col));
            }
            else if(colIndex == 1)
            {
                fY.push_back(std::stod(col));
            }
            colIndex++;
        }
    }
    // Define the step in X
    fStep = fX[1] - fX[0];
    // Get the total XS
    // To get counts in a theta bin, one has to multiply xs by sin(theta)
    // Get also the graph of theo xs
    fTheoXSGraph = new TGraph();
    if(fIsAngle)
    {
        for(int i = 0; i < fY.size(); i++)
        {
            fTheoXSGraph->AddPoint(fX[i], fY[i]);
            fY[i] = fY[i] * TMath::Sin(fX[i] * TMath::DegToRad());
        }
        fTotalXS = std::accumulate(fY.begin(), fY.end(), 0.0) * fStep * TMath::TwoPi();
    }
    else // For xs in energy is just the sum
    {
        for(int i = 0; i < fY.size(); i++)
        {
            fTheoXSGraph->AddPoint(fX[i], fY[i]);
        }
        fTotalXS = std::accumulate(fY.begin(), fY.end(), 0.0);
    }
    // Compute the CDF
    double sumXS {};
    for(const auto& xs : fY)
        sumXS += xs;
    // Now the CDF
    std::vector<double> CDFData {};
    for(int i = 0; i < fY.size(); i++)
    {
        double termCDF {};
        for(int j = 0; j <= i; j++)
        {
            termCDF += fY[j];
        }
        termCDF /= sumXS;
        CDFData.push_back(termCDF);
    }
    // Get the Spline
    fCDF = new TSpline3 {"fCDF", &CDFData[0], &fX[0], (int)CDFData.size(), "b2,e2", 0, 0};
    fCDF->SetTitle("CDF;r;#theta_{CM} [#circ]");
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
    fCDF->Draw();
}

double ActSim::CrossSection::Sample(const double randomValue)
{
    return fCDF->Eval(randomValue);
}

void ActSim::CrossSection::DrawTheo()
{
    auto* c1 {new TCanvas("cTheo")};

    fTheoXSGraph->SetTitle("TheoXS");
    // Format the graph depending the type
    if(fIsAngle)
    {
        fTheoXSGraph->GetXaxis()->SetTitle("Angle [deg]");
        fTheoXSGraph->GetYaxis()->SetTitle("d#sigma / d#Omega [mb sr^{-1}]");
    }
    else
    {
        fTheoXSGraph->GetXaxis()->SetTitle("E [MeV]");
        fTheoXSGraph->GetYaxis()->SetTitle("d#sigma / dE [mb MeV^{-1}]");
    }
    fTheoXSGraph->Draw();
}
