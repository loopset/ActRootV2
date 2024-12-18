#include "ActCrossSection.h"

#include "TAxis.h"
#include "TMath.h"

#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>


std::string ActSim::CrossSection::StripSpaces(std::string line)
{
    while(*line.begin() == ' ')
        line = line.substr(1, line.length());
    return line;
}

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
            col = StripSpaces(col);
            // If empty, skip
            if(col.length() == 0)
                continue;
            // Save col as double
            if(colIndex == 0)
            {
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
    if(fIsAngle)
    {
        std::vector<double> fY_AngleGraph(fY.size());
        for(int i = 0; i < fY.size(); i++)
        {
            fY_AngleGraph[i] = fY[i];
            fY[i] = fY[i] * TMath::Sin(fX[i] * TMath::DegToRad());
        }
        fTotalXS = std::accumulate(fY.begin(), fY.end(), 0.0) * fStep * TMath::TwoPi();
    }
    else // For xs in energy is just the sum
    {
        fTotalXS = std::accumulate(fY.begin(), fY.end(), 0.0);
    }
    // Compute the CDF
    double sumXS {};
    for(const auto& xs : fY)
    {
        sumXS += xs;
    }
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
    std::cout << "Evaluando spline en puntos clave:" << std::endl;
    std::cout << "Eval(0.5) = " << fCDF->Eval(0.5) << std::endl;
    std::cout << "Eval(1.0) = " << fCDF->Eval(1.0) << std::endl;
    std::cout << "Eval(0.0) = " << fCDF->Eval(0.0) << std::endl;

    TCanvas* canvas = new TCanvas("canvas", "Datos y Spline", 800, 600);
    TGraph* graph = new TGraph(CDFData.size(), &CDFData[0], &fX[0]);
    graph->SetTitle("Datos y Spline;X;Y");
    graph->SetMarkerStyle(20);  // Estilo de marcador (círculos)
    graph->SetMarkerSize(1.0);  // Tamaño de los marcadores
    graph->SetMarkerColor(kBlue);  // Color de los marcadores
    graph->Draw();

    for (size_t i = 0; i < CDFData.size(); ++i) {
    std::cout << "CDFData[" << i << "] = " << CDFData[i] << ", fX[" << i << "] = " << fX[i] << std::endl;
}
}

double ActSim::CrossSection::xsIntervalcm(const TString& file, double minAngle, double maxAngle)
{
    double xsIntervalValue {};
    for(int i = 0; i < fX.size(); i++)
    {
        // Only process the data if the angle is within the specified range
        if(fX[i] >= minAngle && fX[i] <= maxAngle)
        {
            xsIntervalValue += fY[i] * TMath::TwoPi() * (fStep * TMath::DegToRad());
        }
    }
    return xsIntervalValue * 1e-27;
}

void ActSim::CrossSection::DrawCDF() const
{
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
    auto* c1 = new TCanvas;

    // Create a TGraph to draw the axes
    if(fIsAngle)
    {
        TGraph* graph = new TGraph(static_cast<int>(fX.size()), &(fX[0]), &(fY[0]));
        graph->SetTitle("TheoXS"); // Title for the graph
        graph->Draw();         // Draw the graph with points and axes

        // Configure the axes of the TGraph
        graph->GetXaxis()->SetTitle("Angle [deg]");                    // Title of the X-axis
        graph->GetYaxis()->SetTitle("d#sigma / d#Omega [mb sr^{-1}]"); // Title of the Y-axis
    }
    else
    {
        TGraph* graph = new TGraph(static_cast<int>(fX.size()), &(fX[0]), &(fY[0]));
        graph->SetTitle("TheoXS"); // Title for the graph
        graph->Draw();         // Draw the graph with points and axes

        // Configure the axes of the TGraph
        graph->GetXaxis()->SetTitle("Angle [deg]");                    // Title of the X-axis
        graph->GetYaxis()->SetTitle("d#sigma / dE [mb MeV^{-1}]"); // Title of the Y-axis
    }
    
    

    // Create and draw the TSpline3
    if(fIsAngle)
    {
        fTheoXS = new TSpline3("theoXS", &(fX[0]), &(fY_AngleGraph[0]), static_cast<int>(fX.size()), "b2,e2", 0, 0);
    }
    else
    {
        fTheoXS = new TSpline3("theoXS", &(fX[0]), &(fY[0]), static_cast<int>(fX.size()), "b2,e2", 0, 0);
    }
    
    fTheoXS->SetLineColor(kRed);
    fTheoXS->SetLineWidth(5);
    fTheoXS->Draw("same"); // Draw the spline on the same canvas

    c1->Modified(); // Mark the canvas as modified
    c1->Update();   // Refresh the canvas to reflect changes
}
