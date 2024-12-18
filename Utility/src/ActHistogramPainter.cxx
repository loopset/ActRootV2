#include "ActHistogramPainter.h"

#include "ActColors.h"
#include "ActDetectorManager.h"
#include "ActInputIterator.h"
#include "ActInputParser.h"
#include "ActMergerDetector.h"
#include "ActMultiRegion.h"
#include "ActOptions.h"
#include "ActSilDetector.h"
#include "ActSilMatrix.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"

#include "Rtypes.h"

#include "TCanvas.h"
#include "TColor.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TPaveText.h"
#include "TPolyMarker.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

ActRoot::HistogramPainter::HistogramPainter()
{
    ReadConfiguration();
}

void ActRoot::HistogramPainter::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "painter.conf";
    // Init parser
    ActRoot::InputParser parser {conf};
    auto hb {parser.GetBlock("HistogramPainter")};
    if(hb->CheckTokenExists("Palette", true) && hb->CheckTokenExists("Reverse", true))
        SetPalette(hb->GetString("Palette"), hb->GetBool("Reverse"));
    if(hb->CheckTokenExists("Palette", true) && !hb->CheckTokenExists("Reverse", true))
        SetPalette(hb->GetString("Palette"));
    if(hb->CheckTokenExists("ShowStats", true))
        fShowHistStats = hb->GetBool("ShowStats");
}

void ActRoot::HistogramPainter::SendParameters(ActRoot::DetectorManager* detman)
{
    fTPC = detman->GetDetectorAs<TPCDetector>()->GetParameters();
    fSil = detman->GetDetectorAs<SilDetector>()->GetParameters();
}

void ActRoot::HistogramPainter::Init()
{
    // Check TCanvas
    if(fCanvas->size() < 2)
        throw std::runtime_error("HistogramPainter::Init(): fCanvas has not minimum size of 2 for having 2 tabs");
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // TPC
    fCanvas->at(0)->Divide(3, 2);
    // Pad
    fHist2D[0][1] = std::make_shared<TH2F>("hPad", "Pad;X [pad];Y [pad]", fTPC->GetNPADSX(), 0, fTPC->GetNPADSX(),
                                           fTPC->GetNPADSY(), 0, fTPC->GetNPADSY());
    // Side
    fHist2D[0][2] = std::make_shared<TH2F>("hSide", "Side;X [pad];Z [tb]", fTPC->GetNPADSX(), 0, fTPC->GetNPADSX(),
                                           fTPC->GetNPADSZ(), 0, fTPC->GetNPADSZUNREBIN());
    // Front
    fHist2D[0][3] = std::make_shared<TH2F>("hFront", "Front;Y [pad];Z [tb]", fTPC->GetNPADSY(), 0, fTPC->GetNPADSY(),
                                           fTPC->GetNPADSZ(), 0, fTPC->GetNPADSZUNREBIN());
    ////////////////////////
    // Clusters!
    // Pad
    fHist2D[0][4] = std::make_shared<TH2F>("hPadC", "Clusters in pad;X [pad];Y [pad]", fTPC->GetNPADSX(), 0,
                                           fTPC->GetNPADSX(), fTPC->GetNPADSY(), 0, fTPC->GetNPADSY());
    // Side
    fHist2D[0][5] = std::make_shared<TH2F>("hSideC", "Clusters in side;X [pad];Z [tb]", fTPC->GetNPADSX(), 0,
                                           fTPC->GetNPADSX(), fTPC->GetNPADSZ(), 0, fTPC->GetNPADSZUNREBIN());
    // Front
    fHist2D[0][6] = std::make_shared<TH2F>("hFrontC", "Clusters in front;Y [pad];Z [tb]", fTPC->GetNPADSY(), 0,
                                           fTPC->GetNPADSY(), fTPC->GetNPADSZ(), 0, fTPC->GetNPADSZUNREBIN());
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Silicons
    fCanvas->at(1)->Divide(3, 2);

    // Set stats options
    for(auto& [c, map] : fHist2D)
        for(auto& [p, h] : map)
            h->SetStats(fShowHistStats);
    // Set stats options
    for(auto& [c, map] : fHist1D)
        for(auto& [p, h] : map)
            h->SetStats(fShowHistStats);
}

