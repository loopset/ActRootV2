#ifndef ActMergerParameters_h
#define ActMergerParameters_h

#include "ActVDetector.h"

#include <string>
namespace ActRoot
{
enum class MergerMode
{
    ENone,        //!< This mode does nothing
    ECalibration, //!< Mode only for calibration purpouses: just with a 3alpha track hitting a silicon
    EAutomatic,   //!< Automatically infer the type of event to treat
};

std::string MergerModeToStr(MergerMode mode);

MergerMode StrToMergerMode(const std::string& str);

enum class MergerEvent
{
    EBeam,    //!< Only one-beam event
    EBinary,  //!< Most interesting events: (in)elastic, transfer, ...
    EMulti,   //!< Events with more than two recoils
    EUnknown, //!< Not know events
};

class MergerParameters : public VParameters
{
public:
    // Just flags setting event-by-event merger settings
    bool fUseRP {};
    bool fIsL1 {};
    bool fIsCal {};

    void Print() const override;
};
} // namespace ActRoot

#endif
