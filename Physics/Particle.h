#ifndef ActPhysics_h
#define ActPhysics_h

#include <string>
namespace ActPhysics
{
    //! A class reading NUBASE particle info
    class Particle
    {
    private:
        double kamuToMeVC2 {931.494028}; //!< Internal constant: amu to MeV/c2
        double keMass {0.510998910}; //!< Internal constant: electron mass in MeV/c2
        std::string fName; //!< Name in database of the particle
        double fMassExcess; //!< Mass excess in MeV/c2
        double fMass; //!< Mass in MeV/c2
        int fA; //!< Masic number
        int fZ; //!< Charge of the particle

    public:
        Particle() = default;
        Particle(int Z, int A);
        Particle(const std::string& particle);
        Particle(const Particle& ) = default;
        Particle& operator=(const Particle& ) = default;
        ~Particle() = default;

        //Getters
        const std::string& GetName() const {return fName;}
        int GetA() const {return fA;}
        int GetZ() const {return fZ;}
        double GetMass() const {return fMass;}
        double GetAMU() const {return fMass / kamuToMeVC2;}//!< Return mass in amu units
        void Print() const;

    private:
        void ParseFile(int Z, int A, const std::string& file = "");
        void ParseFile(const std::string& particle, const std::string& file = "");
        void Extract(const std::string& line);
        std::string StripWhitespaces(std::string str);
    };
}

#endif
