#ifndef ActVDetector_h
#define ActVDetector_h

#include "ActVData.h"
#include "TTree.h"
#include "ActTPCLegacyData.h"
#include "ActCalibrationManager.h"

#include <memory>
#include <string>

//Abstract class representing a detector
namespace ActRoot
{
    class InputBlock;//forward declaration
    //! Base class contaning all methods of an analysis detector
    class VDetector
    {
    protected:
        MEventReduced* fMEvent {};//!< Legacy dependency to MEvent... Could be changed in future
        std::shared_ptr<CalibrationManager> fCalMan {};//!< Pointer to CalibrationManager to avoid working with singleton
        
    public:
        VDetector() = default;
        virtual ~VDetector() = default;

        //Read configuration for each detector
        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) = 0;

        //Interface to CalibrationManager
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) = 0;
        //Initialize data input and output
        //With TTrees
        virtual void InitInputRawData(std::shared_ptr<TTree> tree, int run) = 0;
        virtual void InitInputData(std::shared_ptr<TTree> tree) = 0;
        virtual void InitOutputData(std::shared_ptr<TTree> tree) = 0;
        virtual void InitOutputPhysics(std::shared_ptr<TTree> tree) = 0;
        
        //Build events
        virtual void BuildEventData() = 0;
        virtual void BuildEventPhysics() = 0;

        //Clear data
        virtual void ClearEventData() = 0;
        virtual void ClearEventPhysics() = 0;

        //Getters
        virtual VData* GetEventData() const {return nullptr;}
        virtual VData* GetEventPhysics() const {return nullptr;}

        //Getters
        virtual void SetEventData(VData* vdata) = 0; 

        //Set and Get MEvent pointer
        void SetMEvent(MEventReduced* mevt){fMEvent = mevt;}
        MEventReduced* GetMEvent() const {return fMEvent;}

        //Set and Get CalibrationManager pointer
        void SetCalMan(std::shared_ptr<CalibrationManager> calman){fCalMan = calman;}
        std::shared_ptr<CalibrationManager> GetCalMan() const {return fCalMan;}
    };
}

#endif
