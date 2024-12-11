#ifndef ActCorrector_h
#define ActCorrector_h

#include "ActGenCorrection.h"
#include "ActPIDCorrector.h"
#include "ActVFilter.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ActAlgorithm
{
class Corrector : public VFilter
{
private:
    bool fIsEnabled {};
    std::shared_ptr<ActPhysics::PIDCorrection> fPID {};
    double fZOffset {};
    std::unordered_map<std::string, ActPhysics::GenCorrection> fAngle {};

public:
    Corrector() = default;
    ~Corrector() override = default;

    void ReadConfiguration() override;

    void Run() override;

    void Print() const override;

    void PrintReports() const override {}

private:
    void ReadPIDCorrector(const std::string& file);
    void ReadAngleCorrectors(const std::vector<std::string>& files);
    void DoPID();
    void DoZOffset();
    void DoAngle();
};
}; // namespace ActAlgorithm

#endif // !ActCorrector_h
