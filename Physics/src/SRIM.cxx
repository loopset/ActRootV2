#include "SRIM.h"

#include "TCanvas.h"
#include "TF1.h"
#include "TMath.h"
#include "TMathBase.h"
#include "TSpline.h"
#include "TLegend.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>

void ActPhysics::SRIM::ReadInterpolations(std::string key, std::string fileName)
{
    std::ifstream streamer(fileName.c_str());
	double auxE, auxSe, auxSn, auxR, auxLongStrag, auxLatStrag;
    std::vector<double> vE, vStop, vR, vLongStrag, vLatStrag;
	while(streamer >> auxE >> auxSe >> auxSn >> auxR >> auxLongStrag >> auxLatStrag)
	{
        vE.push_back(auxE);
        vStop.push_back(auxSe + auxSn);
        vR.push_back(auxR);
        vLongStrag.push_back(auxLongStrag);
        vLatStrag.push_back(auxLatStrag);
	}
	streamer.close();

    ///////////////////////// RANGE <---> ENERGY
	fSplinesDirect[key] = std::make_unique<TSpline3>(("spdirect" + key).c_str(),
													 &(vE[0]),//energy
													 &(vR[0]),//range
													 vE.size(),
													 "b2, e2",
													 0., 0.);
	fInterpolationsDirect[key] = std::make_unique<TF1>(("func_direct_" + key).c_str(),
													   [key, this](double* x, double* p)
													   {return fSplinesDirect[key]->Eval(x[0]);},
													   vE.front(), vE.back(), 1);//between [0,1000] MeV
    fInterpolationsDirect[key]->SetTitle(";Energy [MeV];Range [mm]");
    
	fSplinesInverse[key] = std::make_unique<TSpline3>(("spinverse" + key).c_str(),
													  &(vR[0]),//range
													  &(vE[0]),//energy
													  vR.size(),
													  "b2, e2",
													  0., 0.);
	fInterpolationsInverse[key] = std::make_unique<TF1>(("func_inverse_" + key).c_str(),
														[key, this](double* x, double* p)
                                                        {return fSplinesInverse[key]->Eval(x[0]);},
                                                        vR.front(), vR.back(), 1);//between [0, Rmax] m
    fInterpolationsInverse[key]->SetTitle(";Range [mm];Energy [MeV]");
    
    ///////////////////////  ENERGY ---> STOPPING POWER
    fSplinesStoppings[key] = std::make_unique<TSpline3>(("spstopping_" + key).c_str(),
                                                        &(vE[0]),//energy
                                                        &(vStop[0]),//stopping power
                                                        vE.size(),
                                                        "b2, e2",
                                                        0., 0.);
	fStoppings[key] = std::make_unique<TF1>(("stopping_" + key).c_str(),
                                            [key, this](double* x, double* p)
                                            {return fSplinesStoppings[key]->Eval(x[0]);},
                                            0., 1000., 1);//between [0,1000] MeV
    fStoppings[key]->SetTitle(";Energy [MeV];#frac{dE}{dx} [MeV/mm]");
    
    ////////////////// RANGE ---> LONGITUDINAL STRAGGLING
    fSplinesLongStrag[key] = std::make_unique<TSpline3>(("spLongStragg_" + key).c_str(),
                                                        &(vR[0]),//energy
                                                        &(vLongStrag[0]),//long stragg
                                                        vR.size(),
                                                        "b2, e2",
                                                        0., 0.);
	fLongStrag[key] = std::make_unique<TF1>(("LongStragg_" + key).c_str(),
                                            [key, this](double* x, double* p)
                                            {return fSplinesLongStrag[key]->Eval(x[0]);},
                                            vR.front(), vR.back(), 1);//between [0,1000] MeV
    fLongStrag[key]->SetTitle(";Energy [MeV];Longitudianl straggling [mm]");
    
    ////////////////// RANGE ---> LATERAL STRAGGLING
    fSplinesLatStrag[key] = std::make_unique<TSpline3>(("spLatStragg_" + key).c_str(),
                                                       &(vR[0]),//energy
                                                       &(vLatStrag[0]),//lat stragg
                                                       vR.size(),
                                                       "b2, e2",
                                                       0., 0.);
	fLatStrag[key] = std::make_unique<TF1>(("LatStragg_" + key).c_str(),
                                           [key, this](double* x, double* p)
                                           {return fSplinesLatStrag[key]->Eval(x[0]);},
                                           vR.front(), vR.back(), 1);//between [0,1000] MeV
    fLatStrag[key]->SetTitle(";Energy [mm];Lateral stragging [mm]");
    
    //and finally store keys
    fKeys.push_back(key);
}

void ActPhysics::SRIM::Draw(std::string what, std::vector<std::string> keys)
{
    std::vector<std::string> keysToDraw { keys};
    if(keys.empty())
    {
        keysToDraw = fKeys;
    }
    //initialize canvas
    auto canvas = std::make_unique<TCanvas>(("canv_" + what).c_str(), "Canvas", 1);
    auto legend = std::make_unique<TLegend>(0.3, 0.3);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    //legend->SetTextSize(0.04);
    canvas->cd();
    int counter {1};
    std::vector<TF1*> funcsToDraw;
    for(auto& key : keysToDraw)
    {
        if (what == "direct")
            funcsToDraw.push_back(fInterpolationsDirect[key].get());
        else if(what == "inverse")
            funcsToDraw.push_back(fInterpolationsInverse[key].get());
        else if(what == "stopping")
            funcsToDraw.push_back(fStoppings[key].get());
        else if(what == "longStrag")
            funcsToDraw.push_back(fLongStrag[key].get());
        else if(what == "latStrag")
            funcsToDraw.push_back(fLatStrag[key].get());
        else
            throw std::runtime_error("Accepted values are: direct, inverse, stopping, longStrag and latStrag!");

        funcsToDraw.back()->SetLineColor(counter);
		funcsToDraw.back()->SetLineWidth(2);
		funcsToDraw.back()->Draw((counter > 1) ? "same" : "l");
		legend->AddEntry(funcsToDraw.back(), key.c_str(), "l");
		counter++;
    }
    legend->Draw();
    canvas->Update();
    canvas->WaitPrimitive("lat", "");
    canvas->Close();
    legend.reset();
}

double ActPhysics::SRIM::ComputeEnergyLoss(double Tini, std::string material, double thickness, double angle, int steps)
{
    double realThick {thickness / TMath::Abs(TMath::Cos(angle))};
    double dX { realThick / steps};
    double remainT { Tini};
    double energyLoss {};
    double stopping {};
    for(int i = 0; i < steps; i++)
    {
        stopping = EvalStoppingPower(material, remainT) * dX;
        if(std::isnan(stopping))
        {
            if(fDebug)
                std::cout<<"Kinetic energy under minimum given by SRIM!"<<'\n';
            //we can consider the particle to be stopped
        }
        remainT -= stopping;
        energyLoss = Tini - remainT;
        if(remainT <= 0.0)
        {
            energyLoss = Tini; // has lost all energy
            break;
        }
    }

    return energyLoss;
}

void ActPhysics::SRIM::CheckKeyIsStored(const std::string& key)
{
    if(!(std::find(fKeys.begin(), fKeys.end(), key) != fKeys.end()))
        throw std::runtime_error(("Error!: " + key + " couldn't be found in SRIM instance"));
}

void ActPhysics::SRIM::CheckFunctionArgument(double val)
{
    if(fDebug && val < 0.)
        std::cout<<"Warning: Negative value passed to Eval function"<<'\n';
}
