#include "ActEventPainter.h"

#include "ActDetectorManager.h"
#include "Buttons.h"
#include "GuiTypes.h"
#include "ActHistogramPainter.h"
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

#include "ActInputData.h"
#include "ActInputParser.h"
#include "ActTPCData.h"
#include "ActInputIterator.h"

#include <algorithm>
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

void ActRoot::EventPainter::Execute()
{
    if(fDetMan)
        DoVerbosePhysics();
    //Fill
    DoFill();
    //Draw
    DoDraw();
}

void ActRoot::EventPainter::DoDraw()
{
    fHistPainter.Draw();
    //Set info to status bar
    auto [run, entry] = fWrap.GetCurrentStatus();
    fRunButton->SetNumber(run, false);
    fEntryButton->SetNumber(entry, false);
}

void ActRoot::EventPainter::DoReset()
{
    fHistPainter.Reset();
}

void ActRoot::EventPainter::DoFill()
{
    fHistPainter.Fill();
}

void ActRoot::EventPainter::DoPreviousEvent()
{
    DoReset();
    auto ok = fWrap.GoPrevious();
    if(!ok)
        return;
    Execute();
}

void ActRoot::EventPainter::DoNextEvent()
{
    DoReset();
    auto ok = fWrap.GoNext();
    if(!ok)
        return;
    Execute();
}

void ActRoot::EventPainter::DoGoTo()
{
    DoReset();
    auto run {fRunButton->GetIntNumber()};
    auto entry {fEntryButton->GetIntNumber()};
    auto ok = fWrap.GoTo(run, entry);
    if(!ok)
        return;
    Execute();
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
    auto* c1 = fHistPainter.SetCanvas(1, "2D pads", 500, 400);
    fFrame1->SetEditable(false);
}

void ActRoot::EventPainter::InitTab2()
{
    //Add TCanvas
    fFrame2->SetEditable(true);
    auto* c2 = fHistPainter.SetCanvas(2, "Pads and Silicons", 500, 400);
    fFrame2->SetEditable(false);
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

void ActRoot::EventPainter::SetPainterAndData(const std::string& detfile, InputData* input)
{
    //Init HistogramPainter
    fHistPainter.ReadDetFile(detfile);   
    //Init InputWrapper
    fWrap = InputWrapper(input);
    fHistPainter.SetInputWrapper(&fWrap);
}

void ActRoot::EventPainter::SetDetMan(DetectorManager *detman)
{
    fDetMan = detman;
}

void ActRoot::EventPainter::DoVerbosePhysics()
{
    //Set event data!
    //1-> Actar
    fDetMan->SetEventData(DetectorType::EActar, fWrap.GetCurrentTPCData());
    //2-> Sil
    fDetMan->SetEventData(DetectorType::ESilicons, fWrap.GetCurrentSilData());
    //3-> Modular
    fDetMan->SetEventData(DetectorType::EModular, fWrap.GetCurrentModularData());
    //Do not store data; but toy pointer needed
    fDetMan->InitializePhysicsOutput(nullptr);
}
