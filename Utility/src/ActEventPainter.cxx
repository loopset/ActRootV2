#include "ActEventPainter.h"

#include "ActDetectorManager.h"
#include "ActHistogramPainter.h"
#include "ActInputIterator.h"
#include "ActMergerDetector.h"
#include "ActOptions.h"
#include "ActTPCDetector.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TGButton.h"
#include "TGFrame.h"
#include "TGLayout.h"
#include "TGNumberEntry.h"
#include "TGTab.h"
#include "TGWindow.h"
#include "TRootCanvas.h"
#include "TString.h"

#include "GuiTypes.h"

#include <vector>

ActRoot::EventPainter::EventPainter(const TGWindow* window, unsigned int width, unsigned int height)
    : TGMainFrame(window, width, height)
{
    // Init buttons
    AddButtons();
    // Create Tab structure
    AddTabs();
    // Init canvas
    AddCanvas();

    // // Other configs
    // SetCleanup(kLocalCleanup);
    SetWindowName("ActRoot EventPainter");
    // MapSubwindows();
    Layout();
    Resize(GetDefaultSize());
    MapWindow();

    // Verbose mode
    fIsVerbose = ActRoot::Options::GetInstance()->GetIsVerbose();
}

ActRoot::EventPainter::~EventPainter()
{
    // Clean buttons
    for(auto& tb : fTextButtons)
    {
        delete tb;
        tb = nullptr;
    }
    delete fButtonsFrame;
    // Clean tabs
    for(auto& c : fCanvas)
    {
        delete c;
        c = nullptr;
    }
    for(auto& vec : {&fFrames})
    {
        for(auto& ptr : *vec)
        {
            delete ptr;
            ptr = nullptr;
        }
    }
    delete fTabManager;
    // delete this;
}

void ActRoot::EventPainter::AddButtons()
{
    // Text buttons
    // 1-> Horizontal frame
    fButtonsFrame = new TGHorizontalFrame {this, 200, 40};
    // 2-> Layout hints
    auto* hints {new TGLayoutHints {kLHintsCenterX, 5, 5, 4, 3}};
    // 3-> Exit button
    fTextButtons.push_back(new TGTextButton {fButtonsFrame, "&Exit"});
    fTextButtons.back()->Connect("Pressed()", "ActRoot::EventPainter", this, "DoExit()");
    fButtonsFrame->AddFrame(fTextButtons.back(), hints);
    // 4-> Reread configuration
    fTextButtons.push_back(new TGTextButton {fButtonsFrame, "&Redo"});
    fTextButtons.back()->Connect("Pressed()", "ActRoot::EventPainter", this, "DoReconfAndExecute()");
    fButtonsFrame->AddFrame(fTextButtons.back(), hints);
    // 5-> Previous
    fTextButtons.push_back(new TGTextButton {fButtonsFrame, "&Previous"});
    fTextButtons.back()->Connect("Pressed()", "ActRoot::EventPainter", this, "DoPreviousEvent()");
    fButtonsFrame->AddFrame(fTextButtons.back(), hints);
    // 6-> Next
    fTextButtons.push_back(new TGTextButton {fButtonsFrame, "&Next"});
    fTextButtons.back()->Connect("Pressed()", "ActRoot::EventPainter", this, "DoNextEvent()");
    fButtonsFrame->AddFrame(fTextButtons.back(), hints);
    // 7-> Run and entry list entries
    // Run
    fRunButton = new TGNumberEntry {fButtonsFrame,
                                    0,
                                    9,
                                    999,
                                    TGNumberFormat::kNESInteger,
                                    TGNumberFormat::kNEANonNegative,
                                    TGNumberFormat::kNELLimitMinMax,
                                    0,
                                    99999};
    fButtonsFrame->AddFrame(fRunButton, hints);
    // Entry
    fEntryButton = new TGNumberEntry {fButtonsFrame,
                                      0,
                                      9,
                                      999,
                                      TGNumberFormat::kNESInteger,
                                      TGNumberFormat::kNEANonNegative,
                                      TGNumberFormat::kNELLimitMinMax,
                                      0,
                                      99999};
    fButtonsFrame->AddFrame(fEntryButton, hints);
    // Connect ReturnPressed for both TGNumberEntries
    for(auto* b : {fRunButton, fEntryButton})
        b->GetNumberEntry()->Connect("ReturnPressed()", "ActRoot::EventPainter", this, "DoGoTo()");
    // 8-> Goto button
    fTextButtons.push_back(new TGTextButton {fButtonsFrame, "&GoTo"});
    fTextButtons.back()->Connect("Pressed()", "ActRoot::EventPainter", this, "DoGoTo()");
    fButtonsFrame->AddFrame(fTextButtons.back(), hints);

    // Map subwindows
    AddFrame(fButtonsFrame, new TGLayoutHints {kLHintsCenterX, 5, 5, 2, 1});
    fButtonsFrame->MapSubwindows();
    fButtonsFrame->MapWindow();
}

