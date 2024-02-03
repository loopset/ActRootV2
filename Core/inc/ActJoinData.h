#ifndef ActJoinData_h
#define ActJoinData_h

#include "TChain.h"

#include <memory>
#include <string>
#include <vector>
namespace ActRoot
{
    class JoinData
    {
    private:
        std::vector<int> fRuns {};
        std::shared_ptr<TChain> fChain {};

    public:
        JoinData() = default;
        JoinData(const std::string& file);
        JoinData(const std::string& runfile, const std::string& pathfile);

        const std::shared_ptr<TChain>& operator->() const { return fChain; }
        std::shared_ptr<TChain> Get() const { return fChain; }
        const std::vector<int>& GetRunList() const { return fRuns; }

    private:
        void ReadFile(const std::string& file);
        void SetRunListFrom(const std::string& file);
    };
} // namespace ActRoot

#endif // !ActJoinData_h
