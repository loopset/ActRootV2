#include "ActGeometry.h"

#include "Rtypes.h"
#include "RtypesCore.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMatrix.h"
#include "TGeoMedium.h"
#include "TGeoNavigator.h"
#include "TGeoVolume.h"
#include "TCanvas.h"
#include "TString.h"
#include "TView3D.h"
#include "TAxis3D.h"
#include "TFile.h"
#include "TRegexp.h"

#include <exception>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

ActSim::SilAssembly::SilAssembly(unsigned int index, const SilUnit& unit, bool alongx, bool alongy)
    : fIndex(index), fUnit(unit)
{
    if(alongx)
        fIsAlongX = true;
    else if(alongy)
        fIsAlongY = true;
    else if(alongy && alongx)
        throw std::runtime_error("AlongX && AlongY not allowed!");
    else
        throw std::runtime_error("No Along[X,Y] was set!");
}

void ActSim::SilAssembly::SetOffsets(double xoffset, double yoffset)
{
    if(xoffset != -1)
    {
        fOffset.first = xoffset;
        fHasXOffset = true;
    }
    if(yoffset != -1)
    {
        fOffset.second = yoffset;
        fHasYOffset = true;
    }
}

void ActSim::DriftChamber::Print() const
{
    std::cout<<"== Drift Info =="<<'\n';
    std::cout<<" X / 2 = "<<X<<" cm"<<'\n';
    std::cout<<" Y / 2 = "<<Y<<" cm"<<'\n';
    std::cout<<" Z / 2 = "<<Z<<" cm"<<'\n';
    std::cout<<"==============="<<std::endl;
}

void ActSim::SilUnit::Print() const
{
    std::cout<<"== SilUnit type "<<fIndex<<" =="<<'\n';
    std::cout<<" Width / 2  = "<<fLengthX<<" cm"<<'\n';
    std::cout<<" Length / 2 = "<<fLengthY<<" cm"<<'\n';
    std::cout<<" Height / 2 = "<<fLengthZ<<" cm"<<'\n';
    std::cout<<"==============="<<'\n';
}

void ActSim::SilAssembly::Print() const
{
    std::cout<<"** SilAssembly number "<<fIndex<<" **"<<'\n';
    std::cout<<"Using SilUnit = "<<fUnit.fIndex<<" with specs"<<'\n';
    fUnit.Print();
    std::cout<<" Placements = "<<'\n';
    for(const auto& [index, place] : fPlacements)
    {
        std::cout<<"    i = "<<index<<" first = "<<place.first<<" second = "<<place.second<<" cm"<<'\n';
    }
    std::cout<<"****************************"<<std::endl;
}

ActSim::Geometry::Geometry()
{
    //init structures and materials
    //set units
    TGeoManager::LockDefaultUnits(false);
    TGeoManager::SetDefaultUnits(TGeoManager::kRootUnits);
    TGeoManager::LockDefaultUnits(true);
    fManager = new TGeoManager("manager", "A simple ACTAR geometry");
    gGeoManager->SetVerboseLevel(0);

    //naive material since we dont compute physics here
    fNoneMaterial = new TGeoMaterial("none", 0.0, 0.0, 0.0);
    fNoneMedium = new TGeoMedium("none", 1, fNoneMaterial);

    //top volume
    //world shape as a box
    fTopVol = fManager->MakeBox("Top",
                                fNoneMedium,
                                100.,
                                100.,
                                100.); // 2x2x2 m3
    fManager->SetTopVolume(fTopVol);
}

ActSim::Geometry::~Geometry()
{
    delete fManager;
}

