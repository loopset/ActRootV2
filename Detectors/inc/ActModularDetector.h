#ifndef ActModularDetector_h
#define ActModularDetector_h

#include "ActModularData.h"
#include "ActModularParameters.h"
#include "ActTPCLegacyData.h"
#include "ActVData.h"
#include "ActVDetector.h"

#include "TTree.h"

#include <memory>

namespace ActRoot
{
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
    // Flag to delete new on destructor
    bool fDelMEvent {};
    bool fDelData {};

public:
    ModularDetector() = default;
    ~ModularDetector() override;

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

    // Setters and getters of data
    void SetInputData(VData* data) override {}
    ModularData* GetInputData() const override { return nullptr; }
    void SetOutputData(VData* data) override { fData = data->CastAs<ModularData>(); }
    ModularData* GetOutputData() const override { return fData; }

    void SetInputFilter(VData* data) override { fData = data->CastAs<ModularData>(); }
    ModularData* GetInputFilter() const override { return fData; }
    void SetOutputFilter(VData* data) override { fData = data->CastAs<ModularData>(); }
    ModularData* GetOutputFilter() const override { return fData; }

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