void ActRoot::EventPainter::AddTabs()
{
    fTabManager = new TGTab {this, 500, 400};
    auto* hints {new TGLayoutHints {kLHintsTop | kLHintsExpandX | kLHintsExpandY, 2, 2, 0, 0}};
    AddFrame(fTabManager, hints);
    // Tab 1
    fTabs.push_back(fTabManager->AddTab("TPC & clusters"));
    fFrames.push_back(new TGCompositeFrame {fTabs.back(), 500, 400, kHorizontalFrame});
    fTabs.back()->AddFrame(fFrames.back(), hints);
    // Tab 2
    fTabs.push_back(fTabManager->AddTab("Sil & others"));
    fFrames.push_back(new TGCompositeFrame {fTabs.back(), 500, 400, kHorizontalFrame});
    fTabs.back()->AddFrame(fFrames.back(), hints);
    // Map all
    fTabManager->MapSubwindows();
    fTabManager->MapWindow();
}

void ActRoot::EventPainter::AddCanvas()
{
    int idx {};
    for(auto& frame : fFrames)
    {
        frame->SetEditable(true);
        auto* c {new TCanvas {TString::Format("c%d", idx), TString::Format("Canvas %d", idx), (int)frame->GetWidth(),
                              (int)frame->GetHeight()}};
        if(c->GetShowToolBar())
            c->ToggleToolBar();
        if(c->GetShowEditor())
            c->ToggleEditor();
        if(!c->GetShowEventStatus())
            c->ToggleEventStatus();

        frame->SetEditable(false);
        fCanvas.push_back(c);
        idx++;
    }
}

void ActRoot::EventPainter::DoExit()
{
    gApplication->Terminate();
}

void ActRoot::EventPainter::CloseWindow()
{
    DoExit();
}

void ActRoot::EventPainter::Execute()
{
    if(fIsVerbose)
        DoVerbosePhysics();
    // Fill
    DoFill();
    // Draw
    DoDraw();
}

void ActRoot::EventPainter::DoDraw()
{
    fHistPainter->Draw();
    // Set info to status bar
    auto [run, entry] = fWrap->GetCurrentStatus();
    fRunButton->SetNumber(run, false);
    fEntryButton->SetNumber(entry, false);
}

void ActRoot::EventPainter::DoReset()
{
    // Reset histograms
    fHistPainter->Reset();
}

void ActRoot::EventPainter::DoFill()
{
    fHistPainter->Fill();
}

void ActRoot::EventPainter::DoPreviousEvent()
{
    DoReset();
    auto ok = fWrap->GoPrevious();
    if(!ok)
        return;
    Execute();
}

void ActRoot::EventPainter::DoNextEvent()
{
    DoReset();
    auto ok = fWrap->GoNext();
    if(!ok)
        return;
    Execute();
}

void ActRoot::EventPainter::DoGoTo()
{
    DoReset();
    auto run {fRunButton->GetIntNumber()};
    auto entry {fEntryButton->GetIntNumber()};
    auto ok = fWrap->GoTo(run, entry);
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
        fWrap->ReGet();
        auto tpc {fDetMan->GetDetectorAs<TPCDetector>()};
        // tpc->SetEventData(fWrap.GetTPCData());
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
        fWrap->ReGet();
        Execute();
    }
}

void ActRoot::EventPainter::DoVerbosePhysics()
{
    // TPC detector
    auto tpc {fDetMan->GetDetectorAs<TPCDetector>()};
    tpc->ClearEventFilter();
    tpc->BuildEventFilter();
    // To avoid scaling things in merger detector
    fWrap->CopyToClone2(tpc->GetOutputFilter());
    fWrap->GetTPCDataClone2()->Print();

    auto merger {fDetMan->GetDetectorAs<MergerDetector>()};
    if(merger->GetIsEnabled())
    {
        merger->ClearEventData();
        merger->BuildEventData();
        fWrap->GetMergerData()->Print();
    }
}
