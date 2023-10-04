#ifndef ACTDRIFTDETECTOR_H
#define ACTDRIFTDETECTOR_H
#include "G4VUserDetectorConstruction.hh"

#include "G4Material.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Types.hh"
#include "G4String.hh"
#include <vector>


/*
  Auxiliar class for build drift volume inside main detector chamber
*/
class ActDriftDetector : public G4VUserDetectorConstruction
{
private:
	G4Material* gasMaterial;

	// G4int numberOfGasesMixture;
	// std::vector<G4String> gasMixtureMaterials;
	// std::vector<G4double> gasMixtureRatios;

	// //gas parameters
	// G4double gasPressure;
	// G4double gasTemperature;

	//sizes
	G4double driftSizeX;
	G4double driftSizeY;
	G4double driftSizeZ;

	G4double driftCenterX;
	G4double driftCenterY;
	G4double driftCenterZ;

	//pointer to base vol, chamber vol
	G4LogicalVolume* fChamberLog;

	G4VPhysicalVolume* ConstructDriftChamber();

public:
	ActDriftDetector(G4LogicalVolume* chamberLog);
	~ActDriftDetector() override = default;

	G4VPhysicalVolume* Construct() override;

	//set gas parameters
	void SetGasMaterial(G4Material* mat){ gasMaterial = mat; };
	void SetDefaultGas();//only for constructor
	// void SetGasMixtureNComponents(G4int val);
	// void SetGasMixtureComponent(G4int num, G4String gasMat, G4double gasRatio);

	// void SetGasPressure(G4double val){ gasPressure = val; }
	// void SetGasTemperature(G4double val){ gasTemperature = val; }

	//sizes
	void SetDriftSizeX(G4double val){ driftSizeX = val; }
	void SetDriftSizeY(G4double val){ driftSizeY = val; }
	void SetDriftSizeZ(G4double val){ driftSizeZ = val; }

	void SetDriftCenterX(G4double val){ driftCenterX = val; }
	void SetDriftCenterY(G4double val){ driftCenterY = val; }
	void SetDriftCenterZ(G4double val){ driftCenterZ = val; }

	//and finally getters
	G4Material* GetGasMaterial() { return gasMaterial; }
	// G4double GetGasPressure() const { return gasPressure; }
	// G4double GetGasTemperature() const { return gasTemperature; }

	G4double GetDriftSizeX() const { return driftSizeX; }
	G4double GetDriftSizeY() const { return driftSizeY; }
	G4double GetDriftSizeZ() const { return driftSizeZ; }

	G4double GetDriftCenterX() const { return driftCenterX; }
	G4double GetDriftCenterY() const { return driftCenterY; }
	G4double GetDriftCenterZ() const { return driftCenterZ; }

	void UpdateGeometry();
	void PrintDetectorParameters();
	

};

#endif
