#include "ActMergerData.h"

#include "ActColors.h"
#include "TGraph.h"
#include "TH1.h"

#include <algorithm>
#include <iostream>

void ActRoot::MergerData::Clear()
{
    *this = MergerData {};
}

void ActRoot::MergerData::Print() const
{
    std::cout << BOLDGREEN << ":::: MergerData ::::" << '\n';
    std::cout << "-> RP : " << fRP << '\n';
    std::cout << "-> SP : " << fSP << '\n';
    std::cout << "-> Silicons : " << '\n';
    for(int i = 0; i < fSilLayers.size(); i++)
        std::cout << "   layer : " << fSilLayers[i] << " idx : " << fSilNs[i] << " E : " << fSilEs[i] << " MeV" << '\n';
    std::cout << "::::::::::::::::::::" << RESET << '\n';
}