#include "Act4SteppingAction.hh"
#include "Act4EventAction.hh"
#include "Act4DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4RunManager.hh"

ActSteppingAction::ActSteppingAction(const ActDetectorConstruction* detConstruction,
    ActEventAction* eventAction)
    : fDetConstruction(detConstruction), fEventAction(eventAction)
{
}

void ActSteppingAction::UserSteppingAction(const G4Step* step)
{
    // get volume of the current step
    auto volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();

    // energy deposit
    auto edep = step->GetTotalEnergyDeposit();
    // step length
    G4double stepLength = 0.;
    if(step->GetTrack()->GetDefinition()->GetPDGCharge() != 0.)
    {
        stepLength = step->GetStepLength();
    }

    if(volume == fDetConstruction->GetDriftPV())
    {
        fEventAction->AddEgas(edep);
    }
}