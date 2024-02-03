#ifndef ActEventPainter_h
#define ActEventPainter_h

#include "ActDetectorManager.h"
#include "ActHistogramPainter.h"
#include "ActInputData.h"
#include "ActInputIterator.h"

#include "RQ_OBJECT.h"
#include "Rtypes.h"
#include "RtypesCore.h"

#include "TCanvas.h"
#include "TGClient.h"
#include "TGFrame.h"
#include "TGNumberEntry.h"
#include "TGStatusBar.h"
#include "TGTab.h"
#include "TGWindow.h"
#include "TH2F.h"
#include "TObject.h"
#include "TRootEmbeddedCanvas.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace ActRoot
{
    class EventPainter : public TGMainFrame
    {
    private:
        // Tabs
        TGTab* fTabManager;
        TGCompositeFrame* fTab1;
        TGCompositeFrame* fFrame1;
        TGCompositeFrame* fTab2;
        TGCompositeFrame* fFrame2;
        // Buttons
        TGHorizontalFrame* fButtonsFrame;
        TGNumberEntry* fRunButton;
        TGNumberEntry* fEntryButton;
        // Histograms
        HistogramPainter fHistPainter;
        // Data to plot
        InputWrapper fWrap {};
        // Pointer to DetMan
        DetectorManager* fDetMan {};
        // In verbose mode?
        bool fIsVerbose {};

    public:
        EventPainter(const TGWindow* window, unsigned int width, unsigned int height);
        virtual ~EventPainter();

        void SetDetectorAndData(const std::string& detfile, const std::string& infile, bool outputAlso = false);
        void SetIsVerbose()
        {
            fIsVerbose = true;
            if(fDetMan)
                fDetMan->SetIsVerbose();
        }

        virtual void CloseWindow();
        void DoExit();
        void DoDraw();
        void DoFill();
        void DoReconfAndCluster();
        void DoReconfAndExecute();
        void DoReset();
        void DoPreviousEvent();
        void DoNextEvent();
        void InitTabs();
        void InitTab1();
        void InitTab2();
        void InitButtons();
        void InitEntryButtons();
        void DoGoTo();
        void Execute();
        void DoVerbosePhysics();

        ClassDef(EventPainter, 0);
    };
} // namespace ActRoot

#endif