void ActRoot::HistogramPainter::FillVoxelsHisto()
{
    if(!fWrap->GetTPCData())
        return;
    // Get CLONE of TPC data
    auto clone {fWrap->GetTPCDataClone()};
    // Fill noise
    for(const auto& voxel : clone->fRaw)
    {
        const auto& pos {voxel.GetPosition()};
        // Pad
        fHist2D[0][1]->Fill(pos.X(), pos.Y(), voxel.GetCharge());
        // Side
        fHist2D[0][2]->Fill(pos.X(), pos.Z() * fTPC->GetREBINZ(), voxel.GetCharge());
        // Front
        fHist2D[0][3]->Fill(pos.Y(), pos.Z() * fTPC->GetREBINZ(), voxel.GetCharge());
    }
    // Fill with voxels of clusters
    for(const auto& cluster : clone->fClusters)
    {
        for(auto& voxel : cluster.GetVoxels())
        {
            const auto& pos {voxel.GetPosition()};
            // Pad
            fHist2D[0][1]->Fill(pos.X(), pos.Y(), voxel.GetCharge());
            // Side
            fHist2D[0][2]->Fill(pos.X(), pos.Z() * fTPC->GetREBINZ(), voxel.GetCharge());
            // Front
            fHist2D[0][3]->Fill(pos.Y(), pos.Z() * fTPC->GetREBINZ(), voxel.GetCharge());
        }
    }
}
void ActRoot::HistogramPainter::FillClusterHistos()
{
    if(!fWrap->GetTPCData())
        return;
    auto* data {fWrap->GetTPCData()};
    for(const auto& cluster : data->fClusters)
    {
        for(const auto& voxel : cluster.GetVoxels())
        {
            const auto& pos {voxel.GetPosition()};
            // Pad
            AttachBinToCluster(fHist2D[0][4], pos.X(), pos.Y(), cluster.GetClusterID());
            // Side
            AttachBinToCluster(fHist2D[0][5], pos.X(), pos.Z() * fTPC->GetREBINZ(), cluster.GetClusterID());
            // Front
            AttachBinToCluster(fHist2D[0][6], pos.Y(), pos.Z() * fTPC->GetREBINZ(), cluster.GetClusterID());
        }
    }
    // Set basic parameters to a better visualization of TPaletteAxis
    int nclusters {static_cast<int>(data->fClusters.size())};
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
        fHist2D[0][pad]->SetMinimum(min);
        fHist2D[0][pad]->SetMaximum(max);
        fHist2D[0][pad]->GetZaxis()->SetNdivisions(ndiv); // = nlabels
    }
}

void ActRoot::HistogramPainter::FillSilMatrices()
{
    if(!fWrap->GetSilData())
        return;
    auto data {fWrap->GetSilData()};
    // For each matrix
    for(auto& [name, matrix] : fSilMatrices)
    {
        // Build energies
        std::map<int, double> energies;
        if(data->fSiE.count(name))
        {
            for(int i = 0; i < data->fSiE[name].size(); i++)
                energies[data->fSiN[name][i]] = data->fSiE[name][i];
        }
        // For each graph
        for(auto& [idx, f] : matrix->GetGraphs())
        {
            f->GetListOfFunctions()->Clear();
            // Add new text
            auto [xy, z] {matrix->GetCentre(idx)};
            auto w {matrix->GetWidth(idx) * 0.3};
            auto h {matrix->GetHeight(idx) * 0.4};
            auto* text {new TPaveText {xy - w / 2, z - h / 2, xy + w / 2, z + h / 2}};
            text->AddText(TString::Format("%d", idx));
            if(energies.count(idx))
            {
                auto* entry = text->AddText(TString::Format("%.2f MeV", energies.at(idx)));
                text->SetTextSize(0.05);
                entry->SetTextColor(46);
            }
            text->SetBorderSize(0);
            text->SetFillStyle(0);
            f->GetListOfFunctions()->Add(text);
        }
    }
}

