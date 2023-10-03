#include "HistogramPainter.h"

#include "Colors.h"
#include "InputParser.h"
#include "InputIterator.h"
#include "Rtypes.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TPCData.h"
#include "TString.h"
#include "TStyle.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

ActRoot::HistogramPainter::HistogramPainter(const std::string& detfile)
{
    ReadDetFile(detfile);
    Init();
}

void ActRoot::HistogramPainter::ReadDetFile(const std::string& detfile)
{
    ActRoot::InputParser parser {detfile};
    auto headers {parser.GetBlockHeaders()};
    //TPC
    if(std::find(headers.begin(), headers.end(), "Actar") != headers.end())
    {
        auto config {parser.GetBlock("Actar")};
        fTPC = TPCParameters(config->GetString("Type"));
        if(config->CheckTokenExists("RebinZ", true))
            fTPC.SetREBINZ(config->GetInt("RebinZ"));
    }
    //Silicon
    if(std::find(headers.begin(), headers.end(), "Silicons") != headers.end())
    {
        //Read layer setup
        auto config {parser.GetBlock("Silicons")};
        auto layers {config->GetStringVector("Layers")};
        //Read action file
        auto legacy {config->GetStringVector("Names")};
        auto file {config->GetString("Actions")};
        fSil.ReadActions(layers, legacy, file);
    }

    //And init
    Init();
}

void ActRoot::HistogramPainter::Init()
{
    //Check TCanvas are initialized
    if(!fCanvs[1])
        std::cout<<BOLDMAGENTA<<"HistogramPainter canvas 1 not initialized!"<<RESET<<'\n';
    if(!fCanvs[2])
        std::cout<<BOLDMAGENTA<<"HistogramPainter canvas 2 not initialized!"<<RESET<<'\n';
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //TPC
    fCanvs[1]->Divide(3, 2);
    //Pad
    fHistTpc[1] = std::make_shared<TH2F>("hPad", "Pad;X [pad];Y [pad]",
                                        fTPC.GetNPADSX(), 0, fTPC.GetNPADSX(),
                                        fTPC.GetNPADSY(), 0, fTPC.GetNPADSY());
    //Side
    fHistTpc[2] = std::make_shared<TH2F>("hSide", "Side;X [pad];Z [tb]",
                                        fTPC.GetNPADSX(), 0, fTPC.GetNPADSX(),
                                        fTPC.GetNPADSZ() / fTPC.GetREBINZ(), 0, fTPC.GetNPADSZ());
    //Front
    fHistTpc[3] = std::make_shared<TH2F>("hFront", "Front;Y [pad];Z [tb]",
                                        fTPC.GetNPADSY(), 0, fTPC.GetNPADSY(),
                                        fTPC.GetNPADSZ() / fTPC.GetREBINZ(), 0 , fTPC.GetNPADSZ());
    for(auto& [_, h] : fHistTpc)
        h->SetStats(false);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Silicons
    fSilMap = {
        {"l0", {{1, 3}, {2, 3}, {3, 3},
                {1, 2}, {2, 2}, {3, 2},
                {1, 1}, {2, 1}, {3, 1}}},
        {"f0", {{1, 4}, {2, 4}, {3, 4},
                {1, 3}, {3, 3},//missing central silicon
                {1, 2}, {2, 2}, {3, 2},
                {1, 1}, {2, 1}, {3, 1}}},
        {"f1", {{1, 4}, {2, 4}, {3, 4},
                {1, 3}, {3, 3},//missing central silicon
                {1, 2}, {2, 2}, {3, 2},
                {1, 1}, {2, 1}, {3, 1}}},
    };
    fCanvs[2]->Divide(3, 2);
    //Left
    fHistSil[1] = std::make_shared<TH2F>("hL0", "L0;Col;Row",
                                         3, 0.5, 3.5,
                                         3, 0.5, 3.5);
    //Front L0
    fHistSil[5] = std::make_shared<TH2F>("hF0", "F0;Col;Row",
                                         3, 0.5, 3.5,
                                         4, 0.5, 3.5);
    //Front L1
    fHistSil[6] = std::make_shared<TH2F>("hF1", "F1;Col;Row",
                                         3, 0.5, 3.5,
                                         4, 0.5, 3.5);
    for(auto& [_, h] : fHistSil)
    {
        h->SetStats(false);
        //Style for TEXT option
        h->SetMarkerColor(kRed);
        h->SetMarkerSize(1.8);
    }
    //For number of decimals
    gStyle->SetPaintTextFormat(".2f MeV");
}

void ActRoot::HistogramPainter::Fill()
{
    //TPC histograms
    for(auto& voxel : fWrap->GetCurrentTPCData()->fVoxels)
    {
        //Pad
        fHistTpc[1]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Y(), voxel.GetCharge());
        //Side
        fHistTpc[2]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Z(), voxel.GetCharge());
        //Front
        fHistTpc[3]->Fill(voxel.GetPosition().Y(), voxel.GetPosition().Z(), voxel.GetCharge());
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Sil histograms
    //1-> Left
    FillSilHisto(1, "l0");
    //2-> Front 0
    FillSilHisto(5, "f0");
    //3-> Front 1
    FillSilHisto(6, "f1");
}

void ActRoot::HistogramPainter::Draw()
{
    //TPC
    for(auto& [pad, h] : fHistTpc)
    {
        fCanvs[1]->cd(pad);
        h->Draw("colz");
    }
    fCanvs[1]->Update();
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //Sil
    for(auto& [pad, h] : fHistSil)
    {
        fCanvs[2]->cd(pad);
        h->Draw("col text");
    }
    //Draw also pad
    fCanvs[2]->cd(4);
    fHistTpc[1]->Draw("colz");
    fCanvs[2]->Update();
}

void ActRoot::HistogramPainter::Reset()
{
    //TPC canvas
    for(auto& [pad, h] : fHistTpc)
    {
        h->Reset();
        h->GetXaxis()->UnZoom();
        h->GetYaxis()->UnZoom();
        fCanvs[1]->cd(pad)->Modified();
    }
    fCanvs[1]->Update();
    //////////////////////////////////////////////////////////////////////////////////////////////
    //Sil canvas
    for(auto& [pad, h] : fHistSil)
    {
        h->Reset();
        h->GetXaxis()->UnZoom();
        h->GetYaxis()->UnZoom();
        fCanvs[2]->cd(pad)->Modified();
    }
    fCanvs[2]->cd(4)->Modified();
    fCanvs[2]->Update();
}

TCanvas* ActRoot::HistogramPainter::SetCanvas(int i, const std::string &title,
                                          double w, double h)
{
    fCanvs[i] = new TCanvas(TString::Format("cHP%d", i), title.c_str(),
                                          w, h);
    //Different settings
    fCanvs[i]->ToggleToolBar();
    //fCanvs[i]->ToggleEventStatus();
    return fCanvs[i];
}

void ActRoot::HistogramPainter::FillSilHisto(int pad, const std::string& layer)
{
    auto E {fWrap->GetCurrentSilData()->fSiE[layer]};
    auto N {fWrap->GetCurrentSilData()->fSiN[layer]};
    for(int hit = 0, sizeE = E.size(); hit < sizeE; hit++)
    {
        auto bins {fSilMap[layer][N[hit]]};
        fHistSil[pad]->Fill(bins.first, bins.second, E[hit]);
    }
}
