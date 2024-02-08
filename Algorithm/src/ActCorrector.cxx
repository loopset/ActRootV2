#include "ActCorrector.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActPIDCorrector.h"

#include "TFile.h"

#include <memory>
#include <string>

void ActCluster::Corrector::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "corrector.conf";

    ActRoot::InputParser parser {conf};
    auto b {parser.GetBlock("Corrector")};
    if(b->CheckTokenExists("PID", true))
        ReadPIDCorrector(b->GetString("PID"));
    if(b->CheckTokenExists("ZOffset", true))
        fZOffset = b->GetDouble("ZOffset");
    if(b->CheckTokenExists("EnableAngle"))
        fEnableAngle = b->GetDouble("EnableAngle");
}

void ActCluster::Corrector::ReadPIDCorrector(const std::string& file)
{
    auto f {std::make_unique<TFile>(file.c_str())};
    auto o {std::shared_ptr<ActPhysics::PIDCorrection>(f->Get<ActPhysics::PIDCorrection>("PIDCorrection"))};
    if(!o)
        throw std::runtime_error("Corrector::ReadPIDCorrector: no PIDCorrection found in file " + file);
}

void ActCluster::Corrector::Run()
{
    DoPID();
    DoZOffset();
    DoAngle();
}

void ActCluster::Corrector::DoPID()
{
    if(fPID)
        fMergerData->fQave = fPID->Apply(fMergerData->fQave, fMergerData->fRP.X());
}

void ActCluster::Corrector::DoZOffset()
{
    for(auto& p : {&fMergerData->fWP, &fMergerData->fRP, &fMergerData->fBP, &fMergerData->fSP})
        p->SetZ(p->Z() + fZOffset);
}

void ActCluster::Corrector::DoAngle()
{
    // So far, it is hardcoded in this code until we
    // recompute it
    float theta {};
    // First correction
    theta = fMergerData->fThetaLight + (-2.14353 + 0.0114464 * fMergerData->fRP.X()) -
            8.52223E-5 * std::pow(fMergerData->fRP.X(), 2);
    // Second correction
    theta = theta + (-1.58175 + 0.0889058 * theta);
    // Set
    fMergerData->fThetaLight = theta;
}

void ActCluster::Corrector::Print() const
{
    std::cout << BOLDCYAN << "++++ Corrector filter ++++" << '\n';
    if(fPID)
        std::cout << "-> PIDCorr   : " << fPID->GetName() << '\n';
    std::cout << "-> ZOffset   : " << fZOffset << " mm" << '\n';
    std::cout << "-> ThetaCorr ? " << std::boolalpha << fEnableAngle << '\n';
    std::cout << "+++++++++++++++++++++++++++" << RESET << '\n';
}
