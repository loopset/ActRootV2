#include "ActSRIM.h"

#include "ActInputParser.h"

#include "TCanvas.h"
#include "TF1.h"
#include "TLegend.h"
#include "TMath.h"
#include "TSpline.h"
#include "TVirtualPad.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

ActPhysics::SRIM::SRIM(const std::string& key, const std::string& file)
{
    ReadTable(key, file);
}

bool ActPhysics::SRIM::IsBreakLine(const std::string& line)
{
    int count {};
    int pos {};
    while((pos = line.find("-", pos)) != std::string::npos || (pos = line.find("=", pos)) != std::string::npos)
    {
        pos++;
        count++;
    }
    // Delimiter lines are recognized by having a large amount of
    // - or = symbols
    return (count > 6);
}
double ActPhysics::SRIM::ConvertToDouble(std::string& str, const std::string& unit)
{
    // Replace commas with points
    std::replace(str.begin(), str.end(), ',', '.');
    // Convert to double
    double ret {};
    try
    {
        ret = std::stod(str);
    }
    catch(std::exception& e)
    {
        throw std::runtime_error("SRIM::ConvertToDouble(): could not convert " + str + " to double!");
    }
    // And now assign units
    // Default should be MeV and mm
    // Energy
    if(unit == "eV")
        ret *= 1e-6;
    else if(unit == "keV")
        ret *= 1e-3;
    else if(unit == "MeV")
        ret *= 1;
    else if(unit == "GeV")
        ret *= 1e3;
    // Length
    else if(unit == "A")
        ret *= 1e-7;
    else if(unit == "um")
        ret *= 1e-3;
    else if(unit == "mm")
        ret *= 1;
    else if(unit == "cm")
        ret *= 1e1;
    else if(unit == "m")
        ret *= 1e3;
    else if(unit == "km")
        ret *= 1e6;
    // None for stopping power (should be obtained correctly in SRIM executable)
    else if(unit == "None")
        ret *= 1;
    else
        throw std::runtime_error("SRIM::ConvertToDouble(): unit " + unit + " not recognized!");
    return ret;
}

ActPhysics::SRIM::PtrSpline
ActPhysics::SRIM::GetSpline(std::vector<double>& x, std::vector<double>& y, const std::string& name)
{
    return std::make_unique<TSpline3>(name.c_str(), &(x[0]), &(y[0]), x.size(), "b2,e2", 0., 0.);
}

