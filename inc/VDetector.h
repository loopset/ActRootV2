#ifndef ActVDetector_h
#define ActVDetector_h

#include <memory>
#include <string>

//Abstract class representing a detector
namespace ActRoot
{
    class InputBlock;//forward declaration
    
    class VDetector
    {
    public:
        VDetector() = default;
        virtual ~VDetector() = default;

        //Read configuration for each detector
        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) = 0;

        // //Set calibrations
        // virtual void AddParameterToCalibrationManager(){};

        // //Initialize data input and output
        virtual void InitInputRawData() = 0;
        // virtual void InitInputData(){};
        // virtual void InitOutputData(){};
        // virtual void InitOutputPhysics(){};

        // //Build events
        // virtual void BuildEventData(){};
        // virtual void BuildEventPhysics(){};

        // //Clear data
        // virtual void ClearEventData() {};
        // virtual void ClearEventPhysics() {};
    };
}

#endif
