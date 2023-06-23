#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

#pragma link C++ class ActRoot::VDetector + ;
#pragma link C++ class ActRoot::TPCDetector + ;
#pragma link C++ class ActRoot::Voxel + ;
#pragma link C++ class ActRoot::ReducedData + ;
#pragma link C++ class ActRoot::MEventReduced + ;
#pragma link C++ class ActRoot::TPCData + ;

#pragma link C++ class ActRoot::DetectorManager;
#pragma link C++ class ActRoot::App;
#pragma link C++ class ActRoot::InputData;
#pragma link C++ enum class ActRoot::DetectorType + ;
#pragma link C++ class ActRoot::InputBlock;
#pragma link C++ class ActRoot::InputParser;

#endif
