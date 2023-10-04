#include "ActActionInitialization.hh"

#include "ActPrimaryGenerator.hh"
#include "ActRunAction.hh"
#include "ActDetectorConstruction.hh"
#include "ActEventAction.hh"
#include "ActSteppingAction.hh"

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