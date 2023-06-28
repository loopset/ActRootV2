#include "EventPainter.h"

#include "Buttons.h"
#include "GuiTypes.h"
#include "InputData.h"
#include "InputParser.h"
#include "TCanvas.h"
#include "TGFrame.h"
#include "TGLayout.h"
#include "TGStatusBar.h"
#include "TGWindow.h"
#include "TH2.h"
#include "TPCData.h"
#include "TRootEmbeddedCanvas.h"
#include "TApplication.h"
#include "TGText.h"
#include "TGButton.h"
#include "TString.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

ActRoot::InputWrapper::InputWrapper(ActRoot::InputData* input)
    : fInput(input), fIt(ActRoot::InputIterator(input)), fData(new ActRoot::TPCData)
{
}

void ActRoot::InputWrapper::GoNext()
{
    auto [runBef, _] = fIt.GetCurrentRunEntry();
    fIt.Next();
    auto [run, entry] = fIt.GetCurrentRunEntry();
    if(runBef != run)
    {
        fInput->GetTree(run)->SetBranchAddress("data", &fData);
    }
    fInput->GetEntry(run, entry);
}

void ActRoot::InputWrapper::GoPrevious()
{
    auto [runBef, _] = fIt.GetCurrentRunEntry();
    fIt.Previous();
    auto [run, entry] = fIt.GetCurrentRunEntry();
    if(runBef != run)
    {
        fInput->GetTree(run)->SetBranchAddress("data", &fData);
    }
    fInput->GetEntry(run, entry);
}

ActRoot::EventPainter::~EventPainter()
{
    Cleanup();
    delete fEmCanv;
}

void ActRoot::EventPainter::CloseWindow()
{
    DoExit();
}

void ActRoot::EventPainter::DoDraw()
{
    for(auto& [idx, h] : fHist2d)
    {
        fCanv->cd(idx);
        h->Draw("colz");
    }
    fCanv->Update();
    //Set info to status bar
    auto [run, entry] = fWrap.GetCurrentStatus();
    auto toStatusBar {TString::Format("run = %d entry = %d", run, entry)};
    fStatusThis->SetText(toStatusBar.Data(), 1);
}

void ActRoot::EventPainter::ResetHistograms()
{
    //2D
    for(auto& [key, h] : fHist2d)
    {
        h->Reset();
        h->GetXaxis()->UnZoom();
        h->GetYaxis()->UnZoom();
        fCanv->cd(key)->Modified();
    }
}

void ActRoot::EventPainter::ResetCanvas()
{
    //fCanv->Clear("D");
    fCanv->Update();
}

void ActRoot::EventPainter::DoReset()
{
    ResetHistograms();
    ResetCanvas();
}

void ActRoot::EventPainter::DoFill()
{
    for(auto& voxel : fWrap.GetCurrentData()->fVoxels)
    {
        //Pad
        fHist2d[1]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Y(), voxel.GetCharge());
        //Side
        fHist2d[2]->Fill(voxel.GetPosition().X(), voxel.GetPosition().Z(), voxel.GetCharge());
        //Front
        fHist2d[3]->Fill(voxel.GetPosition().Y(), voxel.GetPosition().Z(), voxel.GetCharge());
    }
}

void ActRoot::EventPainter::DoPreviousEvent()
{
    DoReset();
    fWrap.GoPrevious();
    DoFill();
    DoDraw();
}

void ActRoot::EventPainter::DoNextEvent()
{
    DoReset();
    fWrap.GoNext();
    DoFill();
    DoDraw();
}

void ActRoot::EventPainter::DoExit()
{
    std::cout<<"Exiting ActRoot::EventPainter..."<<'\n';
    gApplication->Terminate();
}


void ActRoot::EventPainter::ReadConfiguration(const std::string &file)
{
    ActRoot::InputParser parser {file};
    auto config {parser.GetBlock("Actar")};
    ftpc = TPCParameters(config->GetString("Type"));
    if(config->CheckTokenExists("RebinZ", true))
        ftpc.SetREBINZ(config->GetInt("RebinZ"));
}

void ActRoot::EventPainter::InitCanvas()
{
    fCanv = new TCanvas("cPainter", 10, 10, fEmCanv->GetCanvasWindowId());
    fCanv->Divide(3, 2);
}

