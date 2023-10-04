#include "ActDriftDetector.hh"

#include "ActDetectorConstruction.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4String.hh"
#include "G4Types.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Region.hh"
#include "G4ThreeVector.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VisAttributes.hh"
#include <vector>

ActDriftDetector::ActDriftDetector(G4LogicalVolume* chamberLog)
	: fChamberLog(chamberLog)
{
	// //default parameters
	// SetGasPressure(1.01325 * bar);
	// SetGasTemperature(293.15 * kelvin);

	SetDefaultGas();

	driftSizeX = 0.5 * m;
	driftSizeY = 0.5 * m;
	driftSizeZ = 0.5 * m;

	driftCenterX = 0. * m;
	driftCenterY = 0. * m;
	driftCenterZ = 0. * m;
}


G4VPhysicalVolume* ActDriftDetector::Construct()
{
	return ConstructDriftChamber();
}

G4VPhysicalVolume* ActDriftDetector::ConstructDriftChamber()
{
	//default box size for ACTAR TPC
	driftSizeX = 295. / 2 * mm;
	driftSizeY = 295. / 2 * mm;
	driftSizeZ = 255. / 2 * mm;
   
	//centered at center of main volume
	driftCenterX = 0. * mm;
	driftCenterY = 0. * mm;
	driftCenterZ = -15.46 * mm; //following ActarSim specs
    
	auto* driftBox = new G4Box("driftBox",
							   driftSizeX,
							   driftSizeY,
							   driftSizeZ);
	auto* driftLog = new G4LogicalVolume(driftBox, gasMaterial, "driftLog");
	auto* driftPhys = new G4PVPlacement(0,
										G4ThreeVector(driftCenterX, driftCenterY, driftCenterZ),
										driftLog,
										"driftPhys",
										fChamberLog,//<- Mother volume is chamber
										false,
										0);

	//do not set sentitive detector by now
    
	//assign a region as recommended by documentation
	auto* driftRegion = new G4Region("driftRegion");
	driftRegion->AddRootLogicalVolume(driftLog);

	//visualization attributes
	auto* driftVisAtt = new G4VisAttributes(G4Colour(0., 1., 0., 0.25));
	driftVisAtt->SetVisibility(true);
	driftLog->SetVisAttributes(driftVisAtt);

    
	return driftPhys;
}

void ActDriftDetector::SetDefaultGas()
{
	//nist database
	G4NistManager* nist = G4NistManager::Instance();
	auto* Air = nist->FindOrBuildMaterial("G4_AIR");
	gasMaterial = Air;
	delete nist;
}
