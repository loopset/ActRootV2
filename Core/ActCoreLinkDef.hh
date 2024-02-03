#include <string>

#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// add link for namespace
#pragma link C++ namespace ActRoot;

// legacy data structures
#pragma link C++ class ReducedData + ;
#pragma link C++ class MEventReduced + ;

// virtual detector
#pragma link C++ class ActRoot::VDetector + ;

// virtual data structure
#pragma link C++ class ActRoot::VData + ;

// pure virtual parameters structure
#pragma link C++ class ActRoot::VParameters + ;

// options manager
#pragma link C++ class ActRoot::Options;

// type definitions
#pragma link C++ enum ActRoot::DetectorType + ;
#pragma link C++ enum ActRoot::ModeType + ;

// tpc
#pragma link C++ class ActRoot::TPCDetector + ;
#pragma link C++ class ActRoot::Voxel + ;
#pragma link C++ class ActRoot::TPCData + ;

#pragma link C++ class ActRoot::DetectorManager;
#pragma link C++ class ActRoot::InputData;
#pragma link C++ class ActRoot::OutputData;
#pragma link C++ class ActRoot::JoinData;
#pragma link C++ class ActRoot::InputBlock;
#pragma link C++ class ActRoot::InputParser;

// calibration manager
#pragma link C++ class ActRoot::CalibrationManager;

// event painter
#pragma link C++ class ActRoot::EventPainter;
#pragma link C++ class ActRoot::HistogramPainter;

// input data utilities
#pragma link C++ class ActRoot::InputIterator;
#pragma link C++ class ActRoot::InputWrapper;

// silicons
#pragma link C++ class ActRoot::SilData + ;
#pragma link C++ class ActRoot::SilDetector;

// modular detector
#pragma link C++ class ActRoot::ModularData + ;
#pragma link C++ class ActRoot::ModularDetector;

// merger detector
#pragma link C++ class ActRoot::MergerDetector;
#pragma link C++ class ActRoot::MergerData + ;

// correction detector
#pragma link C++ class ActRoot::CorrDetector;

// utils: cuts manager
#pragma link C++ class ActRoot::CutsManager < int>;
#pragma link C++ class ActRoot::CutsManager < std::string>;

// MTExecutor
#pragma link C++ class ActRoot::MTExecutor;

// utils function
#pragma link C++ function ActRoot::IsEqZero;

#endif
