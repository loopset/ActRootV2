#ifndef ActGenCorrection_h
#define ActGenCorrection_h

#include "TF1.h"

#include <memory>
#include <string>
#include <vector>
namespace ActPhysics
{
//! General class consisting of a vector of TF1s
// that is used to apply sequentially a correction to a variable
// As the angle correction for E796
class GenCorrection
{
    std::string fName {};
    std::vector<std::shared_ptr<TF1>> fCorrs {};

public:
    GenCorrection(const std::string& name) : fName(name) {}

    void Add(TF1* f);
    std::shared_ptr<TF1> Get(int idx) { return fCorrs[idx]; }
    double Apply(double x);
    void Print() const;
};
} // namespace ActPhysics

#endif
