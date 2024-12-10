#include "ActGenCorrection.h"

#include "ActColors.h"

#include "TF1.h"

#include <iostream>
#include <memory>

void ActPhysics::GenCorrection::Add(TF1* f)
{
    fCorrs.push_back(std::make_shared<TF1>(*f));
}

double ActPhysics::GenCorrection::Apply(double x)
{
    double ret {x};
    for(const auto& f : fCorrs)
        ret = f->Eval(ret);
    return ret;
}

void ActPhysics::GenCorrection::Print() const
{
    std::cout << BOLDCYAN << "····· GenCorrection ·····" << '\n';
    std::cout << "-> Name : " << fName << '\n';
    std::cout << "-> Funcs : " << '\n';
    for(const auto& f : fCorrs)
        std::cout << "    " << f->GetName() << '\n';
    std::cout << "······························" << RESET << '\n';
}
