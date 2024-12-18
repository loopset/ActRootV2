#ifndef ActCrossSection_h
#define ActCrossSection_h

#include "TCanvas.h"
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
    std::vector<double> fY_AngleGraph {};
    double fStep {};
    bool fIsAngle {};
    double fTotalXS {};
    TSpline3* fCDF {};
    TSpline3* fCDFGraph {};
    TSpline3* fTheoXS {};

public:
    std::string StripSpaces(std::string line);
    void ReadData(const std::string& file, const bool isAngle);
    double xsIntervalcm(const TString& file, double minAngle, double maxAngle);
    void DrawCDF() const;
    double Sample(const double angle);
    void DrawTheo();
    double GetTotalXSmbarn() { return fTotalXS; }
    double GetTotalXScm() const { return fTotalXS * 1e-27; }

private:
};
} // namespace ActSim
#endif
