#ifndef ActSilMatrix_h
#define ActSilMatrix_h

#include "ActUtils.h"

#include "RtypesCore.h"

#include "TCutG.h"
#include "TMultiGraph.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace ActPhysics
{
class SilMatrix
{
private:
    TMultiGraph* fMulti {}; //!
    std::string fName {};
    std::map<int, TCutG*> fMatrix {};
    int fPadIdx {};
    bool fIsStyleSet {};

public:
    SilMatrix() = default;
    SilMatrix(const std::string& name) : fName(name) {}
    ~SilMatrix();

    void AddSil(int idx, const std::pair<double, double>& x, const std::pair<double, double>& y);
    void AddSil(int idx, TCutG* g) { fMatrix[idx] = g; }
    bool IsInside(int idx, double x, double y);
    std::optional<int> IsInside(double x, double y);
    bool IsInMatrix(int idx) const { return fMatrix.count(idx); }

    void SetSyle(bool enableLabel = true, Style_t ls = kSolid, Width_t lw = 2, Style_t fs = 0);
    void SetName(const std::string& name) { fName = name; }
    void SetPadIdx(int idx) { fPadIdx = idx; }
    std::string GetName() const { return fName; }
    std::set<int> GetSilIndexes() const;
    TCutG* GetSil(int idx) const { return fMatrix.at(idx); }
    const std::map<int, TCutG*>& GetGraphs() const { return fMatrix; }
    std::pair<double, double> GetCentre(int idx) const;
    double GetWidth(int idx) const;
    double GetHeight(int idx) const;
    int GetPadIdx() const { return fPadIdx; }

    TMultiGraph* Draw(bool same = true, const std::string& xlabel = "Y [mm]", const std::string& ylabel = "Z [mm]");
    void MoveZTo(double ztarget, const std::set<int>& idxs);
    void MoveXYTo(double xRef, const std::pair<double, double>& yzCentre, double xTarget);
    double GetMeanZ(const std::set<int>& idxs);
    void Erase(int idx);

    void Read(const std::string& file);
    void Write(const std::string& file);
    void Write();

    void Print() const;

    SilMatrix* Clone() const;
    TMultiGraph* DrawClone(bool same = true);

    // Funtions to be used in HistogramPainter
    void DrawForPainter();
    void CreateMultiGraphForPainter();

private:
    void InitGraph(int idx);
    void AddPoint(int idx, double x, double y);
};
} // namespace ActPhysics
#endif // !ActSilMatrix_h
