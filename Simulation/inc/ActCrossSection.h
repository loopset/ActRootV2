#ifndef ActCrossSection_h
#define ActCrossSection_h

#include "TH1.h"
#include "TSpline.h"

#include <string>
#include <vector>

namespace ActSim
{
class CrossSection
{
private:
    std::vector<double> fX {};
    std::vector<double> fY {};
    TSpline3* fCDF {};       //!< For CDF sampling
    TH1D* fHist {};          //!< For direct count sampling
    TGraph* fTheoXSGraph {}; //!< Theoretical input graph
    double fStep {};
    double fTotalXS {};
    bool fIsAngle {};

public:
    CrossSection(bool isAngular = true) : fIsAngle(isAngular) {}

    // Main methods to read
    void ReadGraph(TGraph* g);
    void ReadFile(const std::string& file);


    // Sampling methods
    double SampleCDF(double r);
    double SampleCDF();
    double SampleHist(TRandom* rand = nullptr);

    // Getters
    double GetTotalXSmbarn() { return fTotalXS; }
    double GetTotalXScm2() const { return fTotalXS * 1e-27; }
    double GetIntervalXS(double minAngle, double maxAngle);

    // Others
    void Draw() const;

private:
    void Init(int n, const double* x, const double* y);
};
} // namespace ActSim
#endif
