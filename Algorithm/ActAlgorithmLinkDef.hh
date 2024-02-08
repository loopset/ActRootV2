#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// Cluster algorithms
#pragma link C++ class ActAlgorithm::VCluster;
#pragma link C++ class ActAlgorithm::RANSAC;
#pragma link C++ class ActAlgorithm::ClIMB;

#pragma link C++ class ActAlgorithm::Interval < int> + ;
#pragma link C++ class ActAlgorithm::Interval < float> + ;
#pragma link C++ class ActAlgorithm::IntervalMap < int> + ;
#pragma link C++ class ActAlgorithm::IntervalMap < float> + ;

// Filter algorithms
#pragma link C++ class ActAlgorithm::VFilter;
#pragma link C++ class ActAlgorithm::MultiStep;
#pragma link C++ class ActAlgorithm::Corrector;

#endif
