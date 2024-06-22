#ifndef ActSRIM_h
#define ActSRIM_h

#include "TF1.h"
#include "TGraph.h"
#include "TSpline.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

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
    using PtrSpline = std::unique_ptr<TSpline3>;
    using PtrFunc = std::unique_ptr<TF1>;
    using PtrGraph = std::unique_ptr<TGraph>;

private:
    std::vector<std::string> fKeys; //!< Store known tables
    // Energy->Range
    std::map<std::string, std::unique_ptr<TSpline3>> fSplinesDirect;
    std::map<std::string, std::unique_ptr<TF1>> fInterpolationsDirect;
    std::map<std::string, PtrGraph> fGraphsDirect;
    // Range->Energy
    std::map<std::string, std::unique_ptr<TSpline3>> fSplinesInverse;
    std::map<std::string, std::unique_ptr<TF1>> fInterpolationsInverse;
    std::map<std::string, PtrGraph> fGraphsInverse;
    // Energy->Stopping powers (nuclear + electronic)
    std::map<std::string, std::unique_ptr<TSpline3>> fSplinesStoppings;
    std::map<std::string, std::unique_ptr<TF1>> fStoppings;
    std::map<std::string, PtrGraph> fGraphsStoppings;
    // Range->Longitudinal straggling
    std::map<std::string, std::unique_ptr<TSpline3>> fSplinesLongStrag;
    std::map<std::string, std::unique_ptr<TF1>> fLongStrag;
    std::map<std::string, PtrGraph> fGraphsLongStrag;
    // Range->Lateral straggling
    std::map<std::string, std::unique_ptr<TSpline3>> fSplinesLatStrag;
    std::map<std::string, std::unique_ptr<TF1>> fLatStrag;
    std::map<std::string, PtrGraph> fGraphsLatStrag;


public:
    SRIM() = default;
    SRIM(const std::string& material, const std::string& file);

    void ReadTable(const std::string& key, const std::string& file);

    [[deprecated("Favour use of new SRIM::ReadTable(...) which does not require manual edition of SRIM file")]] void
    ReadInterpolations(std::string key, std::string fileName);

    double EvalDirect(const std::string& key, double energy) { return fInterpolationsDirect[key]->Eval(energy); }
    double EvalInverse(const std::string& key, double range) { return fInterpolationsInverse[key]->Eval(range); }

    // Evaluate the other columns of the SRIM table
    double EvalStoppingPower(const std::string& key, double energy) { return fStoppings[key]->Eval(energy); }
    double EvalLongStraggling(const std::string& key, double range) { return fLongStrag[key]->Eval(range); }
    double EvalLatStraggling(const std::string& key, double range) { return fLatStrag[key]->Eval(range); }

    void Draw(const std::string& what, const std::vector<std::string>& keys = {});

    double Slow(const std::string& material, double Tini, double thickness, double angleInRad = 0);

    double EvalInitialEnergy(const std::string& material, double Tafter, double thickness, double angleInRad = 0);

    bool CheckKeyIsStored(const std::string& key);

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
