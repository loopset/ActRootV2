#ifndef ActHistogramPainter_h
#define ActHistogramPainter_h

#include "ActDetectorManager.h"
#include "ActInputIterator.h"
#include "ActSilDetector.h"
#include "ActTPCDetector.h"

#include "TCanvas.h"
#include "TGraph.h"
#include "TH2.h"
#include "TPolyLine.h"
#include "TPolyMarker.h"
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
public:
    using Histo2DMap = std::map<int, std::shared_ptr<TH2F>>;
    using Histo1DMap = std::map<int, std::shared_ptr<TH1F>>;
    using LineMap = std::map<int, std::vector<std::shared_ptr<TPolyLine>>>;
    using MarkerMap = std::map<int, std::vector<std::shared_ptr<TPolyMarker>>>;
    using GraphMap = std::map<int, std::vector<std::shared_ptr<TGraph>>>;

private:
    // Canvases, from EventPainter
    std::vector<TCanvas*>* fCanvas;

    // Pointer to Wrapper
    InputWrapper* fWrap {};

    // Pointer to DetMan
    DetectorManager* fDetMan {};

    // Histograms
    std::map<int, Histo2DMap> fHist2D;
    std::map<int, Histo1DMap> fHist1D;
    std::map<int, LineMap> fLines;
    std::map<int, MarkerMap> fMarkers;
    std::map<int, std::shared_ptr<TH2F>> fHistTpc;
    std::map<int, std::shared_ptr<TH2F>> fHistSil;
    std::map<int, std::vector<std::shared_ptr<TPolyLine>>> fPolyTpc;
    std::map<int, std::shared_ptr<TPolyMarker>> fMarkerTpc;
    std::map<int, GraphMap> fGraphs;

    // Parameters of detectors
    TPCParameters* fTPC {};
    SilParameters* fSil {};

    // Silicon map
    std::unordered_map<std::string, std::vector<std::pair<int, int>>> fSilMap;

    // Store settings read in config file
    bool fShowHistStats {false};

public:
    HistogramPainter();

    // Initialization of histogram structure
    void Init();

    // Configuration file of class
    void ReadConfiguration();

    // Pass parameters from DetMan to this class
    void SendParameters(DetectorManager* detman);

    // Send wrapper pointer
    void SendInputWrapper(InputWrapper* wrap) { fWrap = wrap; }

    // Send detector manager
    void SendDetectorManager(DetectorManager* detman)
    {
        fDetMan = detman;
        InitRegionGraphs();
    }

    // TCanvas
    void SendCanvas(std::vector<TCanvas*>* canv)
    {
        fCanvas = canv;
        Init();
    }

    // Main functions
    void Fill();
    void Draw();
    void Reset();

private:
    void SetPalette(const std::string& name, bool reverse = false);
    void FillVoxelsHisto();
    void FillSilHisto(int pad, const std::string& layer);
    void FillClusterHistos();
    void InitRegionGraphs();
    void DrawRegions();
    void DrawPolyLines();
    void DrawPolyMarkers();
    void AttachBinToCluster(std::shared_ptr<TH2F> h, double x, double y, int clusterID);
};
} // namespace ActRoot

#endif
