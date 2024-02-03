#ifndef ActVData_h
#define ActVData_h

namespace ActRoot
{
    class VData
    {
    public:
        virtual void Clear() = 0;
        virtual void Print() const = 0;
    };
}

#endif
