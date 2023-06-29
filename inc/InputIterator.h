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
        std::map<int, int>::iterator fRunIt;
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
    class InputWrapper
    {
    private:
        InputData* fInput;
        InputIterator fIt;
        TPCData* fData;

    public:
        InputWrapper() = default;
        InputWrapper(InputData* input);
        ~InputWrapper() = default;

        void GoNext();
        void GoPrevious();
        void GoTo(int run, int entry);
        TPCData* GetCurrentData() {return fData;}
        std::pair<int, int> GetCurrentStatus() const {return fIt.GetCurrentRunEntry();}
    };
}
#endif
