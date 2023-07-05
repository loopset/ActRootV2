#ifndef ActEventPainter_h
#define ActEventPainter_h

#include "InputData.h"
#include "RQ_OBJECT.h"
#include "Rtypes.h"
#include "RtypesCore.h"
#include "SilDetector.h"
#include "TCanvas.h"
#include "TGClient.h"
#include "TGFrame.h"
#include "TGNumberEntry.h"
#include "TGWindow.h"
#include "TObject.h"
#include "TRootEmbeddedCanvas.h"
#include "TH2F.h"
#include "TGStatusBar.h"
#include "TGTab.h"

#include "TPCData.h"
#include "TPCDetector.h"
#include "InputIterator.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace ActRoot
{
    class EventPainter : public TGMainFrame
    {
    private:
        //Tabs
        TGTab* fTabManager;
        TGCompositeFrame* fTab1;
        TGCompositeFrame* fFrame1;
        TGCompositeFrame* fTab2;
        TGCompositeFrame* fFrame2;
        TCanvas* fCanv1;
        //Status bar
        TGStatusBar* fStatusCanv;
        TGStatusBar* fStatusThis;
        TGHorizontalFrame* fStatusFrame;
        //Buttons
        TGHorizontalFrame* fButtonsFrame;
        TGNumberEntry* fRunButton;
        TGNumberEntry* fEntryButton;
        //Histograms
        std::unordered_map<int, std::shared_ptr<TH2F>> fHist2d;
        //TPC detector configuration
        TPCParameters ftpc;
        //Sil detector
        SilParameters fsil;
        //Data to plot
        InputWrapper fWrap {};
        
    public:
        EventPainter(const TGWindow* window, unsigned int width, unsigned int height);
        virtual ~EventPainter();
        
        void SetDetAndData(const std::string& detector, InputData* input);
        
        virtual void CloseWindow();
        void DoExit();
        void DoDraw();
        void DoFill();
        void DoReset();
        void DoPreviousEvent();
        void DoNextEvent();
        void Init2DHistograms();
        void ResetHistograms();
        void ResetCanvas();
        void CanvasToStatusBar(int event, int px, int py, TObject* obj);
        void InitStatusBars();
        void InitTabs();
        void InitTab1();
        void InitTab2();
        void InitButtons();
        void InitEntryButtons();
        void DoGoTo();

    private:
        void ReadConfiguration(const std::string& file);

        ClassDef(EventPainter, 0);
    };
}

#endif
