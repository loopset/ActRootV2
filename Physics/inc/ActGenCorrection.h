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
    std::vector<std::string> fKeys {};
    std::vector<std::shared_ptr<TF1>> fFuncs {};

public:
    GenCorrection() = default;
    GenCorrection(const std::string& name) : fName(name) {}

    void Add(TF1* f);
    void Add(const std::string& key, TF1* f);
    void Read(const std::string& file);

    const std::string& GetName() const { return fName; }
    std::shared_ptr<TF1> Get(int idx) { return fFuncs[idx]; }
    std::shared_ptr<TF1> Get(const std::string& key);
    void Write(const std::string& file);

    double Eval(int idx, double x) { return fFuncs[idx]->Eval(x); }
    double Eval(const std::string& key, double x) { return Get(key)->Eval(x); }
    double Apply(double x);
    void Print(bool verbose = false) const;
};
} // namespace ActPhysics

#endif
