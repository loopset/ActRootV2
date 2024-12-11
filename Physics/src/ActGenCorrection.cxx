#include "ActGenCorrection.h"

#include "ActColors.h"

#include "TF1.h"
#include "TFile.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

void ActPhysics::GenCorrection::Add(TF1* f)
{
    fKeys.push_back(f->GetName());
    fFuncs.push_back(std::make_shared<TF1>(*f));
}

void ActPhysics::GenCorrection::Add(const std::string& key, TF1* f)
{
    fKeys.push_back(key);
    fFuncs.push_back(std::make_shared<TF1>(*f));
    fFuncs.back()->SetName(key.c_str());
}

void ActPhysics::GenCorrection::Read(const std::string& file)
{
    auto f {std::make_unique<TFile>(file.c_str())};
    fName = *(f->Get<std::string>("Name"));
    fKeys = *(f->Get<std::vector<std::string>>("Keys"));
    for(const auto& key : fKeys)
        fFuncs.push_back(std::shared_ptr<TF1>(f->Get<TF1>(key.c_str())));
}

std::shared_ptr<TF1> ActPhysics::GenCorrection::Get(const std::string& key)
{
    auto it {std::find(fKeys.begin(), fKeys.end(), key)};
    if(it == fKeys.end())
        return nullptr;
    auto idx {std::distance(fKeys.begin(), it)};
    return fFuncs[idx];
}

double ActPhysics::GenCorrection::Apply(double x)
{
    double ret {x};
    for(const auto& f : fFuncs)
        ret = f->Eval(ret);
    return ret;
}

void ActPhysics::GenCorrection::Write(const std::string& file)
{
    auto f {std::make_unique<TFile>(file.c_str(), "recreate")};
    f->WriteObject(&fName, "Name");
    f->WriteObject(&fKeys, "Keys");
    for(int i = 0; i < fKeys.size(); i++)
        fFuncs[i]->Write();
}

void ActPhysics::GenCorrection::Print(bool verbose) const
{
    std::cout << BOLDCYAN << "····· GenCorrection ·····" << '\n';
    std::cout << "-> Name  : " << fName << '\n';
    std::cout << "-> Funcs : " << '\n';
    for(const auto& f : fFuncs)
    {
        f->Print(verbose ? "v" : "");
        std::cout << "--------------------" << '\n';
    }
    std::cout << "······························" << RESET << '\n';
}
