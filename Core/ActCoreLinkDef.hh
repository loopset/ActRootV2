#include <string>

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

// Thomas' legacy data structures
#pragma link C++ class ReducedData + ;
#pragma link C++ class MEventReduced + ;

// type definitions
#pragma link C++ enum ActRoot::DetectorType + ;
#pragma link C++ enum ActRoot::ModeType + ;

// virtual detector
#pragma link C++ class ActRoot::VDetector + ;

// detectors
#pragma link C++ class ActRoot::TPCDetector + ;
#pragma link C++ class ActRoot::SilDetector;
#pragma link C++ class ActRoot::ModularDetector;
#pragma link C++ class ActRoot::MergerDetector;
#pragma link C++ class ActRoot::CorrDetector;

// detector manager
#pragma link C++ class ActRoot::DetectorManager;


// virtual data
#pragma link C++ class ActRoot::VData + ;

// data for detectors
#pragma link C++ class ActRoot::Voxel + ;
#pragma link C++ class ActRoot::Cluster + ;
#pragma link C++ class ActRoot::TPCData + ;
#pragma link C++ class ActRoot::SilData + ;
#pragma link C++ class ActRoot::ModularData + ;
#pragma link C++ class ActRoot::MergerData + ;

// data manager, inputs and outputs
#pragma link C++ class ActRoot::DataManager;
#pragma link C++ class ActRoot::InputData;
#pragma link C++ class ActRoot::OutputData;

// virtual parameters
#pragma link C++ class ActRoot::VParameters + ;

// parameters
#pragma link C++ class ActRoot::TPCParameters + ;
#pragma link C++ class ActRoot::SilParameters + ;
#pragma link C++ class ActRoot::ModularParameters + ;
#pragma link C++ class ActRoot::MergerParameters + ;

// options manager
#pragma link C++ class ActRoot::Options;

// file parsers
#pragma link C++ class ActRoot::InputBlock;
#pragma link C++ class ActRoot::InputParser;

// calibration manager
#pragma link C++ class ActRoot::CalibrationManager;

// input iterator
#pragma link C++ class ActRoot::InputIterator;
#pragma link C++ class ActRoot::InputWrapper;

#endif