void ActPhysics::SRIM::ReadTable(const std::string& key, const std::string& file)
{
    std::ifstream streamer(file);
    if(!streamer)
        throw std::runtime_error("SRIM::ReadInterpolations(): could not open file " + file);

    // Init vectors
    std::vector<double> vE, vStop, vR, vLongStrag, vLatStrag;
    // Read lines
    std::string line {};
    bool read {};
    while(std::getline(streamer, line))
    {
        if(IsBreakLine(line))
            continue;
        // Find beginning of columns
        if(line.find("Straggling") != std::string::npos)
        {
            read = true;
            continue;
        }
        // Find end
        if(line.find("Multiply") != std::string::npos)
            break;
        // Read!
        if(read)
        {
            std::string e, ue, electro, nucl, r, ur, ls, uls, as, uas;
            std::istringstream lineStreamer {line};
            while(lineStreamer >> e >> ue >> electro >> nucl >> r >> ur >> ls >> uls >> as >> uas)
            {
                // Energy
                vE.push_back(ConvertToDouble(e, ue));
                // Stopping power
                vStop.push_back(ConvertToDouble(nucl, "None") + ConvertToDouble(electro, "None"));
                // Range
                vR.push_back(ConvertToDouble(r, ur));
                // Longitudinal straggling
                vLongStrag.push_back(ConvertToDouble(ls, uls));
                // Lateral straggling
                vLatStrag.push_back(ConvertToDouble(as, uas));
            }
        }
    }
    streamer.close();

    // Init splines and funcs
    // 1-> Energy -> Range
    fSplinesDirect[key] = GetSpline(vE, vR, "speEtoR");
    fInterpolationsDirect[key] =
        std::make_unique<TF1>(("fEtoR" + key).c_str(), [key, this](double* x, double* p)
                              { return fSplinesDirect[key]->Eval(x[0]); }, vE.front(), vE.back(), 1);
    fInterpolationsDirect[key]->SetTitle(";Energy [MeV];Range [mm]");
    // 2-> Range -> Energy
    fSplinesInverse[key] = GetSpline(vR, vE, "speRtoE");
    fInterpolationsInverse[key] =
        std::make_unique<TF1>(("fRtoE" + key).c_str(), [key, this](double* x, double* p)
                              { return fSplinesInverse[key]->Eval(x[0]); }, vR.front(), vR.back(), 1);
    fInterpolationsInverse[key]->SetTitle(";Range [mm];Energy [MeV]");

    // 3-> E to Stopping
    fSplinesStoppings[key] = GetSpline(vE, vStop, "speEtodE");
    fStoppings[key] = std::make_unique<TF1>(("fEtodE" + key).c_str(), [key, this](double* x, double* p)
                                            { return fSplinesStoppings[key]->Eval(x[0]); }, vE.front(), vE.back(), 1);
    fStoppings[key]->SetTitle(";Energy [MeV];#frac{dE}{dx} [MeV/mm]");

    // 4-> R to LS
    fSplinesLongStrag[key] = GetSpline(vR, vLongStrag, "speRtoLongS");
    fLongStrag[key] = std::make_unique<TF1>(("fRtoLongS" + key).c_str(), [key, this](double* x, double* p)
                                            { return fSplinesLongStrag[key]->Eval(x[0]); }, vR.front(), vR.back(), 1);
    fLongStrag[key]->SetTitle(";Range [mm];Longitudinal straggling [mm]");

    // 5-> R to LatS
    fSplinesLatStrag[key] = GetSpline(vR, vLatStrag, "speRtoLatS");
    fLatStrag[key] = std::make_unique<TF1>(("fRtoLatS" + key).c_str(), [key, this](double* x, double* p)
                                           { return fSplinesLatStrag[key]->Eval(x[0]); }, vR.front(), vR.back(), 1);
    fLatStrag[key]->SetTitle(";Range [mm];Lateral stragging [mm]");

    // and finally store keys
    fKeys.push_back(key);
}

