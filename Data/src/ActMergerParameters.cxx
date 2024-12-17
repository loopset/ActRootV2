#include "ActMergerParameters.h"

#include "ActColors.h"

#include <iostream>

void ActRoot::MergerParameters::Print() const
{
    std::cout << BOLDYELLOW << ":::: MergerParameters ::::" << '\n';
    std::cout << "-> UseRP ? " << std::boolalpha << fUseRP << '\n';
    std::cout << "-> IsL1  ? " << std::boolalpha << fIsL1 << '\n';
    std::cout << "-> IsCal ? " << std::boolalpha << fIsCal << '\n';
    std::cout << "::::::::::::::::::::::::::::::" << RESET << '\n';
}
