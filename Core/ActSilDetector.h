#ifndef ActSilDetector_h
#define ActSilDetector_h

#include "ActSilData.h"
#include "ActTPCLegacyData.h"
#include "ActVData.h"
#include "ActVDetector.h"

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
    class SilParameters
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
        void Print() const; //!< Dump info stored
        std::pair<std::string, int> GetSilIndex(int vxi);
        void ReadActions(const std::vector<std::string>& layers, const std::vector<std::string>& names,
                         const std::string& file);
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

        // Parameter getters
        SilParameters* GetParametersPointer() { return &fPars; }

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
        SilData* GetEventData() const override { return fData; }
        VData* GetEventMerger() const override { return nullptr; } // managed by MergerDetector

        // Setters of data
        void SetEventData(VData* vdata) override;

        // Printer of configuration
        void Print() const override;
        // Printer of reports
        void PrintReports() const override;
    };
} // namespace ActRoot

#endif
