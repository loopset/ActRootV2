#ifndef ActTPCLegacyData_h
#define ActTPCLegacyData_h

/*
  This class just holds the MEventReduced and ReducedData objects
*/
#include "Rtypes.h"
#include "TObject.h"
#include <vector>

class ReducedData: public TObject
{
public:
    ReducedData(){hasSaturation = false;};
    ~ReducedData(){};
    unsigned short globalchannelid;
    bool hasSaturation;
    std::vector<float> peakheight;
    std::vector<float> peaktime;
    ClassDef(ReducedData, 1);
};

class MEventReduced: public TObject
{
public:
    MEventReduced(){};
    ~MEventReduced(){};
	unsigned long int event;
	unsigned long int timestamp;
	std::vector<ReducedData> CoboAsad;
    ClassDef(MEventReduced, 1);
};


#endif
