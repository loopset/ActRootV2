#ifndef ActEventPainter_h
#define ActEventPainter_h

#include "InputData.h"
#include "RQ_OBJECT.h"
#include "Rtypes.h"
#include "RtypesCore.h"
#include "TCanvas.h"
#include "TGClient.h"
#include "TGFrame.h"
#include "TGWindow.h"
#include "TObject.h"
#include "TPCData.h"
#include "TPCDetector.h"
#include "TRootEmbeddedCanvas.h"
#include "TH2F.h"
#include "TGStatusBar.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace ActRoot
{
    class InputWrapper
    {
    private:
        InputData* fInput;
        InputIterator fIt;
        TPCData* fData;

    public:
        InputWrapper() = default;
        InputWrapper(InputData* input);
        ~InputWrapper() = default;

        void GetEntry(int run, int entry);
        void GoNext();
        void GoPrevious();
        TPCData* GetCurrentData() {return fData;}
        std::pair<int, int> GetCurrentStatus() const {return fIt.GetCurrentRunEntry();}
    };
    class EventPainter : public TGMainFrame
    {
    private:
        TRootEmbeddedCanvas* fEmCanv;
        TCanvas* fCanv;
        TGStatusBar* fStatusCanv;
        TGStatusBar* fStatusThis;
        TGHorizontalFrame* fButtonsFrame;
        std::unordered_map<int, std::shared_ptr<TH2F>> fHist2d;
        //TPC detector configuration
        TPCParameters ftpc;
        //Data to plot
        InputWrapper fWrap;
        
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
        void InitCanvas();
        void Init2DHistograms();
        void ResetHistograms();
        void ResetCanvas();
        void ReadConfiguration(const std::string& file);
        void CanvasToStatusBar(int event, int px, int py, TObject* obj);
        void InitStatusBars();

        ClassDef(EventPainter, 0);
    };
}

#endif
