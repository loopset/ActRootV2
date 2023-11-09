#include "ActMergerData.h"

#include "ActColors.h"

#include <iostream>

void ActRoot::MergerData::Clear()
{
    *this = {};
}

void ActRoot::MergerData::Print() const
{
    std::cout << BOLDGREEN << ":::: MergerData ::::" << '\n';
    std::cout << "-> RP : " << fRP << '\n';
    std::cout << "-> SP : " << fSP << '\n';
    std::cout << "::::::::::::::::::::" << RESET << '\n';
}
