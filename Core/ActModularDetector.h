#ifndef ActModularDetector_h
#define ActModularDetector_h

#include "ActModularData.h"
#include "ActVDetector.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace ActRoot
{
    //! A class holding ModularLeaf experimental setups: VXI equivalences
    class ModularParameters
    {
    private:
        std::map<int, std::string> fVXI;
    public:
        std::string GetName(int vxi);//!< Get name of ModularLeaf according to Action file
        void ReadActions(const std::vector<std::string>& names,
                         const std::string& file); //!< Read Action file
        void Print() const;
    };

    //!< Detector of type ModularLeaf
    class ModularDetector : public VDetector
    {
    private:
        //Parameters
        ModularParameters fPars;//!< Basic detector configurations
        //Data
        ModularData* fData {};//!< Pointer to Data

    public:
        ModularDetector() = default;
        virtual ~ModularDetector() = default;

        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        virtual void InitInputRawData(std::shared_ptr<TTree> tree, int run) override;
        virtual void InitOutputData(std::shared_ptr<TTree> tree) override;
        virtual void BuildEventData() override;
        virtual void ClearEventData() override;
    };
}

#endif
