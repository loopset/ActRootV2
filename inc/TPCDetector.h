#ifndef ActTPCDetector_h
#define ActTPCDetector_h

#include "TPCData.h"
#include "TPCLegacyData.h"
#include "InputData.h"
#include "VDetector.h"

#include <memory>
#include <string>
#include <vector>
namespace ActRoot
{
    class InputBlock;//forward declaration

    class TPCParameters
    {
    private:
        double fPadSide {2};//mm
        int fNPADSX;
        int fNPADSY;
        int fNPADSZ;
        int fREBINZ {1};
        int fNB_COBO {18};
        int fNB_ASAD {4};
        int fNB_AGET {5};
        int fNB_CHANNEL {68};

    public:
        TPCParameters() = default;
        TPCParameters(const std::string& type);
        //Setters
        void SetREBINZ(int rebin) {fREBINZ = rebin;}
        //Getters
        double GetPadSide() const {return fPadSide;}
        int GetNPADSX() const {return fNPADSX;}
        int GetNPADSY() const {return fNPADSY;}
        int GetNPADSZ() const {return fNPADSZ;}
        int GetREBINZ() const {return fREBINZ;}
        int GetNBCOBO() const {return fNB_COBO;}
        int GetNBASAD() const {return fNB_ASAD;}
        int GetNBAGET() const {return fNB_AGET;}
        int GetNBCHANNEL() const {return fNB_CHANNEL;}
    };
    
    class TPCDetector : public ActRoot::VDetector
    {
    private:
        //Parameters of detector
        TPCParameters fPars;
        //Data
        MEventReduced* fMEvent {};
        TPCData* fData {};
        int fCurrentRun;

    public:
        TPCDetector()
        {
            //fMEvent = new MEventReduced;
            //fData = new ActTPCData;
        };
        virtual ~TPCDetector() = default;

        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        // void AddParameterToCalibrationManager() override;
        virtual void InitInputRawData(std::shared_ptr<TTree> tree, int run) override;
        // void InitInputData() override;
        virtual void InitOutputData() override;
        // void InitOutputPhysics() override;
        virtual void BuildEventData() override;
        // void BuildEventPhysics() override;
        // void ClearEventData() override;
        // void ClearEventPhysics() override;
        ////////////////////////////////
        //ActTPCData* GetDataPointer() { return fData; }
    };
}

#endif
