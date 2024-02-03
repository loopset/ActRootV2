#ifndef ACT4ACTIONINITIALIZATION_HH
#define ACT4ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

class ActDetectorConstruction;

class ActActionInitialization : public G4VUserActionInitialization
{
private:
    ActDetectorConstruction* fDetConstruction {};

public:
    ActActionInitialization(ActDetectorConstruction*);
    ~ActActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;
};

#endif