#ifndef ActPhysics_h
#define ActPhysics_h

#include <string>
namespace ActPhysics
{
    //! A class reading NUBASE particle info
    class Particle
    {
    private:
        const double kamuToMeVC2 {931.494028}; //!< Internal constant: amu to MeV/c2
        const double keMass {0.510998910}; //!< Internal constant: electron mass in MeV/c2
        std::string fName; //!< Name in database of the particle
        double fMassExcess; //!< Mass excess in MeV/c2
        double fMass; //!< Mass in MeV/c2
        int fA; //!< Masic number
        int fZ; //!< Charge of the particle

    public:
        Particle(int Z, int A);

        //Getters
        const std::string& GetName() const {return fName;}
        int GetA() const {return fA;}
        int GetZ() const {return fZ;}
        double GetMass() const {return fMass;}

        void Print() const;

    private:
        void ParseFile(int Z, int A, const std::string& file = "");
        void Extract(const std::string& line);
    };
}

#endif
