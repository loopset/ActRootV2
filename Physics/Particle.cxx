#include "Particle.h"

#include "Constants.h"

#include "TSystem.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

ActPhysics::Particle::Particle(int Z, int A)
{
    ParseFile(Z, A);
}

ActPhysics::Particle::Particle(const std::string& particle)
{
    ParseFile(particle);
}

void ActPhysics::Particle::ParseFile(int Z, int A, const std::string& file)
{
    //If no file provided, use default
    std::string filename {file};
    if(filename.length() == 0)
        filename = std::string(gSystem->Getenv("ACTROOT")) + "/Physics/Data/nubase20.txt";
    //Streamer
    std::ifstream streamer {filename};
    if(!streamer)
        throw std::runtime_error("Mass database file " + filename + " could not be opened");
    //Parse
    std::string line, zstr, astr, istr;
    while(std::getline(streamer, line))
    {
        astr = line.substr(0, 3);
        zstr = line.substr(4, 3);
        istr = line.substr(5, 1);
        int a {std::stoi(astr)}; int z {std::stoi(zstr)};
        int i {std::stoi(istr)};//this measures whether it is an isomer or a normal ground state ( = 0)
        //std::cout<<line<<'\n';
        //std::cout<<"a = "<<a<<" z = "<<z<<" i = "<<istr<<'\n';
        if(a == A && z == Z && i == 0)//we will work almost always with standard gs
        {
            Extract(line);
            break;
        }
    }
}

void ActPhysics::Particle::ParseFile(const std::string& particle, const std::string& file)
{
    auto isotope {StripWhitespaces(particle)};
    //If no file provided, use default
    std::string filename {file};
    if(filename.length() == 0)
        filename = std::string(gSystem->Getenv("ACTROOT")) + "/Physics/Data/nubase20.txt";
    //Streamer
    std::ifstream streamer {filename};
    if(!streamer)
        throw std::runtime_error("Mass database file " + filename + " could not be opened");
    //Parse
    std::string line, name, istr;
    while(std::getline(streamer, line))
    {
        istr = line.substr(5, 1);
        name = line.substr(11, 5);
        name = StripWhitespaces(name);
        int i {std::stoi(istr)};//this measures whether it is an isomer or a normal ground state ( = 0)
        //std::cout<<line<<'\n';
        //std::cout<<"a = "<<a<<" z = "<<z<<" i = "<<istr<<'\n';
        if(name == isotope && i == 0)//we will work almost always with standard gs
        {
            Extract(line);
            break;
        }
    }
}

void ActPhysics::Particle::Extract(const std::string& line)
{
    //Get important info
    //1->Name
    auto name {line.substr(11, 5)};
    //Strip whitespaces if needed
    name.erase(std::remove_if(name.begin(), name.end(), [](char x){return std::isspace(x);}), name.end());
    fName = name;
    //2->A
    auto astr {line.substr(0, 3)};
    fA = std::stoi(astr);
    //3->Z
    auto zstr {line.substr(4, 3)};
    fZ = std::stoi(zstr);
    //4-> Mass excess
    auto massexcess {line.substr(18, 13)};
    fMassExcess = std::stod(massexcess) * 1.e-3;//keV to MeV
    //Build mass
    fMass = fA * Constants::kamuToMeVC2 + fMassExcess - fZ * Constants::keMass;
}

void ActPhysics::Particle::Print() const
{
    std::cout<<"======== Particle: "<<fName<<" ========="<<'\n';
    std::cout<<"-> A    = "<<fA<<'\n';
    std::cout<<"-> Z    = "<<fZ<<'\n';
    std::cout<<"-> Mass = "<<fMass<<" MeV / c2"<<'\n';
}

std::string ActPhysics::Particle::StripWhitespaces(std::string str)
{
    str.erase(std::remove_if(str.begin(), str.end(), [](char x){return std::isspace(x);}), str.end());
    return str;
}

double ActPhysics::Particle::GetSnX(unsigned int X) const
{
    Particle N(fZ, fA - X);
    return N.GetMass() + X * Constants::knMass - fMass;
}

double ActPhysics::Particle::GetSpX(unsigned int X) const
{
    Particle P(fZ - X, fA - X);
    return P.GetMass() + X * Constants::kpMass - fMass;
}
