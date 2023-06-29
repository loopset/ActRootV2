#include "EventPainter.h"

#include "Buttons.h"
#include "GuiTypes.h"
#include "TCanvas.h"
#include "TCollection.h"
#include "TGFrame.h"
#include "TGLayout.h"
#include "TGStatusBar.h"
#include "TGTab.h"
#include "TGWindow.h"
#include "TH2.h"
#include "TApplication.h"
#include "TGText.h"
#include "TGButton.h"
#include "TString.h"
#include "TRootCanvas.h"
#include "TGToolBar.h"
#include "TGNumberEntry.h"

#include "InputData.h"
#include "InputParser.h"
#include "TPCData.h"
#include "InputIterator.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

void ActRoot::EventPainter::DoExit()
{
    std::cout<<"Exiting ActRoot::EventPainter..."<<'\n';
    gApplication->Terminate();
}


void ActRoot::EventPainter::CloseWindow()
{
    DoExit();
}

void ActRoot::EventPainter::DoDraw()
{
    for(auto& [idx, h] : fHist2d)
    {
        fCanv1->cd(idx);
        h->Draw("colz");
    }
    fCanv1->Update();
    //Set info to status bar
    auto [run, entry] = fWrap.GetCurrentStatus();
    auto toStatusBar {TString::Format("run = %d entry = %d", run, entry)};
    fStatusThis->SetText(toStatusBar.Data(), 1);
    fRunButton->SetNumber(run, false);
    fEntryButton->SetNumber(entry, false);
}

void ActRoot::EventPainter::ResetHistograms()
{
    //2D
    for(auto& [key, h] : fHist2d)
    {
        h->Reset();
        h->GetXaxis()->UnZoom();
        h->GetYaxis()->UnZoom();
        fCanv1->cd(key)->Modified();
    }
}

void ActRoot::EventPainter::ResetCanvas()
{
    //fCanv->Clear("D");
    fCanv1->Update();
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
    auto ok = fWrap.GoPrevious();
    if(!ok)
        return;
    DoFill();
    DoDraw();
}

void ActRoot::EventPainter::DoNextEvent()
{
    DoReset();
    auto ok = fWrap.GoNext();
    if(!ok)
        return;
    DoFill();
    DoDraw();
}

void ActRoot::EventPainter::DoGoTo()
{
    DoReset();
    auto run {fRunButton->GetIntNumber()};
    auto entry {fEntryButton->GetIntNumber()};
    auto ok = fWrap.GoTo(run, entry);
    if(!ok)
        return;
    DoFill();
    DoDraw();
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

void ActRoot::EventPainter::InitButtons()
{
    //Buttons bar
    fButtonsFrame = new TGHorizontalFrame(this, 200, 40);
    auto* lb = new TGLayoutHints(kLHintsCenterX, 5, 5, 4, 3);
    //1->Exit button
    TGTextButton* exit = new TGTextButton(fButtonsFrame, "&Exit ");
    exit->Connect("Pressed()", "ActRoot::EventPainter", this, "DoExit()");
    fButtonsFrame->AddFrame(exit, lb);
    //2->Draw
    // TGTextButton* draw = new TGTextButton(fButtonsFrame, "&Draw ");
    // draw->Connect("Pressed()", "ActRoot::EventPainter", this, "DoDraw()");
    // fButtonsFrame->AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4));
    //3->Reset
    TGTextButton* reset = new TGTextButton(fButtonsFrame, "&Reset ");
    reset->Connect("Pressed()", "ActRoot::EventPainter", this, "DoReset()");
    fButtonsFrame->AddFrame(reset, lb);
    //4->Previous
    TGTextButton* previous = new TGTextButton(fButtonsFrame, "&Previous ");
    previous->Connect("Pressed()", "ActRoot::EventPainter", this, "DoPreviousEvent()");
    fButtonsFrame->AddFrame(previous, lb);
    //5->Next
    TGTextButton* next = new TGTextButton(fButtonsFrame, "&Next ");
    next->Connect("Pressed()", "ActRoot::EventPainter", this, "DoNextEvent()");
    fButtonsFrame->AddFrame(next, lb);
    //Add frame
    AddFrame(fButtonsFrame, new TGLayoutHints(kLHintsCenterX, 5, 5, 2, 1));
    //Map
    fButtonsFrame->MapSubwindows();
    fButtonsFrame->MapWindow();
}

void ActRoot::EventPainter::InitEntryButtons()
{
    auto* lb = new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4);
    //Run
    fRunButton = new TGNumberEntry(fButtonsFrame, 0, 9,999, TGNumberFormat::kNESInteger,
                                   TGNumberFormat::kNEANonNegative,
                                   TGNumberFormat::kNELLimitMinMax,
                                   0, 99999);
    //fRunButton->Connect("ValueSet(Long_t)", "ActRoot::EventPainter", this, "DoSetRun()");
    fButtonsFrame->AddFrame(fRunButton, lb);
    //Entry
    fEntryButton = new TGNumberEntry(fButtonsFrame, 0, 9, 999, TGNumberFormat::kNESInteger,
                                     TGNumberFormat::kNEANonNegative,
                                     TGNumberFormat::kNELLimitMinMax,
                                     0, 99999);
    fButtonsFrame->AddFrame(fEntryButton, lb);
    //Go to
    TGTextButton* gooto = new TGTextButton(fButtonsFrame, "&GoTo ");
    gooto->Connect("Pressed()", "ActRoot::EventPainter", this, "DoGoTo()");
    fButtonsFrame->AddFrame(gooto, lb);
    fButtonsFrame->MapSubwindows();
    fButtonsFrame->MapWindow();
}

