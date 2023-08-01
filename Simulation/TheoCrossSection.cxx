#include "TheoCrossSection.h"

#include "Rtypes.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TF1.h"
#include "TMath.h"
#include "TRandom.h"
#include "TRandom3.h"
#include "TSpline.h"
#include "TString.h"
#include "TAxis.h"
#include "TLegend.h"
#include "Math/IFunction.h"
#include "Math/SpecFuncMathMore.h"
#include "TVirtualPad.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

void ActSim::TheoCrossSection::ReadScatteringCrossSection(std::string fileName)
{
    std::ifstream streamer {};
    streamer.open(fileName.c_str());
    if(!streamer)
        throw std::runtime_error(("Couldn't open " + fileName).c_str());
    //ignore lines
    std::string line {};
    for(int i = 0; i < nIgnoredLinesScattering; i++)
    {
        std::getline(streamer, line);
    }
    bool stopAtEmptyNumber { false};//we reach end of data points
        while(std::getline(streamer, line))
    {
        //to check if it is energy or xs
        int oddOrEven {0};
        //loop through numbers
        for(int i = 0; i < ENDFLineWidth; i += ENDFNumberWidth)
        {
            std::string number {line.substr(i, ENDFNumberWidth)};
            //check if it's empty
            if(number.find_first_not_of(' ') == std::string::npos)
            {
                stopAtEmptyNumber = true;
                break;
            }
            //otherwise, get double
            double coeff {};
            int expo  {};
            std::istringstream numberStreamer {number};
            numberStreamer >> coeff >> expo;
            double finalNumber { coeff * std::pow(10, expo)};
            if(oddOrEven % 2 == 0)
                vXScattering.push_back(finalNumber * converteVToMeV);
            else
                vYScattering.push_back(finalNumber);

            oddOrEven++;
        }
        if(stopAtEmptyNumber)
            break;
    }
    streamer.close();

    //initialize smart pointers
    splineScattering = std::make_unique<TSpline3>("splineScattering",
                                             &(vXScattering[0]),
                                             &(vYScattering[0]),
                                             vXScattering.size(),
                                             "b2,e2",
                                             0., 0.);
    functScattering = std::make_unique<TF1>("functScattering",
                                      [this](double* x, double* p){return splineScattering->Eval(x[0]);},
                                      vXScattering.front(),
                                      vXScattering.back(),
                                      1);
    std::cout<<"SimCrossSection: successfully read Scattering XS!"<<'\n';
}

void ActSim::TheoCrossSection::ParseLine(std::string& line, std::vector<double>& coeffs)
{
    for(int i = 0; i < ENDFLineWidth; i += ENDFNumberWidth)
    {
        std::string number {line.substr(i, ENDFNumberWidth)};
        //check if it's empty
        if(number.find_first_not_of(' ') == std::string::npos)
        {
            break;
        }
        //otherwise, get double
        double coeff {};
        int expo  {};
        std::istringstream numberStreamer {number};
        numberStreamer >> coeff >> expo;
        double finalNumber { coeff * std::pow(10, expo)};
        coeffs.push_back(finalNumber);
    }
}

void ActSim::TheoCrossSection::ReadAngularCrossSection(std::string fileName)
{
    std::ifstream streamer {};
    streamer.open(fileName.c_str());
    if(!streamer)
        throw std::runtime_error(("Couldn't open " + fileName).c_str());
    //ignore heading lines
    std::string line {};
    for(int i = 0; i < nIgnoredLinesAngular; i++)
    {
        std::getline(streamer, line);
    }
    //we should start always by an energy line!
    while(std::getline(streamer, line))
    {
        // ENERGY
        double coeff {}; int expo {};
        std::string energyString {line.substr(1 * ENDFNumberWidth, ENDFNumberWidth)};
        std::istringstream(energyString) >> coeff >> expo;
        double energy { coeff * std::pow(10, expo)};
        //NUMBER OF LEGENDRE COEFFS
        std::string nLegendreString { line.substr(4 * ENDFNumberWidth, ENDFNumberWidth)};
        int nLegendre {std::stoi(nLegendreString)};

        //SWITCH ACCORDING TO NUM OF NLEGENDRE
        int linesToParse {};
        bool stop {false};
        switch (nLegendre)
        {
        case 1 ... 6:
            //just parse one line
            linesToParse = 1;
            break;
        case 7 ... 12:
            linesToParse = 2;
            break;
        case 13 ... 18:
            linesToParse = 3;
            break;
        default:
            stop = true;
            break;
        }
        //reached end of data points
        if(stop)
            break;
        //parse lines
        std::vector<double> coeffs {};
        for(int i = 0; i < linesToParse; i++)
        {
            std::getline(streamer, line);
            ParseLine(line, coeffs);
        }
        //and write coeffs to map
        for(int i = 1; i <= nLegendre; i++)
        {
            mAngular[i].first.push_back(energy * converteVToMeV);
            mAngular[i].second.push_back(coeffs[i - 1]);
        }
    }
    streamer.close();

    //Print size for crosscheck
    //std::cout<<"Size of Legendre coefficients map is: "<<mAngular.size()<<'\n';

    //initialize splines and functions
    for(auto& leg : mAngular)
    {
        int label { leg.first};
        splinesAngular[label] = std::make_unique<TSpline3>(("spline_a" + std::to_string(label)).c_str(),
                                                               &(mAngular[label].first[0]),
                                                               &(mAngular[label].second[0]),
                                                               mAngular[label].first.size(),
                                                               "b2,e2",
                                                               0., 0.);
        funcsAngular[label] = std::make_unique<TF1>(("func_a" + std::to_string(label)).c_str(),
                                                    [label, this](double* x, double* p){return splinesAngular[label]->Eval(x[0]);},
                                                    mAngular[label].first.front(),
                                                    mAngular[label].first.back(),
                                                    1);
    }
    std::cout<<"SimCrossSection: successfully read Angular XS!"<<'\n';
}

