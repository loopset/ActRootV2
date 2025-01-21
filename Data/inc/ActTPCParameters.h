#ifndef ActTPCParameters_h
#define ActTPCParameters_h

#include "ActVParameters.h"

#include <string>

namespace ActRoot
{
class TPCParameters : public VParameters
{
private:
    double fPadSide {2}; // mm
    int fNPADSX;
    int fNPADSY;
    int fNPADSZ;
    int fREBINZ {1};
    int fNB_COBO {18};
    int fNB_ASAD {4};
    int fNB_AGET {4};
    int fNB_CHANNEL {68};
    // And contain also real sizes in mm
    double fX {};
    double fY {};
    double fZ {};

public:
    TPCParameters() = default;
    TPCParameters(const std::string& type);
    // Setters
    void SetREBINZ(int rebin);
    void SetNPADSZ(int npadsz) { fNPADSZ = npadsz; }
    // Getters
    double GetPadSide() const { return fPadSide; }
    int GetNPADSX() const { return fNPADSX; }
    int GetNPADSY() const { return fNPADSY; }
    int GetNPADSZUNREBIN() const { return fNPADSZ; }
    int GetNPADSZ() const { return fNPADSZ / fREBINZ; }
    int GetREBINZ() const { return fREBINZ; }
    int GetNBCOBO() const { return fNB_COBO; }
    int GetNBASAD() const { return fNB_ASAD; }
    int GetNBAGET() const { return fNB_AGET; }
    int GetNBCHANNEL() const { return fNB_CHANNEL; }
    double GetX() const { return fX; }
    double X() const { return GetX(); }
    double GetY() const { return fY; }
    double Y() const { return GetY(); }
    double GetZ() const { return fZ; }
    double Z() const { return GetZ(); }

    void Print() const override;
};
} // namespace ActRoot
#endif
