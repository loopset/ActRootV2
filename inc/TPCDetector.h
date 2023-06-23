#ifndef ActTPCDetector_h
#define ActTPCDetector_h

//#include "ActTPCData.h"
//#include "ActTPCLegacyData.h"
#include "VDetector.h"

#include <memory>
#include <string>
#include <vector>
namespace ActRoot
{
    class InputBlock;//forward declaration
    
    class TPCDetector : public ActRoot::VDetector
    {
    private:
        //Look up table
        std::vector<std::vector<int>> fLookUp;
        //Sizes and configurations
        int fNPADSX;
        int fNPADSY;
        int fNPADSZ;
        int fREBINZ;
        double fPadSide;
        //Data
        //MEventReduced* fMEvent;
        //ActTPCData* fData;

    public:
        TPCDetector()
        {
            //fMEvent = new MEventReduced;
            //fData = new ActTPCData;
        };
        virtual ~TPCDetector() = default;

        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        // void AddParameterToCalibrationManager() override;
        virtual void InitInputRawData() override;
        // void InitInputData() override;
        // void InitOutputData() override;
        // void InitOutputPhysics() override;
        // void BuildEventData() override;
        // void BuildEventPhysics() override;
        // void ClearEventData() override;
        // void ClearEventPhysics() override;
        ////////////////////////////////
        //ActTPCData* GetDataPointer() { return fData; }
    };
}

#endif