void ActPhysics::SRIM::ReadInterpolations(std::string key, std::string fileName)
{
    std::ifstream streamer(fileName.c_str());
    if(!streamer)
        throw std::runtime_error("SRIM::ReadInterpolations(): could not open file " + fileName);
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
                                                     &(vE[0]), // energy
                                                     &(vR[0]), // range
                                                     vE.size(), "b2, e2", 0., 0.);
    fInterpolationsDirect[key] =
        std::make_unique<TF1>(("func_direct_" + key).c_str(), [key, this](double* x, double* p)
                              { return fSplinesDirect[key]->Eval(x[0]); }, vE.front(), vE.back(), 1);
    fInterpolationsDirect[key]->SetTitle(";Energy [MeV];Range [mm]");

    fSplinesInverse[key] = std::make_unique<TSpline3>(("spinverse" + key).c_str(),
                                                      &(vR[0]), // range
                                                      &(vE[0]), // energy
                                                      vR.size(), "b2, e2", 0., 0.);
    fInterpolationsInverse[key] =
        std::make_unique<TF1>(("func_inverse_" + key).c_str(), [key, this](double* x, double* p)
                              { return fSplinesInverse[key]->Eval(x[0]); }, vR.front(), vR.back(), 1);
    fInterpolationsInverse[key]->SetTitle(";Range [mm];Energy [MeV]");

    ///////////////////////  ENERGY ---> STOPPING POWER
    fSplinesStoppings[key] = std::make_unique<TSpline3>(("spstopping_" + key).c_str(),
                                                        &(vE[0]),    // energy
                                                        &(vStop[0]), // stopping power
                                                        vE.size(), "b2, e2", 0., 0.);
    fStoppings[key] = std::make_unique<TF1>(("stopping_" + key).c_str(), [key, this](double* x, double* p)
                                            { return fSplinesStoppings[key]->Eval(x[0]); }, 0., 1000., 1);
    fStoppings[key]->SetTitle(";Energy [MeV];#frac{dE}{dx} [MeV/mm]");

    ////////////////// RANGE ---> LONGITUDINAL STRAGGLING
    fSplinesLongStrag[key] = std::make_unique<TSpline3>(("spLongStragg_" + key).c_str(),
                                                        &(vR[0]),         // energy
                                                        &(vLongStrag[0]), // long stragg
                                                        vR.size(), "b2, e2", 0., 0.);
    fLongStrag[key] = std::make_unique<TF1>(("LongStragg_" + key).c_str(), [key, this](double* x, double* p)
                                            { return fSplinesLongStrag[key]->Eval(x[0]); }, vR.front(), vR.back(), 1);
    fLongStrag[key]->SetTitle(";Energy [MeV];Longitudianl straggling [mm]");

    ////////////////// RANGE ---> LATERAL STRAGGLING
    fSplinesLatStrag[key] = std::make_unique<TSpline3>(("spLatStragg_" + key).c_str(),
                                                       &(vR[0]),        // energy
                                                       &(vLatStrag[0]), // lat stragg
                                                       vR.size(), "b2, e2", 0., 0.);
    fLatStrag[key] = std::make_unique<TF1>(("LatStragg_" + key).c_str(), [key, this](double* x, double* p)
                                           { return fSplinesLatStrag[key]->Eval(x[0]); }, vR.front(), vR.back(), 1);
    fLatStrag[key]->SetTitle(";Energy [mm];Lateral stragging []");

    // and finally store keys
    fKeys.push_back(key);
}

void ActPhysics::SRIM::Draw(const std::string& what, const std::vector<std::string>& keys)
{
    std::vector<std::string> keysToDraw {keys};
    if(keys.empty())
    {
        keysToDraw = fKeys;
    }
    // initialize canvas
    auto canvas = std::make_unique<TCanvas>(("canv_" + what).c_str(), "Canvas", 1);
    auto legend = std::make_unique<TLegend>(0.3, 0.3);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    // legend->SetTextSize(0.04);
    canvas->cd();
    int counter {1};
    std::vector<TF1*> funcsToDraw;
    for(auto& key : keysToDraw)
    {
        if(what == "direct")
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

double ActPhysics::SRIM::Slow(const std::string& material, double Tini, double thickness, double angleInRad)
{
    // Init range
    auto RIni {EvalDirect(material, Tini)};
    // Compute distance
    auto dist {thickness / TMath::Cos(angleInRad)};
    // New range
    auto RAfter {RIni - dist};
    if(RAfter <= 0)
        return 0;
    auto ret {EvalInverse(material, RAfter)};
    if(ret > Tini)
        throw std::runtime_error(
            "SRIM::Slow(): Tafter > Tini due to TSpline precision. Consider using a larger step in thickness");
    return ret;
}


double
ActPhysics::SRIM::EvalInitialEnergy(const std::string& material, double Tafter, double thickness, double angleInRad)
{
    // After range
    auto RAfter {EvalDirect(material, Tafter)};
    // Distance
    auto dist {thickness / TMath::Cos(angleInRad)};
    // New range
    auto RIni {RAfter + dist};
    return EvalInverse(material, RIni);
}

bool ActPhysics::SRIM::CheckKeyIsStored(const std::string& key)
{
    return std::find(fKeys.begin(), fKeys.end(), key) != fKeys.end();
}

void ActPhysics::SRIM::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    // Parse block: read all tokens as keys and values as paths to files
    for(const auto& token : block->GetTokens())
        ReadTable(token, block->GetString(token));
}

void ActPhysics::SRIM::ReadConfiguration(const std::string& file)
{
    ActRoot::InputParser parser {file};
    auto block {parser.GetBlock("SRIM")};
    ReadConfiguration(block);
}
