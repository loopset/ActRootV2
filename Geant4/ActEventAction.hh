#ifndef ACTEVENTACTION_HH
#define ACTEVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

class ActEventAction : public G4UserEventAction
{
private:
    G4double fGasEnergy {};
public:
    ActEventAction() = default;
    ~ActEventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void AddEgas(G4double de) {fGasEnergy += de; }
};

#endif