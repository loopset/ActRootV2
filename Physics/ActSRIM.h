#ifndef ActSRIM_h
#define ActSRIM_h

#include "TF1.h"
#include "TSpline.h"

#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ActPhysics
{
    //! A class to interface with SRIM files
    /*!
      Allows to perform all type of computations with a SRIM table
      You cab even **draw** the file with the corresponding function!
      Warning: internally uses automatic pointers
    */
    class SRIM
    {
    private:
        bool fDebug;                    //!< To print debug info (passed in constructor)
        std::vector<std::string> fKeys; //!< Store known tables
        // Energy->Range
        std::map<std::string, std::unique_ptr<TSpline3>> fSplinesDirect;
        std::map<std::string, std::unique_ptr<TF1>> fInterpolationsDirect;
        // Range->Energy
        std::map<std::string, std::unique_ptr<TSpline3>> fSplinesInverse;
        std::map<std::string, std::unique_ptr<TF1>> fInterpolationsInverse;
        // Energy->Stopping powers (nuclear + electronic)
        std::map<std::string, std::unique_ptr<TSpline3>> fSplinesStoppings;
        std::map<std::string, std::unique_ptr<TF1>> fStoppings;
        // Range->Longitudinal straggling
        std::map<std::string, std::unique_ptr<TSpline3>> fSplinesLongStrag;
        std::map<std::string, std::unique_ptr<TF1>> fLongStrag;
        // Range->Lateral straggling
        std::map<std::string, std::unique_ptr<TSpline3>> fSplinesLatStrag;
        std::map<std::string, std::unique_ptr<TF1>> fLatStrag;


    public:
        SRIM(bool debug = false) : fDebug(debug) {}

        void ReadInterpolations(std::string key, std::string fileName);

        double EvalDirect(std::string key, double energy)
        {
            CheckFunctionArgument(energy);
            return fInterpolationsDirect[key]->Eval(energy);
        }
        double EvalInverse(std::string key, double range)
        {
            CheckFunctionArgument(range);
            return fInterpolationsInverse[key]->Eval(range);
        }

        // Evaluate the other columns
        double EvalStoppingPower(std::string key, double energy)
        {
            CheckFunctionArgument(energy);
            return fStoppings[key]->Eval(energy);
        }
        double EvalLongStraggling(std::string key, double range)
        {
            CheckFunctionArgument(range);
            return fLongStrag[key]->Eval(range);
        }
        double EvalLatStraggling(std::string key, double range)
        {
            CheckFunctionArgument(range);
            return fLatStrag[key]->Eval(range);
        }

        void Draw(std::string what, std::vector<std::string> keys = {});

        double
        ComputeEnergyLoss(double Tini, std::string material, double thickness, double angleInRad, int steps = 10);

        void CheckKeyIsStored(const std::string& key);

        void CheckFunctionArgument(double val);
    };
}; // namespace ActPhysics

#endif