void ActRoot::EventPainter::InitStatusBars()
{
    //Status bar
    fStatusFrame = new TGHorizontalFrame(this, 50, 10);
    //1->For painter info
    int pars2[] = {20, 80};
    fStatusThis = new TGStatusBar(fStatusFrame, 50, 10, kHorizontalFrame);
    fStatusThis->SetParts(pars2, 2);
    fStatusThis->Draw3DCorner(false);
    fStatusFrame->AddFrame(fStatusThis, new TGLayoutHints(kLHintsExpandX, 2, 2, 0, 0));
    //2-> For canvas info
    int pars1 [] = {20, 80};
    fStatusCanv = new TGStatusBar(fStatusFrame, 50, 10, kVerticalFrame);
    fStatusCanv->SetParts(pars1, 2);
    fStatusCanv->Draw3DCorner(false);
    fStatusFrame->AddFrame(fStatusCanv, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 2, 0, 0));
    //Add to global window
    AddFrame(fStatusFrame, new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 2));
    //Default text
    fStatusThis->SetText("ActRootv2", 0);
    //Map
    fStatusFrame->MapSubwindows();
    fStatusFrame->MapWindow();
}

void ActRoot::EventPainter::InitTabs()
{
    fTabManager = new TGTab(this, 500, 400);
    AddFrame(fTabManager, new TGLayoutHints(kLHintsTop | kLHintsExpandX |
                                            kLHintsExpandY, 2, 2, 0, 0));
    //Tab 1
    fTab1 = fTabManager->AddTab("2D histos");
    fFrame1 = new TGCompositeFrame(fTab1, 500, 400, kHorizontalFrame);
    fTab1->AddFrame(fFrame1, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 2, 2, 0, 0));
    //Tab 2
    fTab2 = fTabManager->AddTab("future");
    fFrame2 = new TGCompositeFrame(fTab2, 500, 400, kHorizontalFrame);
    fTab2->AddFrame(fFrame2, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 2, 2, 0, 0));
    //Map all
    fTabManager->MapSubwindows();
    fTabManager->MapWindow();
}

void ActRoot::EventPainter::InitTab1()
{
    //Add TCanvas
    fFrame1->SetEditable(true);
    fCanv1 = new TCanvas("cPainter1", "2D pads", 500, 400);
    fCanv1->ToggleToolBar();
    fCanv1->ToggleEventStatus();
    fCanv1->Divide(3, 2);
    fFrame1->SetEditable(false);
    //Connect status bar
    fCanv1->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)", "ActRoot::EventPainter",
                   this, "CanvasToStatusBar(int,int,int,TObject*)");
}

void ActRoot::EventPainter::InitTab2()
{
    ;
}

ActRoot::EventPainter::EventPainter(const TGWindow* window, unsigned int width, unsigned int height)
    : TGMainFrame(window, width, height)
{
    //Init buttons
    InitButtons();
    InitEntryButtons();
    //Create Tab structure
    InitTabs();
    //Init Tab1
    InitTab1();
    //Init Tab2
    InitTab2();
    //Status bars
    InitStatusBars();

    //Other configs
    //SetCleanup(kDeepCleanup);
    SetWindowName("ActRoot EventPainter");
    //MapSubwindows();
    Layout();
    Resize(GetDefaultSize());
    MapWindow();
}

ActRoot::EventPainter::~EventPainter()
{
    Cleanup();
}

void ActRoot::EventPainter::SetDetAndData(const std::string& detector, InputData* input)
{
    //Init detector and histogram config
    ReadConfiguration(detector);
    Init2DHistograms();
    //Init InputWrapper
    fWrap = InputWrapper(input);
}

void ActRoot::EventPainter::ReadConfiguration(const std::string &file)
{
    ActRoot::InputParser parser {file};
    auto config {parser.GetBlock("Actar")};
    ftpc = TPCParameters(config->GetString("Type"));
    if(config->CheckTokenExists("RebinZ", true))
        ftpc.SetREBINZ(config->GetInt("RebinZ"));
}
