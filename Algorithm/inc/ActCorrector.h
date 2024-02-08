#ifndef ActCorrector_h
#define ActCorrector_h

#include "ActPIDCorrector.h"
#include "ActVFilter.h"

#include <string>

namespace ActAlgorithm
{
class Corrector : public VFilter
{
private:
    std::shared_ptr<ActPhysics::PIDCorrection> fPID {};
    double fZOffset {};
    bool fEnableAngle {};

public:
    Corrector() = default;
    ~Corrector() override = default;

    void ReadConfiguration() override;

    void Run() override;

    void Print() const override;

    void PrintReports() const override {}

private:
    void ReadPIDCorrector(const std::string& file);
    void DoPID();
    void DoZOffset();
    void DoAngle();
};
}; // namespace ActCluster

#endif // !ActCorrector_h
