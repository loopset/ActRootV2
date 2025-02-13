#include "ActSRIM.h"

#include "ActInputParser.h"

#include "TCanvas.h"
#include "TGraph.h"
#include "TMath.h"
#include "TROOT.h"
#include "TRandom.h"
#include "TSpline.h"
#include "TString.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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
    return std::make_shared<TSpline3>(name.c_str(), &(x[0]), &(y[0]), x.size(), "b2,e2", 0., 0.);
}

ActPhysics::SRIM::PtrGraph
ActPhysics::SRIM::GetGraph(std::vector<double>& x, std::vector<double>& y, const std::string& name)
{
    auto g {std::make_shared<TGraph>(x.size(), x.data(), y.data())};
    g->SetName(("g" + name).c_str());
    g->SetBit(TGraph::kIsSortedX);
    return std::move(g);
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
    fSplinesDirect[key] = GetSpline(vE, vR, "EtoR");
    fGraphsDirect[key] = GetGraph(vE, vR, "EtoT");
    fGraphsDirect[key]->SetTitle(";Energy [MeV];Range [mm]");
    // 2-> Range -> Energy
    fSplinesInverse[key] = GetSpline(vR, vE, "RtoE");
    fGraphsInverse[key] = GetGraph(vR, vE, "RtoE");
    fGraphsInverse[key]->SetTitle(";Range [mm];Energy [MeV]");

    // 3-> E to Stopping
    fSplinesStoppings[key] = GetSpline(vE, vStop, "EtodE");
    fGraphsStoppings[key] = GetGraph(vE, vStop, "EtodE");
    fGraphsStoppings[key]->SetTitle(";Energy [MeV];#frac{dE}{dx} [MeV/mm]");

    // 4-> R to LS
    fSplinesLongStrag[key] = GetSpline(vR, vLongStrag, "RtoLongS");
    fGraphsLongStrag[key] = GetGraph(vR, vLongStrag, "RtoLongS");
    fGraphsLongStrag[key]->SetTitle(";Range [mm];Longitudinal straggling [mm]");

    // 5-> R to LatS
    fSplinesLatStrag[key] = GetSpline(vR, vLatStrag, "RtoLatS");
    fGraphsLatStrag[key] = GetGraph(vR, vLatStrag, "RtoLatS");
    fGraphsLatStrag[key]->SetTitle(";Range [mm];Lateral stragging [mm]");

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

    // Init splines and funcs
    // 1-> Energy -> Range
    fSplinesDirect[key] = GetSpline(vE, vR, "EtoR");
    fGraphsDirect[key] = GetGraph(vE, vR, "EtoT");
    fGraphsDirect[key]->SetTitle(";Energy [MeV];Range [mm]");
    // 2-> Range -> Energy
    fSplinesInverse[key] = GetSpline(vR, vE, "RtoE");
    fGraphsInverse[key] = GetGraph(vR, vE, "RtoE");
    fGraphsInverse[key]->SetTitle(";Range [mm];Energy [MeV]");

    // 3-> E to Stopping
    fSplinesStoppings[key] = GetSpline(vE, vStop, "EtodE");
    fGraphsStoppings[key] = GetGraph(vE, vStop, "EtodE");
    fGraphsStoppings[key]->SetTitle(";Energy [MeV];#frac{dE}{dx} [MeV/mm]");

    // 4-> R to LS
    fSplinesLongStrag[key] = GetSpline(vR, vLongStrag, "RtoLongS");
    fGraphsLongStrag[key] = GetGraph(vR, vLongStrag, "RtoLongS");
    fGraphsLongStrag[key]->SetTitle(";Range [mm];Longitudinal straggling [mm]");

    // 5-> R to LatS
    fSplinesLatStrag[key] = GetSpline(vR, vLatStrag, "RtoLatS");
    fGraphsLatStrag[key] = GetGraph(vR, vLatStrag, "RtoLatS");
    fGraphsLatStrag[key]->SetTitle(";Range [mm];Lateral stragging [mm]");

    // and finally store keys
    fKeys.push_back(key);
}

// All eval functions
double ActPhysics::SRIM::EvalDirect(const std::string& key, double energy)
{
    return fGraphsDirect[key]->Eval(energy, (fUseSpline) ? fSplinesDirect[key].get() : nullptr);
}

double ActPhysics::SRIM::EvalInverse(const std::string& key, double range)
{
    return fGraphsInverse[key]->Eval(range, (fUseSpline) ? fSplinesInverse[key].get() : nullptr);
}

double ActPhysics::SRIM::EvalStoppingPower(const std::string& key, double energy)
{
    return fGraphsStoppings[key]->Eval(energy, (fUseSpline) ? fSplinesStoppings[key].get() : nullptr);
}

double ActPhysics::SRIM::EvalLongStraggling(const std::string& key, double range)
{
    return fGraphsLongStrag[key]->Eval(range, (fUseSpline) ? fSplinesLongStrag[key].get() : nullptr);
}

double ActPhysics::SRIM::EvalLatStraggling(const std::string& key, double range)
{
    return fGraphsLatStrag[key]->Eval(range, (fUseSpline) ? fSplinesLatStrag[key].get() : nullptr);
}

void ActPhysics::SRIM::Draw(const std::vector<std::string>& keys)
{
    std::vector<std::string> keysToDraw {keys};
    if(keys.empty())
        keysToDraw = fKeys;

    // Canvas and legend
    std::vector<TCanvas*> cs;
    int idx {};
    for(const auto& key : keysToDraw)
    {
        cs.push_back(new TCanvas {TString::Format("cSRIM%d", idx), TString::Format("SRIM for %s", key.c_str())});
        auto* c {cs.back()};
        c->DivideSquare(4);
        int inner {1};
        for(auto ptr : {fGraphsDirect[key], fGraphsInverse[key], fGraphsStoppings[key], fGraphsLongStrag[key]})
        {
            c->cd(inner);
            ptr->SetLineWidth(2);
            ptr->DrawClone("al");
            inner++;
        }
        idx++;
    }
    gROOT->SetSelectedPad(nullptr);
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

double ActPhysics::SRIM::SlowWithStraggling(const std::string& material, double Tini, double thickness,
                                            double angleInRad, TRandom* rand)
{
    // Compute effective length
    double dist {thickness / TMath::Cos(angleInRad)};
    // Initial range
    auto RIni {EvalRange(material, Tini)};
    // Initial straggling
    auto uRini {EvalLongStraggling(material, RIni)};
    // New range
    auto RAfter {RIni - dist};
    if(RAfter <= 0)
        return 0;
    // Final straggling
    auto uRAfter {EvalLongStraggling(material, RAfter)};
    // Build uncertainty in distance
    auto udist {TMath::Sqrt(uRini * uRini - uRAfter * uRAfter)};
    // New distance
    dist = (rand ? rand : gRandom)->Gaus(dist, udist);
    RAfter = RIni - dist;
    if(RAfter <= 0)
        return 0;
    return EvalEnergy(material, RAfter);
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

double ActPhysics::SRIM::TravelledDistance(const std::string& material, double Tini, double Tafter)
{
    // Range at init point
    auto Rini {EvalRange(material, Tini)};
    // Range at end point
    auto Rafter {EvalRange(material, Tini)};
    return (Rini - Rafter);
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
