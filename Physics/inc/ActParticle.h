#ifndef ActParticle_h
#define ActParticle_h

#include "ActConstants.h"

#include <string>
namespace ActPhysics
{
//! A class reading NUBASE particle info
class Particle
{
private:
    std::string fName {};  //!< Name in database of the particle
    double fMassExcess {}; //!< Mass excess in MeV/c2
    double fMass {};       //!< Mass in MeV/c2 = fGSMass + fEx
    double fGSMass {};     //!< Ground-state mass in MeV/c2
    double fEx {};         //!< Excitation energy. Not read from table, only settable through SetEx method
    int fA {};             //!< Masic number
    int fZ {};             //!< Charge of the particle

public:
    Particle() = default;
    Particle(int Z, int A);
    Particle(const std::string& particle);
    Particle(const Particle&) = default;
    Particle& operator=(const Particle&) = default;
    ~Particle() = default;

    // Setters
    void SetEx(double Ex)
    {
        fEx = Ex;
        fMass = fGSMass + fEx;
    }

    // Getters
    const std::string& GetName() const { return fName; }
    int GetA() const { return fA; }
    int GetZ() const { return fZ; }
    int GetN() const { return fA - fZ; }
    double GetGSMass() const { return fGSMass; }                             //!< GS mass. Ensures Ex = 0 always
    double GetGSMassAMU() const { return fGSMass * Constants::kamuToMeVC2; } //!< GS mass in amu
    double GetMass() const { return fMass; }                                 //!< Return mass in MeV / c2 units
    double GetAMU() const { return fMass / Constants::kamuToMeVC2; }         //!< Return mass in amu units
    double GetBE() const; //!< Return binding energy. If BE < 0 is unbound
    double GetMassExcess() const { return fMassExcess; }
    double GetSn() const { return GetSnX(1); }
    double GetS2n() const { return GetSnX(2); }
    double GetSp() const { return GetSpX(1); }
    double GetS2p() const { return GetSpX(2); }
    double GetEx() const { return fEx; }

    // Utility funtions
    void Print() const;

private:
    void ParseFile(int Z, int A, const std::string& file = "");
    void ParseFile(const std::string& particle, const std::string& file = "");
    void Extract(const std::string& line);
    std::string StripWhitespaces(std::string str);
    double GetSnX(unsigned int X) const;
    double GetSpX(unsigned int X) const;
};
} // namespace ActPhysics
#endif
