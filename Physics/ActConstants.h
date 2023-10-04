#ifndef ActConstants_h
#define ActConstants_h

namespace ActPhysics
{
    namespace Constants
    {
        //Mases of fundamental particles
        const double kamuToMeVC2 {931.494028};//!< amu to MeV / c2 scaling factor
        const double keMass {0.510998910};//!< electron mass in MeV / c2
        const double kpMass {938.27208816};//!< proton mass in MeV / c2
        const double knMass {939.56542052};//!< neutron mass in MeV / c2

        //Conversion of units
        const double kMeVToGeV {1e-3}; //!< Scaling factor for MeV -> GeV
    }
}

#endif
