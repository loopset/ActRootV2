#ifndef ACTDETECTORCONSTRUCTION_H
#define ACTDETECTORCONSTRUCTION_H

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;
//forward declaration of classes for detectors!
//class ActDriftDetector;

class ActDetectorConstruction : public G4VUserDetectorConstruction
{
private:
	//logical volumes
	G4LogicalVolume* fWorldLV {};
	G4LogicalVolume* fChamberLV {};
	G4LogicalVolume* fDriftLV {};
	//pointers to physical volumes
	G4VPhysicalVolume* fWorldPV {};
	G4VPhysicalVolume* fChamberPV {};
	G4VPhysicalVolume* fDriftPV {};
	G4VPhysicalVolume* fPadPlanePV {};
	G4VPhysicalVolume* fSilPV {};

public:
	ActDetectorConstruction() = default;
	~ActDetectorConstruction() override = default;

	G4VPhysicalVolume* Construct() override;

	G4LogicalVolume* GetChamberLV() const { return fChamberLV; }

	G4VPhysicalVolume* GetDriftPV() const { return fDriftPV; }
	G4LogicalVolume* GetDriftLV() const { return fDriftLV; }
	
private:
	void DefineMaterials();
	G4VPhysicalVolume* DefineVolumes();
	G4VPhysicalVolume* DefineACTARTPC();
};

#endif //ACTDETECTORCONSTRUCTION_H
