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
    ECorrections,
    ENone
};

enum class ModeType
{
    ECluster,
    EData,
    EFilter,
    EMerge,
    ECorrect,
    EVisual, // for visual verbose in actplot
    ENone
};

} // namespace ActRoot
#endif // !ActTypes_h
