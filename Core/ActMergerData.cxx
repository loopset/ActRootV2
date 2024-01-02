#include "ActMergerData.h"

#include "ActColors.h"

#include "TGraph.h"
#include "TH1.h"

#include <algorithm>
#include <iostream>

void ActRoot::MergerData::Clear()
{
    // Reset but still keep run and entry number
    auto run {fRun};
    auto entry {fEntry};
    *this = MergerData {};
    fRun = run;
    fEntry = entry;
}

void ActRoot::MergerData::Print() const
{
    std::cout << BOLDGREEN << ":::: MergerData ::::" << '\n';
    std::cout << "-> RP : " << fRP << '\n';
    std::cout << "-> SP : " << fSP << '\n';
    std::cout << "-> Silicons : " << '\n';
    for(int i = 0; i < fSilLayers.size(); i++)
        std::cout << "   layer : " << fSilLayers[i] << " idx : " << fSilNs[i] << " E : " << fSilEs[i] << " MeV" << '\n';
    std::cout << "-> ThetaBeam  : " << fThetaBeam << '\n';
    std::cout << "-> Theta      : " << fThetaLight << '\n';
    std::cout << "-> ThetaDebug : " << fThetaDebug << '\n';
    std::cout << "-> Phi        : " << fPhiLight << '\n';
    std::cout << "-> Qave       : " << fQave << '\n';
    std::cout << "-> TL         : " << fTrackLength << '\n';
    std::cout << "::::::::::::::::::::" << RESET << '\n';
}
