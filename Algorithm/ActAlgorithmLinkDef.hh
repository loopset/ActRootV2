#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// namespace
#pragma link C++ namespace ActAlgorithm;

// Enum class
#pragma link C++ enum ActAlgorithm::RegionType + ;

// Cluster algorithms
#pragma link C++ class ActAlgorithm::VCluster;
#pragma link C++ class ActAlgorithm::RANSAC;
#pragma link C++ class ActAlgorithm::ClIMB;

// Utility classes
#pragma link C++ class ActAlgorithm::Interval < int> + ;
#pragma link C++ class ActAlgorithm::Interval < float> + ;
#pragma link C++ class ActAlgorithm::IntervalMap < int> + ;
#pragma link C++ class ActAlgorithm::IntervalMap < float> + ;

#pragma link C++ class ActAlgorithm::Region + ;

// Filter algorithms
#pragma link C++ class ActAlgorithm::VFilter;
#pragma link C++ class ActAlgorithm::MultiStep;
#pragma link C++ class ActAlgorithm::MultiRegion;
#pragma link C++ class ActAlgorithm::Corrector;

#endif
