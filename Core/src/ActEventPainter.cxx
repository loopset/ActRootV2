#include "ActEventPainter.h"

#include "ActColors.h"
#include "ActDetectorManager.h"
#include "ActHistogramPainter.h"
#include "ActInputData.h"
#include "ActInputIterator.h"
#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActMergerDetector.h"
#include "ActTPCData.h"
#include "ActTPCDetector.h"
#include "ActTypes.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TCollection.h"
#include "TGButton.h"
#include "TGFrame.h"
#include "TGLayout.h"
#include "TGNumberEntry.h"
#include "TGStatusBar.h"
#include "TGTab.h"
#include "TGText.h"
#include "TGToolBar.h"
#include "TGWindow.h"
#include "TH2.h"
#include "TRootCanvas.h"
#include "TString.h"

#include "Buttons.h"
#include "GuiTypes.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

void ActRoot::EventPainter::DoExit()
{
    std::cout << "Exiting ActRoot::EventPainter..." << '\n';
    gApplication->Terminate();
}

void ActRoot::EventPainter::CloseWindow()
{
    DoExit();
}

void ActRoot::EventPainter::Execute()
{
    if(fDetMan && fIsVerbose)
        DoVerbosePhysics();
    // Fill
    DoFill();
    // Draw
    DoDraw();
}

void ActRoot::EventPainter::DoDraw()
{
    fHistPainter.Draw();
    // Set info to status bar
    auto [run, entry] = fWrap.GetCurrentStatus();
    fRunButton->SetNumber(run, false);
    fEntryButton->SetNumber(entry, false);
}

void ActRoot::EventPainter::DoReset()
{
    // Reset histograms
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

void ActRoot::EventPainter::DoReconfAndCluster()
{
    DoReset();
    if(fDetMan)
    {
        fDetMan->Reconfigure();
        fWrap.ReGet();
        auto tpc {fDetMan->GetDetectorAs<TPCDetector>()};
        tpc->SetEventData(fWrap.GetTPCData());
        tpc->Recluster();
        Execute();
    }
}

void ActRoot::EventPainter::DoReconfAndExecute()
{
    DoReset();
    // Reread confs of detectors
    if(fDetMan)
    {
        fDetMan->Reconfigure();
        fWrap.ReGet();
        Execute();
    }
}

void ActRoot::EventPainter::InitButtons()
{
    // Buttons bar
    fButtonsFrame = new TGHorizontalFrame(this, 200, 40);
    auto* lb = new TGLayoutHints(kLHintsCenterX, 5, 5, 4, 3);
    // 1->Exit button
    TGTextButton* exit = new TGTextButton(fButtonsFrame, "&Exit ");
    exit->Connect("Pressed()", "ActRoot::EventPainter", this, "DoExit()");
    fButtonsFrame->AddFrame(exit, lb);
    // 2-> recluster
    TGTextButton* reclu = new TGTextButton(fButtonsFrame, "&ReCluster ");
    reclu->Connect("Pressed()", "ActRoot::EventPainter", this, "DoReconfAndCluster()");
    fButtonsFrame->AddFrame(reclu, lb);
    // 3->Reset
    TGTextButton* reset = new TGTextButton(fButtonsFrame, "&ReMerge ");
    reset->Connect("Pressed()", "ActRoot::EventPainter", this, "DoReconfAndExecute()");
    fButtonsFrame->AddFrame(reset, lb);
    // 4->Previous
    TGTextButton* previous = new TGTextButton(fButtonsFrame, "&Previous ");
    previous->Connect("Pressed()", "ActRoot::EventPainter", this, "DoPreviousEvent()");
    fButtonsFrame->AddFrame(previous, lb);
    // 5->Next
    TGTextButton* next = new TGTextButton(fButtonsFrame, "&Next ");
    next->Connect("Pressed()", "ActRoot::EventPainter", this, "DoNextEvent()");
    fButtonsFrame->AddFrame(next, lb);
    // Add frame
    AddFrame(fButtonsFrame, new TGLayoutHints(kLHintsCenterX, 5, 5, 2, 1));
    // Map
    fButtonsFrame->MapSubwindows();
    fButtonsFrame->MapWindow();
}

void ActRoot::EventPainter::InitEntryButtons()
{
    auto* lb = new TGLayoutHints(kLHintsCenterX, 5, 5, 3, 4);
    // Run
    fRunButton = new TGNumberEntry(fButtonsFrame, 0, 9, 999, TGNumberFormat::kNESInteger,
                                   TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMinMax, 0, 99999);
    // fRunButton->Connect("ValueSet(Long_t)", "ActRoot::EventPainter", this, "DoSetRun()");
    fButtonsFrame->AddFrame(fRunButton, lb);
    // Entry
    fEntryButton = new TGNumberEntry(fButtonsFrame, 0, 9, 999, TGNumberFormat::kNESInteger,
                                     TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMinMax, 0, 99999);
    fButtonsFrame->AddFrame(fEntryButton, lb);
    // Go to
    TGTextButton* gooto = new TGTextButton(fButtonsFrame, "&GoTo ");
    gooto->Connect("Pressed()", "ActRoot::EventPainter", this, "DoGoTo()");
    fButtonsFrame->AddFrame(gooto, lb);
    fButtonsFrame->MapSubwindows();
    fButtonsFrame->MapWindow();
}

void ActRoot::EventPainter::InitTabs()
{
    fTabManager = new TGTab(this, 500, 400);
    AddFrame(fTabManager, new TGLayoutHints(kLHintsTop | kLHintsExpandX | kLHintsExpandY, 2, 2, 0, 0));
    // Tab 1
    fTab1 = fTabManager->AddTab("TPC & clusters");
    fFrame1 = new TGCompositeFrame(fTab1, 500, 400, kHorizontalFrame);
    fTab1->AddFrame(fFrame1, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 2, 2, 0, 0));
    // Tab 2
    fTab2 = fTabManager->AddTab("Sil & others");
    fFrame2 = new TGCompositeFrame(fTab2, 500, 400, kHorizontalFrame);
    fTab2->AddFrame(fFrame2, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 2, 2, 0, 0));
    // Map all
    fTabManager->MapSubwindows();
    fTabManager->MapWindow();
}

