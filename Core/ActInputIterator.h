#ifndef ActInputIterator_h
#define ActInputIterator_h

#include "ActVData.h"

#include <map>
#include <utility>
#include <vector>
namespace ActRoot
{
    // forward declaration
    class InputData;
    class InputIterator
    {
    private:
        int fCurrentRun;
        int fCurrentEntry;
        int fLastRun;
        std::map<int, int> fEntries;
        bool fIsManual {};
        std::map<int, std::vector<int>> fManual;
        int fManIdx;

    public:
        InputIterator() = default;
        InputIterator(const InputData* input);
        ~InputIterator() = default;

        bool Previous();
        bool Next();
        bool GoTo(int run, int entry);
        std::pair<int, int> GetCurrentRunEntry() const { return {fCurrentRun, fCurrentEntry}; }
        bool HasRunChanged() const { return fLastRun != fCurrentRun; }

    private:
        bool CheckEntryIsInRange(int run, int entry);
        std::pair<int, int> GetManualNext();
        std::pair<int, int> GetManualPrev();
    };

    // more forward declarations
    class TPCData;
    class SilData;
    class ModularData;
    class TPCPhysics;
    class MergerData;
    class InputWrapper
    {
    private:
        InputData* fInput {};
        InputIterator fIt;
        // Data
        TPCData* fTPCData {};
        SilData* fSilData {};
        ModularData* fModularData {};
        // Physics
        TPCPhysics* fTPCPhysics {};
        // Merger data
        MergerData* fMergerData {};

    public:
        InputWrapper() = default;
        InputWrapper(InputData* input);
        ~InputWrapper() = default;

        bool GoNext();
        bool GoPrevious();
        bool GoTo(int run, int entry);

        // Get current data
        TPCData* GetTPCData() const { return fTPCData; }
        SilData* GetSilData() const { return fSilData; }
        ModularData* GetModularData() const { return fModularData; }
        TPCPhysics* GetTPCPhysics() const { return fTPCPhysics; }
        MergerData* GetMergerData() const { return fMergerData; }
        std::pair<int, int> GetCurrentStatus() const { return fIt.GetCurrentRunEntry(); }
        const InputIterator& GetIt() const { return fIt; }
        InputData* GetInput() const { return fInput; }

        // Setters of data
        void SetTPCData(TPCData* data) { fTPCData = data; }
        void SetSilData(SilData* data) { fSilData = data; }
        void SetModularData(ModularData* data) { fModularData = data; }
        void SetTPCPhysics(TPCPhysics* data) { fTPCPhysics = data; }
        void SetMergerData(MergerData* data) { fMergerData = data; }

    private:
        void SetBranchAddress(int run);
    };
} // namespace ActRoot
#endif
