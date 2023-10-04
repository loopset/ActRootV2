#include "ActEventAction.hh"
#include "ActRunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4UnitsTable.hh"

#include "Randomize.hh"
#include <iomanip>

void ActEventAction::BeginOfEventAction(const G4Event* event)
{
    fGasEnergy = 0.;
}

void ActEventAction::EndOfEventAction(const G4Event* event)
{
    auto* analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH1(0, fGasEnergy);
    analysisManager->FillNtupleDColumn(0, fGasEnergy);
    analysisManager->AddNtupleRow();
}