void ActRoot::HistogramPainter::Fill()
{
    // TPC histograms
    FillVoxelsHisto();
    FillClusterHistos();
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Sil matrices
    FillSilMatrices();
}
//
void ActRoot::HistogramPainter::Draw()
{
    for(auto& [c, map] : fHist2D)
    {
        for(auto& [p, h] : map)
        {
            fCanvas->at(c)->cd(p);
            if(c == 0)
                h->Draw("colz");
            else if(c == 1)
                h->Draw("col text");
            else
                h->Draw("scat");
        }
    }
    for(auto& [c, map] : fHist1D)
    {
        for(auto& [p, h] : map)
        {
            fCanvas->at(c)->cd(p);
            h->Draw("hist");
        }
    }
    DrawRegions();
    DrawPolyLines();
    DrawPolyMarkers();
    DrawSilMatrices();
    DrawProjections();

    // Update all after drawing poly things
    for(auto& c : *fCanvas)
        c->Update();
}

void ActRoot::HistogramPainter::Reset()
{
    fLines.clear();
    fMarkers.clear();
    for(auto& [c, map] : fHist2D)
    {
        for(auto& [p, h] : map)
        {
            h->Reset();
            h->GetXaxis()->UnZoom();
            h->GetYaxis()->UnZoom();
            fCanvas->at(c)->cd(p)->Modified();
        }
    }
    for(auto& [c, map] : fHist1D)
    {
        for(auto& [p, h] : map)
        {
            h->Reset();
            h->GetXaxis()->UnZoom();
            h->GetYaxis()->UnZoom();
            fCanvas->at(c)->cd(p)->Modified();
        }
    }

    // Update all canvas
    for(auto& c : *fCanvas)
        c->Update();
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

void ActRoot::HistogramPainter::DrawPolyLines()
{
    if(!fWrap->GetTPCData())
        return;

    auto data {fWrap->GetTPCDataClone2()};

    for(const auto& cluster : data->fClusters)
    {
        int xmin {0};
        int xmax {fTPC->GetNPADSX()};
        // Pad
        fLines[0][4].push_back(cluster.GetLine().GetPolyLine("xy", xmin, xmax, fTPC->GetNPADSY(),
                                                             fTPC->GetNPADSZUNREBIN(), fTPC->GetREBINZ()));
        // Side
        fLines[0][5].push_back(cluster.GetLine().GetPolyLine("xz", xmin, xmax, fTPC->GetNPADSY(),
                                                             fTPC->GetNPADSZUNREBIN(), fTPC->GetREBINZ()));
        // Front
        fLines[0][6].push_back(cluster.GetLine().GetPolyLine("yz", xmin, xmax, fTPC->GetNPADSY(),
                                                             fTPC->GetNPADSZUNREBIN(), fTPC->GetREBINZ()));
    }
    // Set line parameters
    for(auto& [c, map] : fLines)
    {
        for(auto& [pad, proj] : map)
        {
            for(auto& poly : proj)
            {
                poly->SetLineWidth(2);
                // Draw
                fCanvas->at(c)->cd(pad);
                poly->Draw("same");
            }
        }
    }
}

void ActRoot::HistogramPainter::DrawPolyMarkers()
{
    if(!fWrap->GetTPCData())
        return;

    auto data {fWrap->GetTPCDataClone2()};

    // Init
    for(const auto& rp : data->fRPs)
    {
        // Pad
        fMarkers[0][4].push_back(std::make_shared<TPolyMarker>());
        fMarkers[0][4].back()->SetNextPoint(rp.X(), rp.Y());
        // Side
        fMarkers[0][5].push_back(std::make_shared<TPolyMarker>());
        fMarkers[0][5].back()->SetNextPoint(rp.X(), rp.Z() * fTPC->GetREBINZ());
        // Front
        fMarkers[0][6].push_back(std::make_shared<TPolyMarker>());
        fMarkers[0][6].back()->SetNextPoint(rp.Y(), rp.Z() * fTPC->GetREBINZ());
    }
    // Set marker style and draw
    for(auto& [c, map] : fMarkers)
    {
        for(auto& [pad, markers] : map)
        {
            fCanvas->at(c)->cd(pad);
            for(auto& marker : markers)
            {
                marker->SetMarkerStyle(58);
                marker->SetMarkerSize(2);
                marker->SetMarkerColor(kRed);

                marker->Draw("same");
            }
        }
    }
}

void ActRoot::HistogramPainter::DrawSilMatrices()
{
    // These are always drawn in the second canvas
    for(const auto& [name, matrix] : fSilMatrices)
    {
        auto idx {matrix->GetPadIdx()};
        fCanvas->at(1)->cd(idx);
        matrix->DrawForPainter();
    }
}

void ActRoot::HistogramPainter::DrawProjections()
{
    auto merger {fDetMan->GetDetectorAs<MergerDetector>()};
    if(merger)
    {
        auto data {merger->GetOutputData()};
        // Cd to correct pad
        fCanvas->at(1)->cd(2);
        data->fQProf.Draw("hist");
        fCanvas->at(1)->cd(3);
        data->fQprojX.Draw("hist");
        // Draw also pad plane in 2nd tab
        fCanvas->at(1)->cd(4);
        fHist2D[0][4]->Draw("colz");
    }
}

void ActRoot::HistogramPainter::InitRegionGraphs()
{
    auto filter {fDetMan->GetDetectorAs<TPCDetector>()->GetFilter()};
    if(auto casted {std::dynamic_pointer_cast<ActAlgorithm::MultiRegion>(filter)}; casted)
    {
        // 1-> Get regions
        const auto& regions {casted->GetRegions()};
        for(const auto& [name, region] : regions)
        {
            // XY
            auto gxy {std::make_shared<TGraph>()};
            region.FillGraph(gxy.get(), "xy", 0, fTPC->GetNPADSZUNREBIN());
            fGraphs[0][4].push_back(gxy);
            // XZ
            auto gxz {std::make_shared<TGraph>()};
            region.FillGraph(gxz.get(), "xz", 0, fTPC->GetNPADSZUNREBIN());
            fGraphs[0][5].push_back(gxz);
            // YZ
            auto gyz {std::make_shared<TGraph>()};
            region.FillGraph(gyz.get(), "yz", 0, fTPC->GetNPADSZUNREBIN());
            fGraphs[0][6].push_back(gyz);
            // Set style
            for(auto& g : {gxy, gxz, gyz})
            {
                g->SetFillStyle(3003);
                g->SetFillColor(kGray + 1);
            }
        }
    }
}

void ActRoot::HistogramPainter::InitSiliconMatrices()
{
    auto specs {fDetMan->GetDetectorAs<MergerDetector>()->GetSilSpecs()};
    for(const auto& [name, layer] : specs->GetLayers())
    {
        fSilMatrices[name] = std::shared_ptr<ActPhysics::SilMatrix>(layer.GetSilMatrix()->Clone());
        fSilMatrices[name]->CreateMultiGraphForPainter();
    }
}

void ActRoot::HistogramPainter::DrawRegions()
{

    for(auto& [c, map] : fGraphs)
    {
        for(auto& [pad, vec] : map)
        {
            fCanvas->at(c)->cd(pad);
            for(auto& g : vec)
                g->Draw("f same");
        }
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
