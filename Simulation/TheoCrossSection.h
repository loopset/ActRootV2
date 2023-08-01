#ifndef ActTheoCrossSection_h
#define ActTheoCrossSection_h
#include "TCanvas.h"
#include "TF1.h"
#include "TLegend.h"
#include "TRandom3.h"
#include "TSpline.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ActSim
{
    //! A class to implement theorethical cross sections in simulations
    /*!
      To be used in the same fashion as Geant4 does
      Computes the xs dotting the absolute * angular distributions using Legendre coefficients
    */
    class TheoCrossSection
    {
    private:
        std::vector<double> vXScattering {};
        std::vector<double> vYScattering {};
        std::unique_ptr<TSpline3> splineScattering {};
        std::unique_ptr<TF1> functScattering {};

        //for legendre: first is energy and second is coeff value
        std::map<int, std::pair<std::vector<double>, std::vector<double>>> mAngular {};
        std::map<int, std::unique_ptr<TSpline3>> splinesAngular {};
        std::map<int, std::unique_ptr<TF1>> funcsAngular {};

        double converteVToMeV {1.0E-6};
        int nIgnoredLinesScattering {155};
        int nIgnoredLinesAngular {158};
        int nLegendreCoeffs {6};

        int ENDFLineWidth {66};//characters
        int ENDFNumberWidth {11};// 66 / 11 = 6 numbers per line

        //for function at a set incident energy
        //IMPORTANT: lambda and its capture elements have to be defined as class members
        //(NOT inside method) becouse otherwise we cannot DRAW (and of course access) from outside it!!
        double scatteringXS {};
        std::vector<double> legendreCoeffs {};
        std::function<double(double*, double*)> lambda {};
        std::unique_ptr<TF1> xsAtEnergy {};

        //for canvas
        std::unique_ptr<TCanvas> canvScattering {};
        std::unique_ptr<TCanvas> canvAngular    {};
        std::unique_ptr<TCanvas> canvFinalXS      {};
        std::unique_ptr<TLegend> legendAngular  {};

    public:
        TheoCrossSection() = default;
        ~TheoCrossSection() = default;

        void SetNIgnoredLinesScattering(int n){ nIgnoredLinesScattering = n; }
        int GetNIgnoredLinesScattering() const { return nIgnoredLinesScattering; }

        void SetNIgnoredLinesAngular(int n){ nIgnoredLinesAngular = n; }
        int GetNIgnoredLinesAngular() const { return nIgnoredLinesAngular; }

        void SetNLegendreCoefficients(int n){ nLegendreCoeffs = n; }
        int GetNLegendreCoefficients() const { return nLegendreCoeffs; }
    
        void ReadScatteringCrossSection(std::string fileName);
        void ReadAngularCrossSection(std::string fileName);

        void ComputeXSAtEnergy(double Tn);
        double EvalScatteringXS(double Tn);
        double IntegrateTotalXS(double low = -1.0, double high = 1.0);
        double SampleXS(TRandom3* generator = nullptr);

        void DrawScattering();
        void DrawAngular();

        void DrawXS();

        void DrawFiles();

    private:
        void ParseLine(std::string& line, std::vector<double>& coeffs);
    };

}

#endif
