#ifndef ActSilDetector_h
#define ActSilDetector_h

#include "ActSilData.h"
#include "ActTPCLegacyData.h"
#include "ActVDetector.h"
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ActRoot
{
    //! A class holding basic silicon parameters
    /*!
      For now, just keeps sizes of silicons 
      using strings as identifiers and VXI equivalences
    */
    class SilParameters
    {
    private:
        std::unordered_map<std::string, int> fSizes; //!< Sizes of silicons based on string, for [0, max]
        std::map<int, std::pair<std::string, int>> fVXI;
    public:
        //Setters
        void SetLayer(const std::string& key, int size){fSizes[key] = size;}
        //Getters
        std::vector<std::string> GetKeys() const;
        int GetSizeOf(const std::string& key){return fSizes[key];}
        void Print() const;//!< Dump info stored
        std::pair<std::string, int> GetSilIndex(int vxi);
        void ReadActions(const std::vector<std::string>& layers,
                         const std::vector<std::string>& names,
                         const std::string& file);
    };
    
    //! Silicon detector class
    class SilDetector : public VDetector
    {
    private:
        //Parameters
        SilParameters fPars;//!< Basic detector configurations
        //Data
        SilData* fData {}; //!< Pointer to SilData
        
    public:
        SilDetector() = default;
        virtual ~SilDetector() = default;

        virtual void ReadConfiguration(std::shared_ptr<InputBlock> config) override;
        virtual void ReadCalibrations(std::shared_ptr<InputBlock> config) override;
        // void AddParameterToCalibrationManager() override;
        virtual void InitInputRawData(std::shared_ptr<TTree> tree, int run) override;
        // void InitInputData() override;
        virtual void InitOutputData(std::shared_ptr<TTree> tree) override;
        // void InitOutputPhysics() override;
        virtual void BuildEventData() override;
        // void BuildEventPhysics() override;
        virtual void ClearEventData() override;

    private:
        SilData* GetSilDataPointer() const {return fData;}
    };
}

#endif
