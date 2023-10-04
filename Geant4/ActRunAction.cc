#include "ActRunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

ActRunAction::ActRunAction()
{
    G4RunManager::GetRunManager()->SetPrintProgress(true);

    //create the analysis manager! Only store naive values by now
    auto* analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetVerboseLevel(1);
    analysisManager->SetNtupleMerging(true);

    //Book histograms and ntuple
    analysisManager->CreateH1("Egas", "Eloss in gas", 200, 0, 100 * MeV);
    analysisManager->CreateNtuple("ActGeant4", "Naive tree");
    analysisManager->CreateNtupleDColumn("Egas");
    analysisManager->FinishNtuple();
}

void ActRunAction::BeginOfRunAction(const G4Run* run)
{
    auto* analysisManager = G4AnalysisManager::Instance();
    G4String file {"ActGeant4_Test.root"};
    analysisManager->OpenFile(file);
}

void ActRunAction::EndOfRunAction(const G4Run* run)
{
    auto* analysisManager = G4AnalysisManager::Instance();
    if(auto* h {analysisManager->GetH1(0)}; h)
    {
        G4cout<<"Stats for ELoss in gas = "<<G4BestUnit(h->mean(), "Energy")<<G4endl;
    }

    //save
    analysisManager->Write();
    analysisManager->CloseFile();
}