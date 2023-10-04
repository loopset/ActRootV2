#ifndef ACTSTEPPINGACTION_HH
#define ACTSTEPPINGACTION_HH

#include "G4UserSteppingAction.hh"

class ActDetectorConstruction;
class ActEventAction;

class ActSteppingAction : public G4UserSteppingAction
{
public:
    ActSteppingAction(const ActDetectorConstruction* detConstruction,
                    ActEventAction* eventAction);
    ~ActSteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override;

private:
    const ActDetectorConstruction* fDetConstruction {};
    ActEventAction* fEventAction {};
};

#endif