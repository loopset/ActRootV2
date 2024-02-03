#ifndef ActSilMatrix_h
#define ActSilMatrix_h

#include "RtypesCore.h"

#include "TCutG.h"

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
        std::string fName {};
        std::map<int, TCutG*> fMatrix;
        bool fIsStyleSet {};

    public:
        SilMatrix() = default;
        SilMatrix(const std::string& name) : fName(name) {}

        void AddSil(int idx, const std::pair<double, double>& x, const std::pair<double, double>& y);
        bool IsInside(int idx, double x, double y);
        std::optional<int> IsInside(double x, double y);

        void SetSyle(bool enableLabel = true, Style_t ls = kSolid, Width_t lw = 2, Style_t fs = 0);
        void SetName(const std::string& name) { fName = name; }
        std::string GetName() const { return fName; }
        std::set<int> GetSilIndexes() const;

        void Draw(bool same = true, const std::string& xlabel = "Y [mm]", const std::string& ylabel = "Z [mm]");
        void MoveZTo(double ztarget, const std::set<int>& idxs);

        void Read(const std::string& file);
        void Write(const std::string& file);

        void Print() const;

    private:
        void InitGraph(int idx);
        void AddPoint(int idx, double x, double y);
    };
} // namespace ActPhysics
#endif // !ActSilMatrix_h