void ActSim::TheoCrossSection::DrawScattering()
{
    canvScattering = std::make_unique<TCanvas>("canvScattering", "Total cross secion");
    canvScattering->cd();
    canvScattering->SetLogx();
    canvScattering->SetLogy();
    functScattering->SetTitle("Scattering cross section;T_{n} [MeV];#sigma [b]");
    functScattering->SetLineColor(kBlue);
    functScattering->SetLineWidth(3);
    functScattering->Draw();
    canvScattering->Update();
    //canvScattering->WaitPrimitive("lat", "");
    //canvScattering->Close();
}

void ActSim::TheoCrossSection::DrawAngular()
{
    canvAngular = std::make_unique<TCanvas>("canvAngular", "Legendre coefficients for angular xs");
    legendAngular      = std::make_unique<TLegend>(0.2, 0.3);
    legendAngular->SetFillStyle(0);
    legendAngular->SetNColumns(2);
    canvAngular->cd();
    canvAngular->SetLogx();
    //canvAngular->SetLogy();
    int counter {1};
    double min {};
    double max {};
    for(auto& leg : funcsAngular)
    {
        leg.second->SetLineWidth(3);
        leg.second->SetLineColor((counter == 10) ? counter + 20 : counter);
        double auxMin { leg.second->GetMinimum()};
        min = (std::abs(auxMin) > std::abs(min)) ? auxMin : min;
        double auxMax { leg.second->GetMaximum()};
        max = (auxMax > max) ? auxMax : max;
        if(counter == 1)
        {
            leg.second->SetTitle("Legendre coeffs for angular XS;T_{n} [MeV];Legendre coeffs a_{n}");
            leg.second->Draw("");
        }
        else
            leg.second->Draw("same");

        legendAngular->AddEntry(leg.second.get(), TString::Format("a_{%d}", counter).Data(), "l");

        //set max and min values
        funcsAngular[1]->GetYaxis()->SetRangeUser(min, max);
        counter++;
    }
    legendAngular->Draw();
    canvAngular->Update();
    //canvAngular->WaitPrimitive("lat", "");
    //legend.reset();
    //canvAngular->Close();
}

void ActSim::TheoCrossSection::ComputeXSAtEnergy(double Tn)
{
    scatteringXS = functScattering->Eval(Tn);
    legendreCoeffs.clear();
    legendreCoeffs.push_back(1.0); //a_0 = 1.0 always according to documentation
    for(auto& leg : mAngular)
    {
        //ensure that this coeff contributes at this energy
        double minT { leg.second.first.front()};
        double maxT { leg.second.first.back()};
        //std::cout<<"minT: "<<minT<<" maxT: "<<maxT<<'\n';
        bool isInInterval { (Tn >= minT) && (Tn <= maxT)};
        legendreCoeffs.push_back((isInInterval) ? funcsAngular[leg.first]->Eval(Tn) : 0.0);
        //std::cout<<"Legendre coeff "<<leg.first<<" : "<<legendreCoeffs.back()<<'\n';
    }
    //lambda
    lambda = [this](double* x, double* p)
    {
        double val {};
        double commonFactor { scatteringXS / (2.0 * TMath::Pi())};
        for(int l = 0; l < legendreCoeffs.size(); l++)
        {
            val += (2.0 * l + 1) / 2 * legendreCoeffs[l] * ROOT::Math::legendre(l, x[0]);
        }
        return commonFactor * val;
    };
    xsAtEnergy.reset();
    xsAtEnergy = std::make_unique<TF1>("xsAtEnergy", lambda, -1.0, 1.0, 1);
    xsAtEnergy->SetTitle(TString::Format("Diff. XS at T_{n} = %.2f MeV;cos #theta_{n};#sigma [b / sr]", Tn));
}

double ActSim::TheoCrossSection::EvalScatteringXS(double Tn)
{
    return functScattering->Eval(Tn);
}

double ActSim::TheoCrossSection::IntegrateTotalXS(double low, double high)
{
    return xsAtEnergy->Integral(low, high);
}

double ActSim::TheoCrossSection::SampleXS(TRandom3* generator)
{
    double cosValue { xsAtEnergy->GetRandom((generator == nullptr) ? gRandom : generator)};
    return TMath::ACos(cosValue);
}

void ActSim::TheoCrossSection::DrawXS()
{
    if(!canvFinalXS)
        canvFinalXS = std::make_unique<TCanvas>("canvFinalXS", "Xs at energy");
    else
        canvFinalXS->Clear();
    canvFinalXS->cd();
    xsAtEnergy->SetLineWidth(3);
    xsAtEnergy->Draw();
    canvFinalXS->cd();
    canvFinalXS->Update();
    canvFinalXS->WaitPrimitive();
    //canvFinalXS->Close();
}

void ActSim::TheoCrossSection::DrawFiles()
{
    DrawScattering();
    DrawAngular();
    gPad->cd();
    gPad->WaitPrimitive("lat", "");
    
}
