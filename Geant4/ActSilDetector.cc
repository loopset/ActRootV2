#include "ActSilDetector.hh"

#include "ActDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4ios.hh"
#include "G4PVPlacement.hh"

#include <set>
ActSilDetector::ActSilDetector(const ActDetectorConstruction* detCons)
    : fDetConstruction(detCons)
{
    //Define materials
    DefineMaterials();
}

G4VPhysicalVolume* ActSilDetector::Construct()
{
    //Define silicons for E796 front
    auto silXOffset {10 * cm};
    auto silYOffset {0 * cm};
    auto silThickness {5.E-2 / 2 * cm};
    auto silHeight {5. / 2 * cm};
    auto silWidth {8. / 2 * cm};
    //Get DriftPV width
    auto* driftBox {dynamic_cast<G4Box*>(fDetConstruction->GetDriftLV()->GetSolid())};
    auto halfLengthDrift {driftBox->GetXHalfLength()};

    //Box
    auto silSV {new G4Box("silSV", silThickness, silWidth, silHeight)};
    //Logic volume
    fSilLV = new G4LogicalVolume(silSV, G4Material::GetMaterial("G4_Si"), "silLV");
    //Physical volumes
    std::set<int> rows {-2, 0, 2, 4};
    std::set<int> cols {-2, 0, 2};
    int ncopy {0};
    for(const auto& r : rows)
    {
        for(const auto& c : cols)
        {
            auto pos {G4ThreeVector(halfLengthDrift + silXOffset, silYOffset + c * silWidth, r * silHeight)};
            fSilPV = new G4PVPlacement(0, pos, fSilLV, "silPV", fDetConstruction->GetChamberLV(), false, ncopy);
            ncopy++;
        }
    }

    return fSilPV;
}

void ActSilDetector::DefineMaterials()
{
    auto* nist {G4NistManager::Instance()};
    auto* Si {nist->FindOrBuildMaterial("G4_Si")};
    if(!Si)
        G4cout<<"Null G4_Si"<<G4endl;
}