#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

#pragma link C++ class ActCluster::Cluster+;
#pragma link C++ class ActCluster::Interval<int>+;
#pragma link C++ class ActCluster::Interval<float>+;
#pragma link C++ class ActCluster::IntervalMap<int>+;
#pragma link C++ class ActCluster::IntervalMap<float>+;

#pragma link C++ class ActCluster::RANSAC;
#pragma link C++ class ActCluster::ClIMB;
#pragma link C++ class ActCluster::MultiStep;

#endif
