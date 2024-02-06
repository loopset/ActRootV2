#ifndef ActEventPainter_h
#define ActEventPainter_h

#include "ActDetectorManager.h"
#include "ActHistogramPainter.h"
#include "ActInputIterator.h"

#include "Rtypes.h"

#include "TCanvas.h"
#include "TGButton.h"
#include "TGClient.h"
#include "TGFrame.h"
#include "TGNumberEntry.h"
#include "TGTab.h"
#include "TGWindow.h"
#include "TRootEmbeddedCanvas.h"

#include <vector>

namespace ActRoot
{
class EventPainter : public TGMainFrame
{
private:
    // Tabs
    TGTab* fTabManager {};
    std::vector<TGCompositeFrame*> fTabs {};
    std::vector<TGCompositeFrame*> fFrames {};
    // Each tab has a TCanvas object
    std::vector<TCanvas*> fCanvas {};

    // Buttons
    TGHorizontalFrame* fButtonsFrame {};
    TGNumberEntry* fRunButton {};
    TGNumberEntry* fEntryButton {};
    std::vector<TGTextButton*> fTextButtons {};

    // Histogram server
    HistogramPainter* fHistPainter {};
    // Input manager
    InputWrapper* fWrap {};
    // Pointer to DetMan
    DetectorManager* fDetMan {};

    // In verbose mode?
    bool fIsVerbose {};

public:
    EventPainter(const TGWindow* window, unsigned int width, unsigned int height);
    virtual ~EventPainter();

    // Setters
    void SendDetectorManager(DetectorManager* detman) { fDetMan = detman; }
    void SendHistogramServer(HistogramPainter* hist) { fHistPainter = hist; }
    void SendInputWrapper(InputWrapper* wrap) { fWrap = wrap; }

    virtual void CloseWindow();

    // Getters
    std::vector<TCanvas*>* GetCanvasPtr() { return &fCanvas; }

    // Exec funtions
    void DoExit();
    void DoDraw();
    void DoFill();
    void DoReconfAndCluster();
    void DoReconfAndExecute();
    void DoReset();
    void DoPreviousEvent();
    void DoNextEvent();
    void DoGoTo();
    void Execute();
    void DoVerbosePhysics();

    ClassDef(EventPainter, 0);

private:
    void AddButtons();
    void AddTabs();
    void AddCanvas();
};
} // namespace ActRoot

#endif
