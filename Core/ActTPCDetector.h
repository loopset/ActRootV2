#ifndef ActTPCDetector_h
#define ActTPCDetector_h

#include "ActTPCData.h"
#include "ActTPCLegacyData.h"
#include "ActInputData.h"
#include "ActTPCPhysics.h"
#include "ActVDetector.h"

#include "TTree.h"

#include <memory>
#include <string>
#include <map>
#include <utility>
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

    class TPCDetector : public ActRoot::VDetector
    {
    private:
        //Parameters of detector
        TPCParameters fPars;
        //Data
        TPCData* fData {};
        //Preanalysis when reading raw data 
        bool fCleanSaturatedVoxels {false};
        bool fCleanPadMatrix {false};
        double fMinTBtoDelete {20};
        double fMinQtoDelete  {2000};
        std::map<std::pair<int, int>,
                 std::pair<std::vector<unsigned int>, double>> fPadMatrix;
        //Physics data
        TPCPhysics* fPhysics {};
        
    public:
        TPCDetector() = default;
        virtual ~TPCDetector() = default;

        //Getters
        const TPCParameters& GetTPCPars() const {return fPars;}

        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        // void AddParameterToCalibrationManager() override;
        virtual void InitInputRawData(std::shared_ptr<TTree> tree, int run) override;
        virtual void InitInputData(std::shared_ptr<TTree> tree) override;
        virtual void InitOutputData(std::shared_ptr<TTree> tree) override;
        virtual void InitOutputPhysics(std::shared_ptr<TTree> tree) override;
        virtual void BuildEventData() override;
        virtual void BuildEventPhysics() override;
        virtual void ClearEventData() override;
        virtual void ClearEventPhysics() override;
        ////////////////////////////////
        //ActTPCData* GetDataPointer() { return fData; }
        
    private:
        void ReadHits(ReducedData& coas, const int& where, int& hitID);
        void CleanPadMatrix();
    };
}

#endif
