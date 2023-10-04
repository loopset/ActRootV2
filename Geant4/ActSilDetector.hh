#ifndef ACTSILDETECTOR_HH
#define ACTSILDETECTOR_HH

class ActDetectorConstruction;

class G4LogicalVolume;
class G4VPhysicalVolume;

class ActSilDetector
{
private:
    //pointer to volumes
    G4LogicalVolume* fSilLV {};
    G4VPhysicalVolume* fSilPV {};
    //pointer to mother volumes
    const ActDetectorConstruction* fDetConstruction {};

public:
    ActSilDetector(const ActDetectorConstruction* detConstruction);

    G4VPhysicalVolume* Construct();
    void DefineMaterials();
};

#endif