#ifndef ActVDetector_h
#define ActVDetector_h

#include "TTree.h"
#include "TPCLegacyData.h"

#include <memory>
#include <string>

//Abstract class representing a detector
namespace ActRoot
{
    class InputBlock;//forward declaration   
    class VDetector
    {
    protected:
        MEventReduced* fMEvent {};
    public:
        VDetector() = default;
        virtual ~VDetector() = default;

        //Read configuration for each detector
        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) = 0;

        // //Set calibrations
        // virtual void AddParameterToCalibrationManager(){};

        //Interface to CalibrationManager
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) = 0;
        // //Initialize data input and output
        virtual void InitInputRawData(std::shared_ptr<TTree> tree, int run) = 0;
        // virtual void InitInputData(){};
        virtual void InitOutputData(std::shared_ptr<TTree> tree) = 0;
        // virtual void InitOutputPhysics(){};

        // //Build events
        virtual void BuildEventData() = 0;
        // virtual void BuildEventPhysics(){};

        // //Clear data
        virtual void ClearEventData() = 0;
        // virtual void ClearEventPhysics() {};

        void SetMEvent(MEventReduced* mevt){fMEvent = mevt;}
        MEventReduced* GetMEvent() const {return fMEvent;}
    };
}

#endif
