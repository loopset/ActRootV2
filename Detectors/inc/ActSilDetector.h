#ifndef ActSilDetector_h
#define ActSilDetector_h

#include "ActSilData.h"
#include "ActSilParameters.h"
#include "ActTPCLegacyData.h"
#include "ActVData.h"
#include "ActVDetector.h"

#include "TTree.h"

namespace ActRoot
{
//! Silicon detector class
class SilDetector : public VDetector
{
private:
    // Parameters
    SilParameters fPars; //!< Basic detector configurations
    // Pointer to MEvent
    MEventReduced* fMEvent {};
    // Data
    SilData* fData {}; //!< Pointer to SilData

    // Set flags to delete new in destructor
    bool fDelMEvent {};
    bool fDelData {};

public:
    SilDetector() = default;
    ~SilDetector() override;

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
    SilData* GetInputData() const override { return nullptr; }
    void SetOutputData(VData* data) override { fData = data->CastAs<SilData>(); }
    SilData* GetOutputData() const override { return fData; }

    void SetInputFilter(VData* data) override { fData = data->CastAs<SilData>(); }
    SilData* GetInputFilter() const override { return fData; }
    void SetOutputFilter(VData* data) override { fData = data->CastAs<SilData>(); }
    SilData* GetOutputFilter() const override { return fData; }

    // Getters of parameters
    SilParameters* GetParameters() override { return &fPars; }

    // Printer of configuration
    void Print() const override;
    // Printer of reports
    void PrintReports() const override;

    // Share MEvent
    void SetMEvent(MEventReduced* mevent) override { fMEvent = mevent; }
    MEventReduced* GetMEvent() override { return fMEvent; }
};
} // namespace ActRoot

#endif