void ActSim::Geometry::Construct()
{
    //drift chamber
    fDriftVol = fManager->MakeBox("Drift",
                                  fNoneMedium,
                                  fActar.X, fActar.Y, fActar.Z);//at center of world
    fDriftVol->SetLineColor(kBlue);
    fTopVol->AddNode(fDriftVol, 1);

    //build silicon types
    for(const auto& [index, ass] : fAssDataMap)
    {
        const auto& silUnit {ass.fUnit};
        fUnitSilVols[index] = fManager->MakeBox(TString::Format("UnitSiliconType%d", silUnit.fIndex),
                                                fNoneMedium,
                                                silUnit.fLengthX,
                                                silUnit.fLengthY,
                                                silUnit.fLengthZ);
        fUnitSilVols.at(index)->SetLineColor(2 + index);
    }
    //and now assemblies!
    //for rotations of silicons!!!
    //only allowed by theta = 90 degrees by now
    //a rotation of phi = 90 is also included for lateral assemblies
    TGeoRotation nullRotation {"nullRotation", 0.0, 0.0, 0.0};
    TGeoRotation siliconRot {"siliconRot", 0.0, 90.0, 0.0};
    TGeoRotation lateralRot {"lateralRot", 90.0, 0.0, 0.0};
    TGeoRotation sideRot    {"sideRot", 180.0, 0.0, 0.0};
    for(const auto& [index, ass] : fAssDataMap)
    {
        fAssemblies[index] = new TGeoVolumeAssembly(TString::Format("SiliconAssembly%d", index));
        //and add its silicons
        for(const auto& [silIndex, place] : ass.fPlacements)
        {
            TGeoTranslation trans {};
            if(ass.fIsAlongX)
                trans = {0., place.first, place.second};
            if(ass.fIsAlongY)
                trans = {place.first, 0., place.second};
            fAssemblies.at(index)->AddNode(fUnitSilVols.at(index),
                                           silIndex,
                                           new TGeoCombiTrans(trans, ass.fIsAlongY ? lateralRot : nullRotation));
        }
        //placement of assembly
        TGeoTranslation assemblyTrans {};
        if(ass.fHasXOffset)
            assemblyTrans = {fActar.X + ass.fOffset.first, 0., 0.};
        else if(ass.fHasYOffset)
            assemblyTrans = {0., fActar.Y + ass.fOffset.second, 0.};
        else if(ass.fHasXOffset && ass.fHasYOffset)
            assemblyTrans = {fActar.X + ass.fOffset.first, fActar.Y + ass.fOffset.second, 0.};
        fTopVol->AddNode(fAssemblies.at(index),
                         1,//copy number 1
                         new TGeoCombiTrans(assemblyTrans, nullRotation));
        //if it is mirrored
        if(ass.fIsMirrored)//usually along y-direction
        {
            TGeoTranslation assemblyTrans2 {};
            auto previous {assemblyTrans.GetTranslation()};
            assemblyTrans2.SetDx(-1 * previous[0]);
            assemblyTrans2.SetDy(-1 * previous[1]);
            assemblyTrans2.SetDz(-1 * previous[2]);
            fTopVol->AddNode(fAssemblies.at(index),
                             -1,//mirrored copy
                             new TGeoCombiTrans(assemblyTrans2, sideRot));
        }
    }
    //and close geometry
    fManager->CloseGeometry();

    //initialize navigator
    fNavigator = new TGeoNavigator(fManager);
    fManager->SetCurrentNavigator(0);
}

void ActSim::Geometry::Print() const
{
    std::cout<<std::fixed<<std::setprecision(3);
    std::cout<<"ACTAR active volume: "<<'\n';
    fActar.Print();
    std::cout<<"Silicon detectors: "<<'\n';
    for(const auto& [index, ass] : fAssDataMap)
    {
        ass.Print();
    }
}

double ActSim::Geometry::GetAssemblyUnitWidth(unsigned int index)
{
    return fAssDataMap.at(index).fUnit.fLengthX * 2;// in cm
}

void ActSim::Geometry::Draw()
{
    fCanvas = new TCanvas("c1", "Drawing geometry", 1);
    fCanvas->cd();
    fManager->GetMasterVolume()->Draw("ogle");
    fCanvas->Update();
    TAxis3D::ToggleRulers();
    fCanvas->cd();
    //canvas->WaitPrimitive();
    //canvas->Close();
}

int ActSim::Geometry::GetAssemblyIndexFromTString(const TString& path)
{
    TRegexp legacy {"SiliconAssembly_."};
    TString subLegacy {path(legacy)};
    TRegexp regexp {"SiliconAssembly._."};
    TString subStr {path(regexp)};
    if(subLegacy.Length())
    {
        auto underscore {subLegacy.Index("_")};
        return TString(subLegacy(underscore + 1)).Atoi();
    }
    else if(subStr.Length())
    {
        auto underscore {subStr.Index("_")};
        return TString(subStr(underscore - 1)).Atoi();
    }
    else
    {
        throw std::runtime_error("Error: Legacy and new methods for AssemblyIndex at the same time");
    }
}

bool ActSim::Geometry::GetAssemblyMirrorFromTString(const TString& path)
{
    TRegexp regexp {"SiliconAssembly._-?./"};
    TString subStr {path(regexp)};
    auto underscore {subStr.Index("_")};
    auto slash {subStr.Index("/")};
    auto length {(slash - 1) - underscore};
    auto mirror {TString(subStr(underscore + 1, length))};
    if(mirror == "1")
        return false;
    else if(mirror == "-1")
        return true;
    else
        throw std::runtime_error("Wrong mirror value in SimGeometry");
}

std::tuple<int, int> ActSim::Geometry::GetSilTypeAndIndexFromTString(const TString& path)
{
    TRegexp regexp {"UnitSiliconType._.."};
    TString subStr {path(regexp)};
    auto underscore { subStr.Index("_")};
    int indexLength {(subStr.Length() - 1) - underscore};
    int silType  {TString(subStr(underscore - 1)).Atoi()};
    int silIndex {TString(subStr(underscore + 1, indexLength)).Atoi()};//selft determination of index length
    return std::make_tuple(silType, silIndex);
}

