#include <string>

#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

// cuts manager
#pragma link C++ class ActRoot::CutsManager <int>;
#pragma link C++ class ActRoot::CutsManager <string>;

// multithreading
#pragma link C++ class ActRoot::MTExecutor;

// ActRoot's GUI
#pragma link C++ class ActRoot::EventPainter;
#pragma link C++ class ActRoot::HistogramPainter;

#endif
