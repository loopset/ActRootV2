#ifndef ActVData_h
#define ActVData_h
#include "Rtypes.h"
namespace ActRoot
{
class VData
{
public:
    VData() = default;
    virtual ~VData() = default;

    virtual void Clear() = 0;
    virtual void Print() const = 0;

    template <typename T>
    inline T* CastAs()
    {
        return dynamic_cast<T*>(this);
    }
    ClassDef(VData, 1);
};
} // namespace ActRoot

#endif
