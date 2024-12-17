#include "ActTPCParameters.h"

#include "ActColors.h"

#include <iostream>
#include <stdexcept>

void ActRoot::TPCParameters::SetREBINZ(int rebin)
{
    if((rebin != 1 && (rebin % 2)) || rebin == 0)
        throw std::runtime_error(
            "TPCParameters::SetREBINZ(): rebin factor is either 0 or not even; must be 1 or 2, 4, 6, ...");
    fREBINZ = rebin;
}

ActRoot::TPCParameters::TPCParameters(const std::string& type)
{
    // std::cout << "Initializing detector type: " << type << '\n';
    if(type == "Actar")
    {
        fNPADSX = 128;
        fNPADSY = 128;
        fNPADSZ = 512;
        fZ = 235;
    }
    else if(type == "protoActar")
    {
        fNPADSX = 64;
        fNPADSY = 32;
        fNPADSZ = 512;
        fZ = 170;
    }
    else
        throw std::runtime_error("No TPCParameters config available for passed " + type);
    // And init sizes
    fX = fNPADSX * fPadSide;
    fY = fNPADSY * fPadSide;
}

void ActRoot::TPCParameters::Print() const
{
    std::cout << BOLDCYAN << "==== TPC parameters ====" << '\n';
    std::cout << "-> NPADSX : " << fNPADSX << '\n';
    std::cout << "-> NPADSY : " << fNPADSY << '\n';
    std::cout << "-> NPADSZ : " << fNPADSZ << '\n';
    std::cout << "-> REBINZ : " << fREBINZ << '\n';
    std::cout << "-> X      : " << fX << " mm" << '\n';
    std::cout << "-> Y      : " << fY << " mm" << '\n';
    std::cout << "-> Z      : " << fZ << " mm" << '\n';
    std::cout << "====================" << RESET << '\n';
}
