#ifndef ActModularDetector_h
#define ActModularDetector_h

#include "ActModularData.h"
#include "ActTPCLegacyData.h"
#include "ActVData.h"
#include "ActVDetector.h"
#include "ActVParameters.h"

#include "TTree.h"

#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ActRoot
{
//! A class holding ModularLeaf experimental setups: VXI equivalences
class ModularParameters : public VParameters
{
private:
    std::map<int, std::string> fVXI;

public:
    std::string GetName(int vxi); //!< Get name of ModularLeaf according to Action file
    void ReadActions(const std::vector<std::string>& names,
                     const std::string& file); //!< Read Action file
    void Print() const override;
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

    void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
    void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
    void Reconfigure() override;

    // Init inputs
    void InitInputData(std::shared_ptr<TTree> tree) override;
    void InitInputFilter(std::shared_ptr<TTree> tree) override;

    // Init outputs
    void InitOutputData(std::shared_ptr<TTree> tree) override;
    void InitOutputFilter(std::shared_ptr<TTree> tree) override;

    // Builders
    void BuildEventData(int run = -1, int entry = -1) override;
    void BuildEventFilter() override;

    // Cleaners
    void ClearEventData() override;
    void ClearEventFilter() override;

    // Getters of data
    ModularData* GetEventData() const override { return fData; }
    VData* GetEventMerger() const override { return nullptr; } // managed by MergerDetector

    // Setters of data
    void SetEventData(VData* vdata) override;

    // Getter of parameters
    ModularParameters* GetParameters() override { return &fPars; }

    // Printer of configuration
    void Print() const override;
    // Printer of parameters
    void PrintReports() const override;

    // Share MEvent
    void SetMEvent(MEventReduced* mevent) override { fMEvent = mevent; }
    MEventReduced* GetMEvent() override { return fMEvent; }
};
} // namespace ActRoot

#endif
