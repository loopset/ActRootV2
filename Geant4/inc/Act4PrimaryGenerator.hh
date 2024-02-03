#ifndef ACT4PRIMARYGENERATOR_HH
#define ACT4PRIMARYGENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;

class ActPrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
private:
    G4ParticleGun* fParticleGun {};

public:
    ActPrimaryGenerator();
    ~ActPrimaryGenerator() override;

    void GeneratePrimaries(G4Event* event) override;

};

#endif