#ifndef ActMergerDetector_h
#define ActMergerDetector_h

#include "ActModularData.h"
#include "ActSilData.h"
#include "ActTPCPhysics.h"
#include "ActSilSpecs.h"

#include "TTree.h"

#include <memory>
#include <string>
#include <vector>

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
        std::shared_ptr<ActPhysics::SilSpecs> fSilSpecs {};

        ModularParameters* fModularPars {};
        ModularData* fModularData {};

        // Parameters of algorithm
    public:
        // Setters of pointer to Parameters in DetMan
        void SetTPCParameters(TPCParameters* tpcPars) { fTPCPars = tpcPars; }
        void SetSilParameters(SilParameters* silPars) { fSilPars = silPars; }
        void SetModularParameters(ModularParameters* modPars) { fModularPars = modPars; }
        
        // Read 
        void ReadSilSpecs(const std::string& file);
        
        // Init INPUT data
        void InitInputMerger(std::shared_ptr<TTree> tree);

        // Init OUTPUT data
        void InitOutputMerger(std::shared_ptr<TTree> tree);

        // Do merge of all detector data
        void MergeEvent();

    private:
        void ComputeSiliconPoint();
    };
} // namespace ActRoot

#endif // !ActMergerDetector_h
