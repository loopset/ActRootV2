#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// add link for namespace
#pragma link C++ namespace ActRoot;

// Thomas' legacy data structures
#pragma link C++ class ReducedData + ;
#pragma link C++ class MEventReduced + ;

// type definitions
#pragma link C++ enum ActRoot::DetectorType + ;
#pragma link C++ enum ActRoot::ModeType + ;

// detectors
#pragma link C++ class ActRoot::TPCDetector + ;
#pragma link C++ class ActRoot::SilDetector;
#pragma link C++ class ActRoot::ModularDetector;
#pragma link C++ class ActRoot::MergerDetector;

// detector manager
#pragma link C++ class ActRoot::DetectorManager;

// data for detectors
#pragma link C++ class ActRoot::Cluster + ;
#pragma link C++ class ActRoot::TPCData + ;
#pragma link C++ class ActRoot::SilData + ;
#pragma link C++ class ActRoot::ModularData + ;
#pragma link C++ class ActRoot::MergerData + ;

// parameters
#pragma link C++ class ActRoot::TPCParameters + ;
#pragma link C++ class ActRoot::SilParameters + ;
#pragma link C++ class ActRoot::ModularParameters + ;
#pragma link C++ class ActRoot::MergerParameters + ;

// input iterators and wrapper
#pragma link C++ class ActRoot::InputIterator;
#pragma link C++ class ActRoot::InputWrapper;

#endif
