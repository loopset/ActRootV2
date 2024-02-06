#ifndef ActSilDetector_h
#define ActSilDetector_h

#include "ActSilData.h"
#include "ActTPCLegacyData.h"
#include "ActVData.h"
#include "ActVDetector.h"
#include "ActVParameters.h"

#include "TTree.h"

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ActRoot
{
//! A class holding basic silicon parameters
/*!
  For now, just keeps sizes of silicons
  using strings as identifiers and VXI equivalences
*/
class SilParameters : public VParameters
{
private:
    std::unordered_map<std::string, int> fSizes; //!< Sizes of silicons based on string, for [0, max]
    std::map<int, std::pair<std::string, int>> fVXI;

public:
    // Setters
    void SetLayer(const std::string& key, int size) { fSizes[key] = size; }
    // Getters
    std::vector<std::string> GetKeys() const;
    int GetSizeOf(const std::string& key) { return fSizes[key]; }
    void Print() const override; //!< Dump info stored
    std::pair<std::string, int> GetSilIndex(int vxi);
    void
    ReadActions(const std::vector<std::string>& layers, const std::vector<std::string>& names, const std::string& file);
};

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

public:
    SilDetector() = default;
    virtual ~SilDetector() = default;

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
