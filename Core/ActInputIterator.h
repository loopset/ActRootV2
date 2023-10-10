#ifndef ActInputIterator_h
#define ActInputIterator_h

#include "ActModularData.h"
#include "ActVData.h"

#include <map>
#include <utility>
namespace ActRoot
{
    //forward declaration
    class InputData;
    class InputIterator
    {
    private:
        int fCurrentRun;
        int fCurrentEntry;
        int fLastRun;
        std::map<int, int> fEntries;

    public:
        InputIterator() = default;
        InputIterator(const InputData* input);
        ~InputIterator() = default;

        bool Previous();
        bool Next();
        bool GoTo(int run, int entry);
        std::pair<int, int> GetCurrentRunEntry() const {return {fCurrentRun, fCurrentEntry};}
        bool HasRunChanged() const {return fLastRun != fCurrentRun;}

    private:
        bool CheckEntryIsInRange(int run, int entry);
    };

    //more forward declarations
    class TPCData;
    class SilData;
    class InputWrapper
    {
    private:
        InputData* fInput;
        InputIterator fIt;
        TPCData* fTPCData;
        SilData* fSilData;
        ModularData* fModularData;

    public:
        InputWrapper() = default;
        InputWrapper(InputData* input);
        ~InputWrapper() = default;

        bool GoNext();
        bool GoPrevious();
        bool GoTo(int run, int entry);

        //Getters
        TPCData* GetCurrentTPCData() const {return fTPCData;}
        SilData* GetCurrentSilData() const {return fSilData;}
        ModularData* GetCurrentModularData() const {return fModularData;}
        std::pair<int, int> GetCurrentStatus() const {return fIt.GetCurrentRunEntry();}
        const InputIterator& GetIt() const {return fIt;}
        InputData* GetInput() const {return fInput;}
        
    private:
        void SetBranchAddress(int run);
    };
}
#endif
