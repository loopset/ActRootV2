#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

//legacy data structures

#pragma link C++ class ReducedData + ;
#pragma link C++ class MEventReduced + ;

#pragma link C++ class ActRoot::VDetector + ;
#pragma link C++ class ActRoot::TPCDetector + ;
#pragma link C++ class ActRoot::Voxel + ;
#pragma link C++ class ActRoot::TPCData + ;

#pragma link C++ class ActRoot::DetectorManager;
#pragma link C++ class ActRoot::App;
#pragma link C++ class ActRoot::InputData;
#pragma link C++ class ActRoot::OutputData;
#pragma link C++ enum class ActRoot::DetectorType + ;
#pragma link C++ class ActRoot::InputBlock;
#pragma link C++ class ActRoot::InputParser;

#pragma link C++ class ActRoot::CalibrationManager;

#pragma link C++ class ActRoot::EventPainter;

#pragma link C++ class ActRoot::InputIterator;
#pragma link C++ class ActRoot::InputWrapper;
#endif