void ActRoot::EventPainter::Init2DHistograms()
{
    //Pad
    fHist2d[1] = std::make_shared<TH2F>("hPad", "Pad;X [pad];Y [pad]",
                                        ftpc.GetNPADSX(), 0, ftpc.GetNPADSX(),
                                        ftpc.GetNPADSY(), 0, ftpc.GetNPADSY());
    //Side
    fHist2d[2] = std::make_shared<TH2F>("hSide", "Side;X [pad];Z [tb]",
                                        ftpc.GetNPADSX(), 0, ftpc.GetNPADSX(),
                                        ftpc.GetNPADSZ() / ftpc.GetREBINZ(), 0, ftpc.GetNPADSZ());
    //Front
    fHist2d[3] = std::make_shared<TH2F>("hFront", "Front;Y [pad];Z [tb]",
                                        ftpc.GetNPADSY(), 0, ftpc.GetNPADSY(),
                                        ftpc.GetNPADSZ() / ftpc.GetREBINZ(), 0 , ftpc.GetNPADSZ());
    for(auto& [_, h] : fHist2d)
        h->SetStats(false);
}

void ActRoot::EventPainter::CanvasToStatusBar(int event, int px, int py, TObject *obj)
{
    if(!obj)
        return;
    auto text1 {obj->GetName()};
    fStatusCanv->SetText(text1, 0);
    auto text3 {obj->GetObjectInfo(px, py)};
    fStatusCanv->SetText(text3, 1);
}

ActRoot::EventPainter::EventPainter(const TGWindow* window, unsigned int width, unsigned int height)
    : TGMainFrame(window, width, height)
{
    //Buttons bar
    fButtonsFrame = new TGHorizontalFrame(this, 200, 40);
    //1->Exit button
    TGTextButton* exit = new TGTextButton(fButtonsFrame, "&Exit ");
    exit->Connect("Pressed()", "ActRoot::EventPainter", this, "DoExit()");
    fButtonsFrame->AddFrame(exit, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    //2->Draw
    // TGTextButton* draw = new TGTextButton(fButtonsFrame, "&Draw ");
    // draw->Connect("Pressed()", "ActRoot::EventPainter", this, "DoDraw()");
    // fButtonsFrame->AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    //3->Reset
    TGTextButton* reset = new TGTextButton(fButtonsFrame, "&Reset ");
    reset->Connect("Pressed()", "ActRoot::EventPainter", this, "DoReset()");
    fButtonsFrame->AddFrame(reset, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    //4->Next
    TGTextButton* next = new TGTextButton(fButtonsFrame, "&Next ");
    next->Connect("Pressed()", "ActRoot::EventPainter", this, "DoNextEvent()");
    fButtonsFrame->AddFrame(next, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    //5->Previous
    TGTextButton* previous = new TGTextButton(fButtonsFrame, "&Previous ");
    previous->Connect("Pressed()", "ActRoot::EventPainter", this, "DoPreviousEvent()");
    fButtonsFrame->AddFrame(previous, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    AddFrame(fButtonsFrame, new TGLayoutHints(kLHintsCenterX, 2, 2, 2, 2));
    //Embebbed canvas
    fEmCanv = new TRootEmbeddedCanvas(0, this, 500, 400);
    InitCanvas();
    fEmCanv->AdoptCanvas(fCanv);
    //Connect status bar
    fCanv->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)", "ActRoot::EventPainter",
                   this, "CanvasToStatusBar(int,int,int,TObject*)");
    AddFrame(fEmCanv, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 0, 0, 1, 1));
    //Status bars
    InitStatusBars();
    fStatusThis->SetText("ActRootv2", 0);
    
    //Other configs
    SetWindowName("ActRoot EventPainter");
    MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();
}

void ActRoot::EventPainter::SetDetAndData(const std::string& detector, InputData* input)
{
    //Init detector and histogram config
    ReadConfiguration(detector);
    Init2DHistograms();
    //Init InputWrapper
    fWrap = InputWrapper(input);
}


void ActRoot::EventPainter::InitStatusBars()
{
    //Status bar
    TGHorizontalFrame* frameStatus = new TGHorizontalFrame(this, 50, 10);
    //1->For painter info
    int pars2[] = {30, 70};
    fStatusThis = new TGStatusBar(frameStatus, 50, 10, kHorizontalFrame);
    fStatusThis->SetParts(pars2, 2);
    fStatusThis->Draw3DCorner(false);
    frameStatus->AddFrame(fStatusThis, new TGLayoutHints(kLHintsExpandX, 5, 0, 5, 0));
    //2-> For canvas info
    int pars1 [] = {30, 70};
    fStatusCanv = new TGStatusBar(frameStatus, 50, 10, kVerticalFrame);
    fStatusCanv->SetParts(pars1, 2);
    fStatusCanv->Draw3DCorner(false);
    frameStatus->AddFrame(fStatusCanv, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 5, 0, 5, 0));
    //Add to global window
    AddFrame(frameStatus, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 2));
}
