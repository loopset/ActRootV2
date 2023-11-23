#ifndef ActVDetector_h
#define ActVDetector_h

#include "ActCalibrationManager.h"
#include "ActTPCLegacyData.h"
#include "ActVData.h"

#include "TTree.h"

#include <memory>
#include <string>

// Abstract class representing a detector
namespace ActRoot
{
    class InputBlock; // forward declaration
    //! Base class contaning all methods of an analysis detector
    class VDetector
    {
    protected:
        std::shared_ptr<CalibrationManager> fCalMan {}; //!< Pointer to CalibrationManager to avoid working with
                                                        //!< singleton

    public:
        VDetector() = default;
        virtual ~VDetector() = default;

        // Read configuration for each detector
        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) = 0;

        // Reconfigure internal parameters from config files
        virtual void Reconfigure() = 0;

        // Interface to CalibrationManager
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) = 0;

        // Initialize data INPUTS
        virtual void InitInputRaw(std::shared_ptr<TTree> tree) = 0;
        virtual void InitInputMerger(std::shared_ptr<TTree> tree) = 0;

        // Initialize data OUTPUTS
        virtual void InitOutputData(std::shared_ptr<TTree> tree) = 0;
        virtual void InitOutputMerger(std::shared_ptr<TTree> tree) = 0;

        // Build
        virtual void BuildEventData() = 0;
        virtual void BuildEventMerger() = 0;

        // Clear data
        virtual void ClearEventData() = 0;
        virtual void ClearEventMerger() = 0;

        // Getters of data structures
        virtual VData* GetEventData() const { return nullptr; }
        virtual VData* GetEventMerger() const { return nullptr; }

        // Getters
        virtual void SetEventData(VData* vdata) = 0;

        // Set and Get CalibrationManager pointer
        void SetCalMan(std::shared_ptr<CalibrationManager> calman) { fCalMan = calman; }
        std::shared_ptr<CalibrationManager> GetCalMan() const { return fCalMan; }

        // Print configuration
        virtual void Print() const = 0;
        // Print reports
        virtual void PrintReports() const = 0;
    };
} // namespace ActRoot

#endif
