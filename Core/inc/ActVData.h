#ifndef ActVData_h
#define ActVData_h

namespace ActRoot
{
class VData
{
public:
    virtual void Clear() = 0;
    virtual void Print() const = 0;

    template <typename T>
    inline T* CastAs()
    {
        return dynamic_cast<T*>(this);
    }
};
} // namespace ActRoot

#endif
