#ifndef ActMergerDetector_h
#define ActMergerDetector_h

#include "ActModularData.h"
#include "ActSilData.h"
#include "ActTPCPhysics.h"

#include "TTree.h"

#include <memory>

namespace ActRoot
{
    // forward declarations
    class TPCParameters;
    class SilParameters;
    class ModularParameters;

    class MergerDetector
    {
    private:
        TPCParameters* fTPCPars {};
        TPCPhysics* fTPCPhyiscs {};

        SilParameters* fSilPars {};
        SilData* fSilData {};

        ModularParameters* fModularPars {};
        ModularData* fModularData {};

    public:
        // Setters of pointer to Parameters in DetMan
        void SetTPCParameters(TPCParameters* tpcPars) { fTPCPars = tpcPars; }
        void SetSilParameters(SilParameters* silPars) { fSilPars = silPars; }
        void SetModularParameters(ModularParameters* modPars) { fModularPars = modPars; }
        
        // Init INPUT data
        void InitInputMerger(std::shared_ptr<TTree> tree);

        // Init OUTPUT data
        void InitOutputMerger(std::shared_ptr<TTree> tree);

        // Do merge of all detector data
        void MergeEvent();
    };
} // namespace ActRoot

#endif // !ActMergerDetector_h
