#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// add link for namespace
#pragma link C++ namespace ActRoot;

// type definitions
#pragma link C++ enum ActRoot::DetectorType + ;
#pragma link C++ enum ActRoot::ModeType + ;

// Base classes
// virtual detector
#pragma link C++ class ActRoot::VDetector + ;

// detectors
#pragma link C++ class ActRoot::TPCDetector + ;
#pragma link C++ class ActRoot::SilDetector;
#pragma link C++ class ActRoot::ModularDetector;
#pragma link C++ class ActRoot::MergerDetector;

// detector manager
#pragma link C++ class ActRoot::DetectorManager;


// data for detectors
#pragma link C++ class ActRoot::Voxel + ;
#pragma link C++ class ActRoot::Line + ;
#pragma link C++ class ActRoot::Region + ;
#pragma link C++ class ActRoot::Cluster + ;

// parameters

// input iterators and wrapper
#pragma link C++ class ActRoot::InputIterator;
#pragma link C++ class ActRoot::InputWrapper;

#endif
