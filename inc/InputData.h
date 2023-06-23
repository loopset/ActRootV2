#ifndef ActInputData_h
#define ActInputData_h

#include "TChain.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace ActRoot
{
    class InputData
    {
    private:
        std::shared_ptr<TChain> fChain;
        std::string fTreeName;
    public:
        InputData();
        InputData(const std::string& treeName);
        ~InputData() {};

        void SetTreeName(const std::string& treeName);
        void AddFile(const std::string& file);
        std::shared_ptr<TChain> GetChain() { return fChain; }
    };
}
#endif
