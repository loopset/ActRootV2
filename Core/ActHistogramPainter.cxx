#include "ActHistogramPainter.h"

#include "ActColors.h"
#include "ActInputIterator.h"
#include "ActInputParser.h"
#include "ActTPCData.h"
#include "ActTPCPhysics.h"

#include "Rtypes.h"

#include "TCanvas.h"
#include "TColor.h"
#include "TEnv.h"
#include "TH2.h"
#include "TPolyMarker.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

void ActRoot::HistogramPainter::ReadConfigurationFile(const std::string& file)
{
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/hist.painter";
    std::string realfile {};
    if(!gSystem->AccessPathName(envfile.c_str())) // bizarre bool return for this function
        realfile = envfile;
    else if(file.length() > 0)
        realfile = file;
    else
    {
        std::cout << "Using default configuration for HistogramPainter" << '\n';
        return;
    }
    // Init parser
    ActRoot::InputParser parser {realfile};
    auto hb {parser.GetBlock("HistogramPainter")};
    if(hb->CheckTokenExists("Palette", true) && hb->CheckTokenExists("Reverse", true))
        SetPalette(hb->GetString("Palette"), hb->GetBool("Reverse"));
    if(hb->CheckTokenExists("Palette", true) && !hb->CheckTokenExists("Reverse", true))
        SetPalette(hb->GetString("Palette"));
    if(hb->CheckTokenExists("ShowStats", true))
        fShowHistStats = hb->GetBool("ShowStats");
}

void ActRoot::HistogramPainter::ReadDetFile(const std::string& detfile)
{
    ActRoot::InputParser parser {detfile};
    auto headers {parser.GetBlockHeaders()};
    // TPC
    if(std::find(headers.begin(), headers.end(), "Actar") != headers.end())
    {
        auto config {parser.GetBlock("Actar")};
        fTPC = TPCParameters(config->GetString("Type"));
        if(config->CheckTokenExists("RebinZ", true))
            fTPC.SetREBINZ(config->GetInt("RebinZ"));
    }
    // Silicon
    if(std::find(headers.begin(), headers.end(), "Silicons") != headers.end())
    {
        // Read layer setup
        auto config {parser.GetBlock("Silicons")};
        auto layers {config->GetStringVector("Layers")};
        // Read action file
        auto legacy {config->GetStringVector("Names")};
        auto file {config->GetString("Actions")};
        fSil.ReadActions(layers, legacy, file);
    }
}

void ActRoot::HistogramPainter::Init()
{
    // Check TCanvas are initialized
    if(!fCanvs[1])
        std::cout << BOLDMAGENTA << "HistogramPainter canvas 1 not initialized!" << RESET << '\n';
    if(!fCanvs[2])
        std::cout << BOLDMAGENTA << "HistogramPainter canvas 2 not initialized!" << RESET << '\n';
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // TPC
    fCanvs[1]->Divide(3, 2);
    // Pad
    fHistTpc[1] = std::make_shared<TH2F>("hPad", "Pad;X [pad];Y [pad]", fTPC.GetNPADSX(), 0, fTPC.GetNPADSX(),
                                         fTPC.GetNPADSY(), 0, fTPC.GetNPADSY());
    // Side
    fHistTpc[2] = std::make_shared<TH2F>("hSide", "Side;X [pad];Z [tb]", fTPC.GetNPADSX(), 0, fTPC.GetNPADSX(),
                                         fTPC.GetNPADSZ() / fTPC.GetREBINZ(), 0, fTPC.GetNPADSZ());
    // Front
    fHistTpc[3] = std::make_shared<TH2F>("hFront", "Front;Y [pad];Z [tb]", fTPC.GetNPADSY(), 0, fTPC.GetNPADSY(),
                                         fTPC.GetNPADSZ() / fTPC.GetREBINZ(), 0, fTPC.GetNPADSZ());
    ////////////////////////
    // Clusters!
    // Pad
    fHistTpc[4] = std::make_shared<TH2F>("hPadC", "Clusters in pad;X [pad];Y [pad]", fTPC.GetNPADSX(), 0,
                                         fTPC.GetNPADSX(), fTPC.GetNPADSY(), 0, fTPC.GetNPADSY());
    // Side
    fHistTpc[5] = std::make_shared<TH2F>("hSideC", "Clusters in side;X [pad];Z [tb]", fTPC.GetNPADSX(), 0,
                                         fTPC.GetNPADSX(), fTPC.GetNPADSZ() / fTPC.GetREBINZ(), 0, fTPC.GetNPADSZ());
    // Front
    fHistTpc[6] = std::make_shared<TH2F>("hFrontC", "Clusters in front;Y [pad];Z [tb]", fTPC.GetNPADSY(), 0,
                                         fTPC.GetNPADSY(), fTPC.GetNPADSZ() / fTPC.GetREBINZ(), 0, fTPC.GetNPADSZ());
    for(auto& [_, h] : fHistTpc)
        h->SetStats(fShowHistStats);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Silicons
    fSilMap = {
        {"l0", {{1, 3}, {2, 3}, {3, 3}, {1, 2}, {2, 2}, {3, 2}, {1, 1}, {2, 1}, {3, 1}}},
        {"f0",
         {{1, 4},
          {2, 4},
          {3, 4},
          {1, 3},
          {3, 3}, // missing central silicon
          {1, 2},
          {2, 2},
          {3, 2},
          {1, 1},
          {2, 1},
          {3, 1}}},
        {"f1",
         {{1, 4},
          {2, 4},
          {3, 4},
          {1, 3},
          {3, 3}, // missing central silicon
          {1, 2},
          {2, 2},
          {3, 2},
          {1, 1},
          {2, 1},
          {3, 1}}},
    };
    fCanvs[2]->Divide(3, 2);
    // Left
    fHistSil[1] = std::make_shared<TH2F>("hL0", "L0;Col;Row", 3, 0.5, 3.5, 3, 0.5, 3.5);
    // Front L0
    fHistSil[5] = std::make_shared<TH2F>("hF0", "F0;Col;Row", 3, 0.5, 3.5, 4, 0.5, 4.5);
    // Front L1
    fHistSil[6] = std::make_shared<TH2F>("hF1", "F1;Col;Row", 3, 0.5, 3.5, 4, 0.5, 4.5);
    for(auto& [_, h] : fHistSil)
    {
        h->SetStats(fShowHistStats);
        // Style for TEXT option
        h->SetMarkerColor(kRed);
        h->SetMarkerSize(1.8);
    }
    // For number of decimals
    gStyle->SetPaintTextFormat(".2f MeV");
}

