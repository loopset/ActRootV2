#ifndef ActTypes_h
#define ActTypes_h
namespace ActRoot
{
enum class DetectorType
{
    EActar,
    ESilicons,
    EModular,
    EMerger
};

enum class ModeType
{
    ECluster,
    EData,
    EFilter,
    EMerger,
    ECorrect,
    ENone
};

} // namespace ActRoot
#endif // !ActTypes_h
