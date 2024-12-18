#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// add link for namespace
#pragma link C++ namespace ActRoot;

// Parameters
#pragma link C++ class ActRoot::VParameters + ;
#pragma link C++ class ActRoot::TPCParameters + ;
#pragma link C++ class ActRoot::SilParameters + ;
#pragma link C++ class ActRoot::ModularParameters + ;
#pragma link C++ class ActRoot::MergerParameters + ;

// Thomas' legacy data structures
#pragma link C++ class ReducedData + ;
#pragma link C++ class MEventReduced + ;

// Data and other structures: cluster, line, region,...
#pragma link C++ class ActRoot::VData + ;
#pragma link C++ class ActRoot::Voxel + ;
#pragma link C++ class ActRoot::Region + ;
#pragma link C++ class ActRoot::Line + ;
#pragma link C++ class ActRoot::Cluster + ;
#pragma link C++ class ActRoot::TPCData + ;
#pragma link C++ class ActRoot::SilData + ;
#pragma link C++ class ActRoot::ModularData + ;
#pragma link C++ class ActRoot::MergerData + ;

// Schema evolution for ActPhysics::Line to ActRoot::Line
#pragma read \
        sourceClass="ActPhysics::Line" \
        source="ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<float>,ROOT::Math::DefaultCoordinateSystemTag> fSigmas; ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<float>,ROOT::Math::DefaultCoordinateSystemTag> fPoint; ROOT::Math::DisplacementVector3D<ROOT::Math::Cartesian3D<float>,ROOT::Math::DefaultCoordinateSystemTag> fDirection; float fChi2" \
        targetClass="ActRoot::Line"\
        target="fSigmas, fPoint, fDirection, fChi2" \
        include="Math/Point3D.h,Math/Vector3D.h" \
        code="{fSigmas = onfile.fSigmas; fPoint = onfile.fPoint; fDirection = onfile.fDirection; fChi2 = onfile.fChi2;}";

#endif
