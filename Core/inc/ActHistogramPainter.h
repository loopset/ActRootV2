#ifndef ActHistogramPainter_h
#define ActHistogramPainter_h

#include "ActDetectorManager.h"
#include "ActInputIterator.h"
#include "ActMergerData.h"
#include "ActSilDetector.h"
#include "ActTPCDetector.h"
#include "ActVData.h"

#include "TCanvas.h"
#include "TH2.h"
#include "TH2F.h"
#include "TPolyLine.h"
#include "TTree.h"

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
        // Canvases
        std::map<int, TCanvas*> fCanvs;
        // Histograms
        std::map<int, std::shared_ptr<TH2F>> fHistTpc;
        std::map<int, std::shared_ptr<TH2F>> fHistSil;
        std::map<int, std::vector<std::shared_ptr<TPolyLine>>> fPolyTpc;
        std::map<int, std::shared_ptr<TPolyMarker>> fMarkerTpc;
        // Parameters of detectors
        TPCParameters* fTPC {};
        SilParameters* fSil {};
        // Silicon map
        std::unordered_map<std::string, std::vector<std::pair<int, int>>> fSilMap;

        // Pointer to Wrapper
        InputWrapper* fWrap {};

        // Store settings read in config file
        bool fShowHistStats {false};

    public:
        HistogramPainter() = default;

        // Initialization of histogram structure
        void Init();

        // Configuration file of class
        void ReadConfigurationFile(const std::string& file = "");

        // Pass parameters from DetMan to this class
        void SendParameters(DetectorManager* detman);

        // Send wrapper pointer
        void SendInputWrapper(InputWrapper* wrap) { fWrap = wrap; }

        // TCanvas
        void SetCanvas(int i, TCanvas* canv) { fCanvs[i] = canv; }
        TCanvas* SetCanvas(int i, const std::string& title, double w, double h);
        TCanvas* GetCanvas(int i) const { return fCanvs.at(i); }

        // Main functions
        void Fill();
        void Draw();
        void Reset();

    private:
        void SetPalette(const std::string& name, bool reverse = false);
        void FillVoxelsHisto();
        void FillSilHisto(int pad, const std::string& layer);
        void FillClusterHistos();
        void DrawPolyLines();
        void DrawPolyMarkers();
        void AttachBinToCluster(std::shared_ptr<TH2F> h, double x, double y, int clusterID);
    };
} // namespace ActRoot

#endif
