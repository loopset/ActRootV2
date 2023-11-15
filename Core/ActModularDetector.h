#ifndef ActModularDetector_h
#define ActModularDetector_h

#include "ActModularData.h"
#include "ActVData.h"
#include "ActVDetector.h"

#include "TTree.h"

#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ActRoot
{
    //! A class holding ModularLeaf experimental setups: VXI equivalences
    class ModularParameters
    {
    private:
        std::map<int, std::string> fVXI;

    public:
        std::string GetName(int vxi); //!< Get name of ModularLeaf according to Action file
        void ReadActions(const std::vector<std::string>& names,
                         const std::string& file); //!< Read Action file
        void Print() const;
    };

    //!< Detector of type ModularLeaf
    class ModularDetector : public VDetector
    {
    private:
        // Parameters
        ModularParameters fPars; //!< Basic detector configurations
        // Data
        ModularData* fData {}; //!< Pointer to Data

    public:
        ModularDetector() = default;
        virtual ~ModularDetector() = default;

        // Pointer to parameters
        ModularParameters* GetParametersPointer() { return &fPars; }

        void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        void Reconfigure() override;
        void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        void InitInputRawData(std::shared_ptr<TTree> tree, int run) override;
        void InitInputData(std::shared_ptr<TTree> tree) override;
        void InitOutputData(std::shared_ptr<TTree> tree) override;
        void InitOutputPhysics(std::shared_ptr<TTree> tree) override;
        void BuildEventData() override;
        void BuildEventPhysics() override;
        void ClearEventData() override;
        void ClearEventPhysics() override;

        // Getters
        ModularData* GetEventData() const override { return fData; }
        ModularData* GetEventPhysics() const override { return fData; }

        // Setters
        void SetEventData(VData* vdata) override;

        // Print any reports from analysis
        void PrintReports() const override;
    };
} // namespace ActRoot

#endif
