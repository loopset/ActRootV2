#ifndef ActTypes_h
#define ActTypes_h
namespace ActRoot
{
enum class DetectorType
{
    EActar,
    ESilicons,
    EModular,
    EMerger,
    ENone
};

enum class ModeType
{
    EReadTPC,    // !< Convert from Raw to TPCData
    EReadSilMod, // !< Convert from Raw to Sil and Modular data
    EFilter,     // !< Exec filter before Merger
    EMerge,      // !< Merge detectors and build physical event
    EFilterMerge,// !< Both operations at the same time
    ECorrect,    // !< Exec filter after merger
    EGui,     // !< For GUI in actplot
    ENone        // !< Default value
};

} // namespace ActRoot
#endif // !ActTypes_h
