#include "ActCorrector.h"

#include "ActColors.h"
#include "ActGenCorrection.h"
#include "ActInputParser.h"
#include "ActOptions.h"
#include "ActPIDCorrector.h"

#include "TFile.h"

#include <memory>
#include <string>

void ActAlgorithm::Corrector::ReadConfiguration()
{
    auto conf {ActRoot::Options::GetInstance()->GetConfigDir()};
    conf += "corrector.conf";

    ActRoot::InputParser parser {conf};
    auto b {parser.GetBlock("Corrector")};
    fIsEnabled = b->GetBool("IsEnabled");
    if(!fIsEnabled)
        return;
    if(b->CheckTokenExists("PID", true))
        ReadPIDCorrector(b->GetString("PID"));
    if(b->CheckTokenExists("ZOffset", true))
        fZOffset = b->GetDouble("ZOffset");
    if(b->CheckTokenExists("Angle", true))
        ReadAngleCorrectors(b->GetStringVector("Angle"));
}

void ActAlgorithm::Corrector::ReadPIDCorrector(const std::string& file)
{
    auto f {std::make_unique<TFile>(file.c_str())};
    fPID = std::shared_ptr<ActPhysics::PIDCorrection>(f->Get<ActPhysics::PIDCorrection>("PIDCorrection"));
    if(!fPID)
        throw std::runtime_error("Corrector::ReadPIDCorrector: no PIDCorrection found in file " + file);
}

void ActAlgorithm::Corrector::ReadAngleCorrectors(const std::vector<std::string>& files)
{
    for(const auto& file : files)
    {
        ActPhysics::GenCorrection corr;
        corr.Read(file);
        fAngle[corr.GetName()] = corr;
    }
}

void ActAlgorithm::Corrector::Run()
{
    if(!fIsEnabled)
        return;
    DoPID();
    DoZOffset();
    DoAngle();
}

void ActAlgorithm::Corrector::DoPID()
{
    if(fPID)
    {
        // PIDCorrection must apply to layer saved in its fName
        // So we have to find it in the MergerData
        auto layer {fPID->GetName()};
        if(std::find(fMergerData->fSilLayers.begin(), fMergerData->fSilLayers.end(), layer) !=
           fMergerData->fSilLayers.end())
            fMergerData->fQave = fPID->Apply(fMergerData->fQave, fMergerData->fSP.Z());
    }
}

void ActAlgorithm::Corrector::DoZOffset()
{
    for(auto& p : {&fMergerData->fWP, &fMergerData->fRP, &fMergerData->fBP, &fMergerData->fSP})
        p->SetZ(p->Z() + fZOffset);
}

void ActAlgorithm::Corrector::DoAngle()
{
    if(!fMergerData->fSilLayers.size())
        return;
    // Which layer?
    auto layer {fMergerData->fSilLayers.front()};
    if(fAngle.count(layer))
    {
        auto& corr {fAngle[layer]};
        // 1-> RP.X() correction
        auto temp {fMergerData->fThetaLight + corr.Eval(0, fMergerData->fRP.X())};
        // 2-> Self correction
        fMergerData->fThetaLight = temp + corr.Eval(1, temp);
    }
    // // So far, it is hardcoded in this code until we
    // // recompute it
    // float theta {};
    // // First correction
    // theta = fMergerData->fThetaLight + (-2.14353 + 0.0114464 * fMergerData->fRP.X()) -
    //         8.52223E-5 * std::pow(fMergerData->fRP.X(), 2);
    // // Second correction
    // theta = theta + (-1.58175 + 0.0889058 * theta);
    // // Set
    // fMergerData->fThetaLight = theta;
}

void ActAlgorithm::Corrector::Print() const
{
    std::cout << BOLDCYAN << "++++ Corrector filter ++++" << '\n';
    if(fIsEnabled)
    {
        if(fPID)
            std::cout << "-> PIDCorr    : " << fPID->GetName() << '\n';
        std::cout << "-> ZOffset    : " << fZOffset << " mm" << '\n';
        if(fAngle.size())
        {
            std::cout << "-> AngleFuncs : " << '\n';
            for(const auto& [key, _] : fAngle)
                std::cout << "    " << key << '\n';
        }
    }
    std::cout << "+++++++++++++++++++++++++++" << RESET << '\n';
}
