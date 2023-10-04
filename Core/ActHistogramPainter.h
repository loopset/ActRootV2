#ifndef ActHistogramPainter_h
#define ActHistogramPainter_h

#include "TCanvas.h"
#include "TH2.h"
#include "TH2F.h"
#include "ActSilDetector.h"
#include "ActTPCDetector.h"
#include "ActInputIterator.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ActRoot
{
    //!< A class to plot physics info in histograms
    class HistogramPainter
    {
    private:
        std::map<int, TCanvas*> fCanvs;
        std::map<int, std::shared_ptr<TH2F>> fHistTpc;
        std::map<int, std::shared_ptr<TH2F>> fHistSil;

        //Parameters of detectors
        TPCParameters fTPC;
        SilParameters fSil;
        //Silicon map
        std::unordered_map<std::string, std::vector<std::pair<int, int>>> fSilMap;

        //Pointer to Wrapper
        InputWrapper* fWrap;
        
    public:
        HistogramPainter() = default;
        HistogramPainter(const std::string& detfile);

        //Init from file
        void ReadDetFile(const std::string& file);

        //Set wrapper pointer
        void SetInputWrapper(InputWrapper* wrap) {fWrap = wrap;} 
        
        //TCanvas
        void SetCanvas(int i, TCanvas* canv){fCanvs[i] = canv;}
        TCanvas* SetCanvas(int i, const std::string& title, double w, double h);
        TCanvas* GetCanvas(int i) const {return fCanvs.at(i);}
        
        void Fill();
        void Draw();
        void Reset();

    private:
        void Init();
        void FillSilHisto(int pad, const std::string& layer);
    };
}

#endif
