#ifndef ActTPCDetector_h
#define ActTPCDetector_h

#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "ActInputData.h"
#include "TTree.h"
#include "ActVDetector.h"

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
        int fNB_AGET {4};
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

    class SilDetector;//forward declaration
    class TPCDetector : public ActRoot::VDetector
    {
    private:
        //Parameters of detector
        TPCParameters fPars;
        //Data
        TPCData* fData {};
        int fCurrentRun;

    public:
        TPCDetector() = default;
        virtual ~TPCDetector() = default;

        //Getters
        const TPCParameters& GetTPCPars() const {return fPars;}

        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        // void AddParameterToCalibrationManager() override;
        virtual void InitInputRawData(std::shared_ptr<TTree> tree, int run) override;
        // void InitInputData() override;
        virtual void InitOutputData(std::shared_ptr<TTree> tree) override;
        // void InitOutputPhysics() override;
        virtual void BuildEventData() override;
        // void BuildEventPhysics() override;
        void ClearEventData() override;
        // void ClearEventPhysics() override;
        ////////////////////////////////
        //ActTPCData* GetDataPointer() { return fData; }
        
    private:
        void ReadHits(ReducedData& coas, const int& where, int& hitID);
    };
}

#endif
