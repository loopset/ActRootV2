#ifndef ACT4RUNACTION_HH
#define ACT4RUNACTION_HH

#include "G4UserRunAction.hh"
#include "globals.hh"
#include "G4AnalysisManager.hh"

class G4Run;

class ActRunAction : public G4UserRunAction
{
public: 
    ActRunAction();
    ~ActRunAction() override = default;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;
};

#endif