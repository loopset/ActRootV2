#ifndef ActInputIterator_h
#define ActInputIterator_h

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
        std::map<int, int> fEntries;

    public:
        InputIterator() = default;
        InputIterator(const InputData* input);
        ~InputIterator() = default;

        bool Previous();
        bool Next();
        bool GoTo(int run, int entry);
        std::pair<int, int> GetCurrentRunEntry() const {return {fCurrentRun, fCurrentEntry};}

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

    public:
        InputWrapper() = default;
        InputWrapper(InputData* input);
        ~InputWrapper() = default;

        bool GoNext();
        bool GoPrevious();
        bool GoTo(int run, int entry);
        TPCData* GetCurrentTPCData() {return fTPCData;}
        SilData* GetCurrentSilData() {return fSilData;}
        std::pair<int, int> GetCurrentStatus() const {return fIt.GetCurrentRunEntry();}

    private:
        void SetBranchAddress(int run);
    };
}
#endif
