#include "ActCorrDetector.h"

#include "ActColors.h"
#include "ActInputParser.h"
#include "ActMergerData.h"
#include "ActPIDCorrector.h"

#include "TEnv.h"
#include "TFile.h"
#include "TSystem.h"
#include "TTree.h"

#include <cmath>
#include <ios>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

void ActRoot::CorrDetector::ReadConfiguration()
{
    // Conf file for this detector is hardcoded in configs/ dir
    std::string file {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    file += "/configs/corrections.conf";
    if(gSystem->AccessPathName(file.c_str()))
        std::cout << BOLDMAGENTA << "corrections.conf config file was not found !" << RESET << '\n';
    // // Parse
    InputParser parser {file};
    auto b {parser.GetBlock("Corrections")};
    if(b->CheckTokenExists("PIDFile"))
        ReadPIDFile(b->GetString("PIDFile"));
    if(b->CheckTokenExists("ZOffset"))
        fZOffset = b->GetDouble("ZOffset");
    if(b->CheckTokenExists("EnableThetaCorr"))
        fEnableThetaCorr = b->GetBool("EnableThetaCorr");
}

void ActRoot::CorrDetector::ReadPIDFile(const std::string& file)
{
    auto f {std::make_unique<TFile>(file.c_str())};
    auto o {f->Get<ActPhysics::PIDCorrection>("PIDCorrection")};
    if(!o)
        throw std::runtime_error("CorrDetector::ReadPIDFile: no PIDCorrection found in file " + file);
    fPIDCorr = dynamic_cast<ActPhysics::PIDCorrection*>(o);
}

void ActRoot::CorrDetector::InitInputCorr(std::shared_ptr<TTree> tree)
{
    if(fIn)
        delete fIn;
    fIn = new MergerData;
    tree->SetBranchAddress("MergerData", &fIn);
}

void ActRoot::CorrDetector::InitOutputCorr(std::shared_ptr<TTree> tree)
{
    if(fOut)
        delete fOut;
    fOut = new MergerData;
    tree->Branch("MergerData", &fOut);
}

void ActRoot::CorrDetector::MoveZ(XYZPoint& point)
{
    point.SetZ(point.Z() + fZOffset);
}

void ActRoot::CorrDetector::CorrectAngle()
{
    // So far, it is hardcoded in this code until we
    // recompute it
    float theta {};
    // First correction
    theta = fOut->fThetaLight + (-2.14353 + 0.0114464 * fOut->fRP.X()) - 8.52223E-5 * std::pow(fOut->fRP.X(), 2);
    // Second correction
    theta = theta + (-1.58175 + 0.0889058 * theta);
    // Set
    fOut->fThetaLight = theta;
}

void ActRoot::CorrDetector::BuildEventCorr()
{
    // Copy data pointed by pointers
    *fOut = *fIn;
    // Correct charge here, using legacy Z value
    fOut->fQave = fPIDCorr->Apply(fIn->fQave, fIn->fSP.Z());
    // Move points in Z!
    // 1-> Reaction point
    MoveZ(fOut->fRP);
    // 2-> Silicon point
    MoveZ(fOut->fSP);
    // 3-> Boundary point
    MoveZ(fOut->fBP);
    // Correct theta if asked
    if(fEnableThetaCorr)
        CorrectAngle();
}

void ActRoot::CorrDetector::Print() const
{
    std::cout << BOLDCYAN << "++++ Corr detector ++++" << '\n';
    if(fPIDCorr)
        std::cout << "-> PIDCorr   : " << fPIDCorr->GetName() << '\n';
    std::cout << "-> ZOffset   : " << fZOffset << " mm" << '\n';
    std::cout << "-> ThetaCorr ? " << std::boolalpha << fEnableThetaCorr << '\n';
    std::cout << "+++++++++++++++++++++++++++" << RESET << '\n';
}