void ActSim::Geometry::PropagateTrackToSiliconArray(const XYZPoint &initPoint,
                                                    const XYZVector &direction,
                                                    int assemblyIndex,
                                                    bool& isMirror,
                                                    double& distance,
                                                    int &silType,
                                                    int &silIndex,
                                                    XYZPoint &newPoint,
                                                    bool debug)
{
    //set to default values
    silType  = -1;
    silIndex = -1;
    isMirror = false;
    //initializing state
    fManager->InitTrack(initPoint.X(), initPoint.Y(), initPoint.Z(),
                        direction.X(), direction.Y(), direction.Z());
    TString path { fManager->GetPath()};
    if(debug)std::cout<<" Path at 0: "<<path<<'\n';
    for(int i = 1; i < 6; i++)
    {
        fManager->FindNextBoundaryAndStep();
        path = fManager->GetPath();
        if(debug)std::cout<<"  Path at "<<i<<" : "<<path<<'\n';
        distance += fManager->GetStep();
        if(path.Contains(TString::Format("SiliconAssembly%d", assemblyIndex)))
        {
            auto silValues { GetSilTypeAndIndexFromTString(path)};
            silType       = std::get<0>(silValues);
            silIndex      = std::get<1>(silValues);
            isMirror      = GetAssemblyMirrorFromTString(path);
            if(debug)std::cout<<"   Fixed ass. = "<<assemblyIndex<<" isMirrored? "<<std::boolalpha<<isMirror<<" silType = "<<silType<<" silIndex = "<<silIndex<<'\n';
            break;
        }
        else if(path.IsWhitespace())//out of world
        {
            break;
        }
    }
    //and return new point!
    newPoint = initPoint + distance * direction.Unit();
}

void ActSim::Geometry::CheckIfStepIsInsideDriftChamber(const XYZPoint &point,
                                                       const XYZVector &direction,
                                                       double step,
                                                       bool &isInside,
                                                       XYZPoint& newPoint,
                                                       bool debug)
{
    fManager->InitTrack(point.X(), point.Y(), point.Z(),
                        direction.X(), direction.Y(), direction.Z());
    fManager->SetStep(step);
    fManager->Step(false);//false bc is given by hand
    auto distance {fManager->GetStep()};
    TString path { fManager->GetPath()};
    if(debug)std::cout<<"Step in drift debug: "<<path<<'\n';
    if(path.Contains("Drift"))
    {
        isInside = true;
        newPoint = point + distance * direction.Unit();
    }
    else
    {
        isInside = false;
        newPoint = {-1, -1, -1};
    }
}

void ActSim::Geometry::FindBoundaryPoint(const XYZPoint &vertex, const XYZVector &direction, double &distance, XYZPoint &bp, bool debug)
{

    //set to default values
    distance = -1;
    bp = {-1, -1, -1};
    fManager->InitTrack(vertex.X(), vertex.Y(), vertex.Z(),
                        direction.X(), direction.Y(), direction.Z());
    //Check that indeed we are inside drif chamber
    TString path {fManager->GetPath()};
    if(debug)std::cout<<"Initia path = "<<path<<'\n';
    if(!path.Contains("Drift"))
    {
        std::cout<<"In SimGeometry::FindBoundaryPoint() initial point was not inside Drift volume!"<<'\n';
        return;
    }
    //Then, propagate to boundary of drift chamber
    fManager->FindNextBoundaryAndStep();
    TString pathbp {fManager->GetPath()};
    if(debug)
        std::cout<<"Path of BP = "<<pathbp<<'\n';
    //and return values!
    distance = fManager->GetStep();
    bp = vertex + direction.Unit() * distance;//remember: SimGeometry works in cm, likely you would like to convert to mm after function calling
}

void ActSim::Geometry::ReadGeometry(std::string path, std::string fileName)
{
    fManager = TGeoManager::Import((path + fileName + ".root").c_str());
    if(!fManager)
        throw std::runtime_error("Error reading TGeoManager from " + fileName);
    //also, read parameters
    auto* infile = new TFile((path + "parameters_" + fileName + ".root").c_str(), "read");
    infile->cd();
    fActar = *(infile->Get<DriftChamber>("drift"));
    std::map<unsigned int , SilAssembly>* assemblyMapAux {};
    infile->GetObject("assemblies", assemblyMapAux);
    fAssDataMap = *assemblyMapAux;
    infile->Close();
    delete infile;
}

void ActSim::Geometry::WriteGeometry(std::string path, std::string fileName)
{
    fManager->Export((path + fileName + ".root").c_str());

    //write parameters to file
    auto* outFile = new TFile((path + "parameters_" + fileName + ".root").c_str(), "recreate");
    outFile->cd();
    outFile->WriteObject(&fActar, "drift");
    outFile->WriteObject(&fAssDataMap, "assemblies");
    outFile->Close();
    delete outFile;
    
}
