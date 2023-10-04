#include "Act4PrimaryGenerator.hh"

#include "G4RunManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"


ActPrimaryGenerator::ActPrimaryGenerator()
{
    G4int nOfParticles {1};
    fParticleGun = new G4ParticleGun(nOfParticles);

    //default kinematics
    auto* particleDefinition {G4ParticleTable::GetParticleTable()->FindParticle("proton")};
    if(!particleDefinition)
        G4cout<<"Particle proton couldn't be found in G4ParticleTable"<<G4endl;
    fParticleGun->SetParticleDefinition(particleDefinition);
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(1, 0, 0));//along x axis
    fParticleGun->SetParticleEnergy(50 * MeV);
}

ActPrimaryGenerator::~ActPrimaryGenerator()
{
    delete fParticleGun;
}

void ActPrimaryGenerator::GeneratePrimaries(G4Event* event)
{
    G4double driftHalfLength {};
    auto* driftLog {G4LogicalVolumeStore::GetInstance()->GetVolume("driftLog")};
    G4Box* driftBox {};
    if(driftLog)
        driftBox = dynamic_cast<G4Box*>(driftLog->GetSolid());
    if(driftBox)
        driftHalfLength = driftBox->GetXHalfLength();
    else
    {
        G4ExceptionDescription msg;
        msg << "World volume of box shape not found." << G4endl;
        msg << "Perhaps you have changed geometry." << G4endl;
        msg << "The gun will be place in the center.";
        G4Exception("PrimaryGeneratorAction::GeneratePrimaries()",
            "MyCode0002", JustWarning, msg);

    }

    //Set vertex position
    G4double xVertex {G4UniformRand() * driftHalfLength};
    G4double yVertex {G4UniformRand() * driftBox->GetYHalfLength()};
    G4double zVertex {0.};
    fParticleGun->SetParticlePosition(G4ThreeVector(xVertex, yVertex, zVertex));
    fParticleGun->GeneratePrimaryVertex(event);
}
