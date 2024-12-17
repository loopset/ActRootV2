#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// namespace
#pragma link C++ namespace ActAlgorithm;

// Enum class
#pragma link C++ enum ActRoot::RegionType + ;

// Cluster algorithms
#pragma link C++ class ActAlgorithm::VCluster;
#pragma link C++ class ActAlgorithm::RANSAC;
#pragma link C++ class ActAlgorithm::ClIMB;

// Utility classes
#pragma link C++ class ActAlgorithm::Interval < int> + ;
#pragma link C++ class ActAlgorithm::Interval < float> + ;
#pragma link C++ class ActAlgorithm::IntervalMap < int> + ;
#pragma link C++ class ActAlgorithm::IntervalMap < float> + ;

// Filter algorithms
#pragma link C++ class ActAlgorithm::VFilter;
#pragma link C++ class ActAlgorithm::MultiStep;
#pragma link C++ class ActAlgorithm::MultiRegion;
#pragma link C++ class ActAlgorithm::Corrector;
#pragma link C++ class ActAlgorithm::MultiAction;

// Action algorithms
#pragma link C++ class ActAlgorithm::VAction;

#pragma link C++ namespace ActAlgorithm::Actions;
#pragma link C++ class ActAlgorithm::Actions::Clean;
#pragma link C++ class ActAlgorithm::Actions::BreakChi2;
#pragma link C++ class ActAlgorithm::Actions::CleanPileUp;
#pragma link C++ class ActAlgorithm::Actions::Merge;
#pragma link C++ class ActAlgorithm::Actions::CleanDeltas;
#pragma link C++ class ActAlgorithm::Actions::CleanZs;
#pragma link C++ class ActAlgorithm::Actions::CleanBadFits;
#pragma link C++ class ActAlgorithm::Actions::FindRP;
#endif
