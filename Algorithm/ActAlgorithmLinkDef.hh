#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// Cluster algorithms
#pragma link C++ class ActCluster::VCluster;
#pragma link C++ class ActCluster::RANSAC;
#pragma link C++ class ActCluster::ClIMB;

#pragma link C++ class ActCluster::Interval < int> + ;
#pragma link C++ class ActCluster::Interval < float> + ;
#pragma link C++ class ActCluster::IntervalMap < int> + ;
#pragma link C++ class ActCluster::IntervalMap < float> + ;

// Filter algorithms

#pragma link C++ class ActCluster::VFilter;
#pragma link C++ class ActCluster::MultiStep;
#pragma link C++ class ActCluster::Corrector;

#endif
