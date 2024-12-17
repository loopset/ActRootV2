#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// add link for namespace
#pragma link C++ namespace ActRoot;

// functions
#pragma link C++ function ActRoot::IsEqZero;

// type definitions
#pragma link C++ enum ActRoot::DetectorType + ;
#pragma link C++ enum ActRoot::ModeType + ;

// data manager, inputs and outputs
#pragma link C++ class ActRoot::DataManager;
#pragma link C++ class ActRoot::InputData;
#pragma link C++ class ActRoot::OutputData;

// options manager
#pragma link C++ class ActRoot::Options;

// file parsers
#pragma link C++ class ActRoot::InputBlock;
#pragma link C++ class ActRoot::InputParser;

// calibration manager
#pragma link C++ class ActRoot::CalibrationManager;

// Base classes
// virtual detector
#pragma link C++ class ActRoot::VDetector + ;

// virtual data
#pragma link C++ class ActRoot::VData + ;

// virtual parameters
#pragma link C++ class ActRoot::VParameters + ;

// base type shared among all sublibraries
#pragma link C++ class ActRoot::Voxel + ;

// Thomas' legacy data structures
#pragma link C++ class ReducedData + ;
#pragma link C++ class MEventReduced + ;

#endif
