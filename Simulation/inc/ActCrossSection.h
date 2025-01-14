#ifndef ActCrossSection_h
#define ActCrossSection_h

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
    TSpline3* fCDF {};
    TGraph* fTheoXSGraph {};
    double fStep {};
    double fTotalXS {};
    bool fIsAngle {};

public:
    CrossSection(bool isAngular = true) : fIsAngle(isAngular) {}

    // Main methods to read
    void ReadGraph(TGraph* g);
    void ReadFile(const std::string& file);

    // Getters and others
    double xsIntervalcm(const TString& file, double minAngle, double maxAngle);
    void DrawCDF() const;
    double Sample(double r);
    double Sample(); // Using internal gRandom
    void DrawTheo();
    double GetTotalXSmbarn() { return fTotalXS; }
    double GetTotalXScm2() const { return fTotalXS * 1e-27; }

private:
    void Init(int n, const double* x, const double* y);
};
} // namespace ActSim
#endif