void ActRoot::HistogramPainter::Fill()
{
    // TPC histograms
    for(auto& voxel : fWrap->GetCurrentTPCData()->fVoxels)
    {
        // Pad
        fHistTpc[1]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Y(), voxel.GetCharge());
        // Side
        fHistTpc[2]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Z(), voxel.GetCharge());
        // Front
        fHistTpc[3]->Fill(voxel.GetPosition().Y(), voxel.GetPosition().Z(), voxel.GetCharge());
    }
    FillClusterHistos();
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Sil histograms
    // 1-> Left
    FillSilHisto(1, "l0");
    // 2-> Front 0
    FillSilHisto(5, "f0");
    // 3-> Front 1
    FillSilHisto(6, "f1");
}

void ActRoot::HistogramPainter::Draw()
{
    // TPC
    for(auto& [pad, h] : fHistTpc)
    {
        fCanvs[1]->cd(pad);
        h->Draw("colz");
    }
    DrawPolyLines();
    DrawPolyMarkers();
    fCanvs[1]->Update();
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Sil
    for(auto& [pad, h] : fHistSil)
    {
        fCanvs[2]->cd(pad);
        h->Draw("col text");
    }
    // Draw also pad
    fCanvs[2]->cd(4);
    fHistTpc[1]->Draw("colz");
    fCanvs[2]->Update();
}

