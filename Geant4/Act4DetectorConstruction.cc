#include "Act4DetectorConstruction.hh"

#include "Act4DriftDetector.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4ThreeVector.hh"
#include "G4Types.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VisAttributes.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4ios.hh"

//Construct method for geometry
G4VPhysicalVolume* ActDetectorConstruction::Construct()
{
	//Define materials
	DefineMaterials();

	//Build solid world
	return DefineVolumes();
}

G4VPhysicalVolume* ActDetectorConstruction::DefineVolumes()
{
	//1--> World
	auto worldSizeX { 1. * m};
	auto worldSizeY { 1. * m};
	auto worldSizeZ { 1. * m};
	auto worldBox = new G4Box("World",
		worldSizeX,
		worldSizeY,
		worldSizeZ);
	fWorldLV = new G4LogicalVolume(worldBox,
		G4Material::GetMaterial("Galactic"),
		"World");
	fWorldPV = new G4PVPlacement(0,
		G4ThreeVector(),
		fWorldLV,
		"World",
		0,
		false,
		0);

	fWorldLV->SetVisAttributes(G4VisAttributes::GetInvisible());

	//2--> mechanical chamber (half-lengths!!!)
	auto chamberSizeX = 606. / 2 * mm;
	auto chamberSizeY = 606. / 2 * mm;
	auto chamberSizeZ = 335. / 2 * mm;

	//at center of world
	auto chamberCenterX = 0. * m;
	auto chamberCenterY = 0. * m;
	auto chamberCenterZ = 0. * m;

	auto* chamberS = new G4Box("Chamber",
		chamberSizeX,
		chamberSizeY,
		chamberSizeZ);
	fChamberLV = new G4LogicalVolume(chamberS,
		G4Material::GetMaterial("G4_AIR"),
		"Chamber");
	fChamberPV = new G4PVPlacement(0,
		G4ThreeVector(chamberCenterX, chamberCenterY, chamberCenterZ),
		fChamberLV,
		"Chamber",
		fWorldLV,
		false,
		0);

	//draw mechanical chamber
	auto chamberVisAtt = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));//gray and alpha=0.25
	chamberVisAtt->SetVisibility(true);
	fChamberLV->SetVisAttributes(chamberVisAtt);

	//3-->Drift chamber
	//default box size for ACTAR TPC
	auto driftSizeX = 295. / 2 * mm;
	auto driftSizeY = 295. / 2 * mm;
	auto driftSizeZ = 255. / 2 * mm;
   
	//centered at center of main volume
	auto driftCenterX = 0. * mm;
	auto driftCenterY = 0. * mm;
	auto driftCenterZ = -15.46 * mm; //following ActarSim specs
    
	auto* driftS = new G4Box("driftBox",
							   driftSizeX,
							   driftSizeY,
							   driftSizeZ);
	fDriftLV = new G4LogicalVolume(driftS, G4Material::GetMaterial("G4_AIR"), "driftLog");
	fDriftPV = new G4PVPlacement(0,
										G4ThreeVector(driftCenterX, driftCenterY, driftCenterZ),
										fDriftLV,
										"driftPhys",
										fChamberLV,//<- Mother volume is chamber
										false,
										0);

	//do not set sentitive detector by now
	//visualization attributes
	auto* driftVisAtt = new G4VisAttributes(G4Colour(0., 1., 0.));
	driftVisAtt->SetVisibility(true);
	fDriftLV->SetVisAttributes(driftVisAtt);

	//4--> pad plane!
	auto padPlaneX { 128. * mm};
	auto padPlaneY { 128. * mm};
	auto padPlaneZ { 4.54 / 2 * mm};

	auto* padPlaneS {new G4Box("PadPlane", padPlaneX, padPlaneY, padPlaneZ)};
	auto* padPlaneLV = new G4LogicalVolume(padPlaneS, G4Material::GetMaterial("G4_Al"), "PadPlane");
	//--place it under chamber
	auto padPlanePosX { 0. * cm};
	auto padPlanePosY { 0. * cm};
	auto padPlanePosZ { -chamberSizeZ + padPlaneZ};
	fPadPlanePV = new G4PVPlacement(0, 
		G4ThreeVector(padPlanePosX, padPlanePosY, padPlanePosZ), 
		padPlaneLV,
		"PadPlane",
		fChamberLV,
		false,
		0);

	//visualization attributes
	auto* padPlaneVisAtt {new G4VisAttributes(G4Color(1., 0., 1.))};
	padPlaneVisAtt->SetVisibility(true);
	padPlaneLV->SetVisAttributes(padPlaneVisAtt);
	
	//always return the physical world
	return fWorldPV;
}

void ActDetectorConstruction::DefineMaterials()
{
	G4double z, a, density, pressure, temperature;
	//nist database
	G4NistManager* nist = G4NistManager::Instance();
	auto* N = nist->FindOrBuildElement("N");
	auto* O = nist->FindOrBuildElement("O");

	//galactic material
	auto* Galactic = new G4Material("Galactic", z = 1.,
		a = 1.01 * g / mole,
		density = universe_mean_density,
		kStateGas,
		temperature = 2.73 * kelvin,
		pressure = 3.e-18 * pascal);

	//air
	auto* Air = nist->FindOrBuildMaterial("G4_AIR");
	if(!Air)G4cout << "Null G4_AIR" << G4endl;
	//mylar
	auto* Mylar = nist->FindOrBuildMaterial("G4_MYLAR");
	if(!Mylar)G4cout << "Null G4_MYLAT" << G4endl;
	//Al
	auto* Aluminium {nist->FindOrBuildMaterial("G4_Al")};
	if(!Aluminium)G4cout<<"Null G4_Al"<<G4endl;

}