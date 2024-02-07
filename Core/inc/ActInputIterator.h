#ifndef ActInputIterator_h
#define ActInputIterator_h

#include <map>
#include <string>
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
    void Print() const;

private:
    bool CheckEntryIsInRange(int run, int entry);
    std::pair<int, int> GetManualNext();
    std::pair<int, int> GetManualPrev();
};

// more forward declarations
class TPCData;
class SilData;
class ModularData;
class MergerData;
class InputWrapper
{
private:
    InputData* fInput {};
    InputIterator fIt;
    // Data
    TPCData* fTPCData {};
    TPCData* fTPCClone {};
    TPCData* fTPCClone2 {};
    SilData* fSilData {};
    ModularData* fModularData {};
    // Merger data
    MergerData* fMergerData {};

public:
    InputWrapper() = default;
    InputWrapper(InputData* input);
    ~InputWrapper();

    bool GoNext();
    bool GoPrevious();
    bool GoTo(int run, int entry);
    void GetEntry(int run, int entry);
    void ReGet();

    // Get current data
    TPCData* GetTPCData() const { return fTPCData; }
    TPCData* GetTPCDataClone() const { return fTPCClone; };
    TPCData* GetTPCDataClone2() const { return fTPCClone2; }
    SilData* GetSilData() const { return fSilData; }
    ModularData* GetModularData() const { return fModularData; }
    MergerData* GetMergerData() const { return fMergerData; }
    std::pair<int, int> GetCurrentStatus() const { return fIt.GetCurrentRunEntry(); }
    const InputIterator& GetIt() const { return fIt; }
    // InputData* GetInput() const { return fInput; }

    // Setters of data
    void SetTPCData(TPCData* data) { fTPCData = data; }
    void SetTPCDataClone(TPCData* data) { fTPCClone = data; }
    void CopyToClone2(TPCData* data);
    void SetSilData(SilData* data) { fSilData = data; }
    void SetModularData(ModularData* data) { fModularData = data; }
    void SetMergerData(MergerData* data) { fMergerData = data; }

private:
    void SetBranchAddress(int run);
};
} // namespace ActRoot
#endif