void ActRoot::HistogramPainter::Reset()
{
    // TPC canvas
    for(auto& [pad, h] : fHistTpc)
    {
        h->Reset();
        h->GetXaxis()->UnZoom();
        h->GetYaxis()->UnZoom();
        fCanvs[1]->cd(pad)->Modified();
    }
    // Clear also polylines
    fPolyTpc.clear();
    fMarkerTpc.clear();
    fCanvs[1]->Update();
    //////////////////////////////////////////////////////////////////////////////////////////////
    // Sil canvas
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

TCanvas* ActRoot::HistogramPainter::SetCanvas(int i, const std::string& title, double w, double h)
{
    fCanvs[i] = new TCanvas(TString::Format("cHP%d", i), title.c_str(), w, h);
    // Different settings
    fCanvs[i]->ToggleToolBar();
    // fCanvs[i]->ToggleEventStatus();
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

void ActRoot::HistogramPainter::AttachBinToCluster(std::shared_ptr<TH2F> h, double x, double y, int clusterID)
{
    if(!h)
        return;
    auto xbin {h->GetXaxis()->FindFixBin(x)};
    auto ybin {h->GetYaxis()->FindFixBin(y)};
    // std::cout<<"xbin = "<<xbin<<" ybin = "<<ybin<<'\n';
    h->SetBinContent(xbin, ybin, clusterID + 1);
}

void ActRoot::HistogramPainter::SetTPCPhysicsPointer(VData* p)
{
    auto casted {dynamic_cast<TPCPhysics*>(p)};
    if(casted)
    {
        fTPCPhysics = casted;
        fTPCPhysics->Print();
    }
    else
        std::cout << "Could not cast pointer to TPCPhysics" << '\n';
}

void ActRoot::HistogramPainter::FillClusterHistos()
{
    if(!fTPCPhysics)
        return;
    for(const auto& cluster : fTPCPhysics->fClusters)
    {
        for(const auto& voxel : cluster.GetVoxels())
        {
            const auto& pos {voxel.GetPosition()};
            // Pad
            AttachBinToCluster(fHistTpc[4], pos.X(), pos.Y(), cluster.GetClusterID());
            // Side
            AttachBinToCluster(fHistTpc[5], pos.X(), pos.Z(), cluster.GetClusterID());
            // Front
            AttachBinToCluster(fHistTpc[6], pos.Y(), pos.Z(), cluster.GetClusterID());
        }
    }
    // Set basic parameters to a better visualization of TPaletteAxis
    int nclusters {static_cast<int>(fTPCPhysics->fClusters.size())};
    int min {};
    int max {};
    int ndiv {};
    if(nclusters <= 1)
    {
        ndiv = 1;
        min = 0;
        max = 1;
    }
    else
    {
        ndiv = nclusters;
        min = 1; // cluster are always plotted from [1, nclusters]
        max = nclusters;
    }
    std::vector<int> clusterPads {4, 5, 6};
    for(const auto& pad : clusterPads)
    {
        fHistTpc[pad]->SetMinimum(min);
        fHistTpc[pad]->SetMaximum(max);
        fHistTpc[pad]->GetZaxis()->SetNdivisions(ndiv); // = nlabels
    }
}

void ActRoot::HistogramPainter::DrawPolyLines()
{
    if(!fTPCPhysics)
        return;
    for(const auto& cluster : fTPCPhysics->fClusters)
    {
        // Pad
        fPolyTpc[4].push_back(
            cluster.GetLine().GetPolyLine("xy", fTPC.GetNPADSX(), fTPC.GetNPADSY(), fTPC.GetNPADSZ()));
        // Side
        fPolyTpc[5].push_back(
            cluster.GetLine().GetPolyLine("xz", fTPC.GetNPADSX(), fTPC.GetNPADSY(), fTPC.GetNPADSZ()));
        // Front
        fPolyTpc[6].push_back(
            cluster.GetLine().GetPolyLine("yz", fTPC.GetNPADSX(), fTPC.GetNPADSY(), fTPC.GetNPADSZ()));
    }
    // Set line parameters
    for(auto& [pad, proj] : fPolyTpc)
    {
        for(auto& poly : proj)
        {
            poly->SetLineWidth(2);
            // Draw
            fCanvs[1]->cd(pad);
            poly->Draw("same");
        }
    }
}

void ActRoot::HistogramPainter::DrawPolyMarkers()
{
    if(!fTPCPhysics)
        return;
    // Reset and init
    fMarkerTpc[4] = std::make_shared<TPolyMarker>();
    fMarkerTpc[5] = std::make_shared<TPolyMarker>();
    fMarkerTpc[6] = std::make_shared<TPolyMarker>();
    for(const auto& [indexes, rp] : fTPCPhysics->fRPs)
    {
        // Pad
        fMarkerTpc[4]->SetNextPoint(rp.X(), rp.Y());
        // Side
        fMarkerTpc[5]->SetNextPoint(rp.X(), rp.Z());
        // Front
        fMarkerTpc[6]->SetNextPoint(rp.Y(), rp.Z());
    }
    // Set marker and draw
    for(auto& [pad, proj] : fMarkerTpc)
    {
        proj->SetMarkerStyle(58);
        proj->SetMarkerSize(2);
        proj->SetMarkerColor(kRed);

        // Draw
        fCanvs[1]->cd(pad);
        proj->Draw("same");
    }
}

void ActRoot::HistogramPainter::SetPalette(const std::string& name, bool reverse)
{
    // Set path to color palettes
    TString path {gSystem->Getenv("ACTROOT")};
    path += "/Core/Data/ColorPalettes/";
    // Get Palette
    path += name;
    path += ".txt";
    if(gSystem->AccessPathName(path))
    {
        std::cout << BOLDRED << "Palette named " << name << " does not exist in ActRoot database" << RESET << '\n';
        return;
    }
    // Set!
    gStyle->SetPalette(path);
    gStyle->SetNumberContours(256);
    // Reverse?
    if(reverse)
        TColor::InvertPalette();
}