void ActRoot::EventPainter::InitTab1()
{
    // Add TCanvas
    fFrame1->SetEditable(true);
    auto* c1 = fHistPainter.SetCanvas(1, "2D pads", 500, 400);
    fFrame1->SetEditable(false);
}

void ActRoot::EventPainter::InitTab2()
{
    // Add TCanvas
    fFrame2->SetEditable(true);
    auto* c2 = fHistPainter.SetCanvas(2, "Pads and Silicons", 500, 400);
    fFrame2->SetEditable(false);
}

ActRoot::EventPainter::EventPainter(const TGWindow* window, unsigned int width, unsigned int height)
    : TGMainFrame(window, width, height)
{
    // Init buttons
    InitButtons();
    InitEntryButtons();
    // Create Tab structure
    InitTabs();
    // Init Tab1
    InitTab1();
    // Init Tab2
    InitTab2();

    // Other configs
    // SetCleanup(kDeepCleanup);
    SetWindowName("ActRoot EventPainter");
    // MapSubwindows();
    Layout();
    Resize(GetDefaultSize());
    MapWindow();
}

ActRoot::EventPainter::~EventPainter()
{
    Cleanup();
}

void ActRoot::EventPainter::SetDetectorAndData(const std::string& detfile, const std::string& infile, bool outputAlso)
{
    // Init input
    auto* input {new InputData(infile, outputAlso)};
    fWrap = std::move(InputWrapper {input});
    // Init detector
    fDetMan = new DetectorManager(ModeType::EMerge);
    // Init histogram painter
    fHistPainter.SendParameters(fDetMan);
    fHistPainter.SendInputWrapper(&fWrap);
    fHistPainter.Init();
}

void ActRoot::EventPainter::DoVerbosePhysics()
{
    std::cout << BOLDRED << "EventPainter::DoVerbosePhysics(): not implemented yet" << RESET << '\n';
    // // Send data to Merger
    // auto merger {fDetMan->GetDetectorAs<MergerDetector>()};
    // // 1-> Actar
    // merger->SetEventData(fWrap.GetTPCData());
    // // 2-> Sil
    // merger->SetEventData(fWrap.GetSilData());
    // // 3-> Modular
    // merger->SetEventData(fWrap.GetModularData());
    // // Do not store data; but toy pointer needed
    // fDetMan->InitOutput(nullptr);
    //
    // // Enable cloning
    // merger->EnableTPCDataClone();
    // // Build event
    // fDetMan->BuildEventMerger(-1, -1);
    //
    // // Point to clone
    // fWrap.SetTPCDataClone2(merger->GetTPCDataClone());
    //
    // // Print results
    // fWrap.GetTPCData()->Print();
    // merger->GetEventMerger()->Print();
    // fWrap.SetMergerData(dynamic_cast<MergerData*>(merger->GetEventMerger()));
    //
    // // Set data
    // auto tpcPhys {fDetMan->GetDetector(DetectorType::EActar)->GetEventPhysics()};
    // tpcPhys->Print();
    // fWrap.SetTPCPhysics(dynamic_cast<TPCPhysics*>(tpcPhys));
    //
    // // Set merger data
    // fDetMan->SetEventData(DetectorType::EMerger, tpcPhys);
    // fDetMan->SetEventData(DetectorType::EMerger, fWrap.GetSilData());
    // fDetMan->SetEventData(DetectorType::EMerger, fWrap.GetModularData());
    // fDetMan->InitializeMergerOutput(nullptr);
    // // And run also merger
    // fDetMan->BuildEventMerger(-1, -1);
    // auto md {fDetMan->GetMerger()->GetMergerData()};
    // md->Print();
    // fWrap.SetMergerData(dynamic_cast<MergerData*>(md));
}
