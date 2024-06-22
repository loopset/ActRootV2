#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

//Geometry classes
#pragma link C++ class ActSim::DriftChamber+;
#pragma link C++ class ActSim::SilUnit+;
#pragma link C++ class ActSim::SilAssembly+;
#pragma link C++ class ActSim::Geometry+;

//Generators
#pragma link C++ class ActSim::KinematicGenerator;

//Runner
#pragma link C++ class ActSim::Runner;

//Theoretical cross section
#pragma link C++ class ActSim::TheoCrossSection;

// Track generator
#pragma link C++ class ActSim::TrackGenerator;

#endif
