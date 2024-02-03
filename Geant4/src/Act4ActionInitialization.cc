#include "Act4ActionInitialization.hh"

#include "Act4PrimaryGenerator.hh"
#include "Act4RunAction.hh"
#include "Act4DetectorConstruction.hh"
#include "Act4EventAction.hh"
#include "Act4SteppingAction.hh"

ActActionInitialization::ActActionInitialization(ActDetectorConstruction* det)
    : fDetConstruction(det)
{}

void ActActionInitialization::BuildForMaster() const 
{
    SetUserAction(new ActRunAction);
}

void ActActionInitialization::Build() const 
{
    SetUserAction(new ActPrimaryGenerator);
    SetUserAction(new ActRunAction);
    auto eventAction = new ActEventAction();
    SetUserAction(eventAction);
    SetUserAction(new ActSteppingAction(fDetConstruction, eventAction));
}