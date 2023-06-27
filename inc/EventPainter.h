#ifndef ActEventPainter_h
#define ActEventPainter_h

#include "InputData.h"
#include "RQ_OBJECT.h"
#include "Rtypes.h"
#include "TCanvas.h"
#include "TGClient.h"
#include "TGFrame.h"
#include "TGWindow.h"
#include "TPCData.h"
#include "TPCDetector.h"
#include "TRootEmbeddedCanvas.h"
#include "TH2F.h"
#include "TGStatusBar.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace ActRoot
{
    class EventPainter : public TGMainFrame
    {
    private:
        TRootEmbeddedCanvas* fEmCanv;
        TCanvas* fCanv;
        TGStatusBar* fStatus;
        TGHorizontalFrame* fButtonsFrame;
        std::unordered_map<int, std::shared_ptr<TH2F>> fHist2d;
        //TPC detector configuration
        TPCParameters ftpc;
        //Holders for current event and run
        int fCurrentRun;
        int fCurrentEntry;
        InputData* fInput;
        TPCData* fData;
        
    public:
        EventPainter(const std::string& file, const TGWindow* window, unsigned int width, unsigned int height);
        virtual ~EventPainter();

        void SetInputData(ActRoot::InputData* input);
        virtual void CloseWindow();
        void DoExit();
        void DoDraw();
        void DoFill();
        void DoReset();
        void DoNextEvent();
        void InitCanvas();
        void Init2DHistograms();
        void ResetHistograms();
        void ResetCanvas();
        void ReadConfiguration(const std::string& file);

        ClassDef(EventPainter, 0);
    };
}

#endif
