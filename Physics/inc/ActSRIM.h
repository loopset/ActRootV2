#ifndef ActSRIM_h
#define ActSRIM_h

#include "TGraph.h"
#include "TSpline.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class TRandom;

namespace ActRoot
{
class InputBlock;
}

namespace ActPhysics
{
//! A class to interface with SRIM files
/*!
  Allows to perform all type of computations with a SRIM table
  You can even **draw** the file with the corresponding function!
  Warning: internally uses automatic pointers
*/
class SRIM
{
public:
    using PtrSpline = std::shared_ptr<TSpline3>;
    using PtrGraph = std::shared_ptr<TGraph>;

private:
    std::vector<std::string> fKeys; //!< Store known tables
    // Energy->Range
    std::map<std::string, PtrSpline> fSplinesDirect;
    std::map<std::string, PtrGraph> fGraphsDirect;
    // Range->Energy
    std::map<std::string, PtrSpline> fSplinesInverse;
    std::map<std::string, PtrGraph> fGraphsInverse;
    // Energy->Stopping powers (nuclear + electronic)
    std::map<std::string, PtrSpline> fSplinesStoppings;
    std::map<std::string, PtrGraph> fGraphsStoppings;
    // Range->Longitudinal straggling
    std::map<std::string, PtrSpline> fSplinesLongStrag;
    std::map<std::string, PtrGraph> fGraphsLongStrag;
    // Range->Lateral straggling
    std::map<std::string, PtrSpline> fSplinesLatStrag;
    std::map<std::string, PtrGraph> fGraphsLatStrag;
    // Bool to use spline or not
    bool fUseSpline {false};


public:
    SRIM() = default;
    SRIM(const std::string& material, const std::string& file);

    // Read SRIM tables
    void ReadTable(const std::string& key, const std::string& file);
    [[deprecated("Favour use of new SRIM::ReadTable(...) which does not require manual edition of SRIM file")]] void
    ReadInterpolations(std::string key, std::string fileName);

    // Set spline flag
    void SetUseSpline(bool use = true) { fUseSpline = use; }

    // Main functions
    // Explicit names (easy to understand)
    double EvalRange(const std::string& key, double energy) { return EvalDirect(key, energy); }
    double EvalEnergy(const std::string& key, double range) { return EvalInverse(key, range); }
    // Legacy names (what does direct mean? range? energy?)
    double EvalDirect(const std::string& key, double energy);
    double EvalInverse(const std::string& key, double range);

    // Evaluate the other columns of the SRIM table
    double EvalStoppingPower(const std::string& key, double energy);
    double EvalLongStraggling(const std::string& key, double range);
    double EvalLatStraggling(const std::string& key, double range);

    // Drawing method
    void Draw(const std::vector<std::string>& keys = {});

    // Main methods: same as in nptools
    double Slow(const std::string& material, double Tini, double thickness, double angleInRad = 0);

    double SlowWithStraggling(const std::string& material, double Tini, double thickness, double angleInRad = 0,
                              TRandom* rand = nullptr);

    double EvalInitialEnergy(const std::string& material, double Tafter, double thickness, double angleInRad = 0);

    double TravelledDistance(const std::string& material, double Tini, double Tafter);

    bool CheckKeyIsStored(const std::string& key);

    // Methods to read from file! Header is [SRIM]
    void ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block);

    void ReadConfiguration(const std::string& file);

private:
    bool IsBreakLine(const std::string& line);
    double ConvertToDouble(std::string& str, const std::string& unit);
    PtrGraph GetGraph(std::vector<double>& x, std::vector<double>& y, const std::string& name);
    PtrSpline GetSpline(std::vector<double>& x, std::vector<double>& y, const std::string& name);
};
}; // namespace ActPhysics

#endif
