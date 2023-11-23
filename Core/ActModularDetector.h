#ifndef ActModularDetector_h
#define ActModularDetector_h

#include "ActModularData.h"
#include "ActTPCLegacyData.h"
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
        // Pointer to MEventReduced
        MEventReduced* fMEvent {};
        // Data
        ModularData* fData {}; //!< Pointer to Data

    public:
        ModularDetector() = default;
        virtual ~ModularDetector() = default;

        // Pointer to parameters
        ModularParameters* GetParametersPointer() { return &fPars; }

        void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        void Reconfigure() override;

        // Init inputs
        void InitInputRaw(std::shared_ptr<TTree> tree) override;
        void InitInputMerger(std::shared_ptr<TTree> tree) override;

        // Init outputs
        void InitOutputData(std::shared_ptr<TTree> tree) override;
        void InitOutputMerger(std::shared_ptr<TTree> tree) override;

        // Builders
        void BuildEventData() override;
        void BuildEventMerger() override;

        // Cleaners
        void ClearEventData() override;
        void ClearEventMerger() override;

        // Getters of data
        ModularData* GetEventData() const override { return fData; }
        VData* GetEventMerger() const override { return nullptr; } // managed by MergerDetector

        // Setters of data
        void SetEventData(VData* vdata) override;

        // Printer of configuration
        void Print() const override;
        // Printer of parameters
        void PrintReports() const override;
    };
} // namespace ActRoot

#endif
