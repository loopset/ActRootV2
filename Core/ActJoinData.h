#ifndef ActJoinData_h
#define ActJoinData_h

#include "TChain.h"

#include <memory>
#include <string>
namespace ActRoot
{
    class JoinData
    {
    private:
        std::shared_ptr<TChain> fChain {};

    public:
        JoinData() = default;
        JoinData(const std::string& file);

        const std::shared_ptr<TChain>& operator->() const { return fChain; }
        std::shared_ptr<TChain> Get() const {return fChain;}

    private:
        void ReadFile(const std::string& file);
    };
} // namespace ActRoot

#endif // !ActJoinData_h
