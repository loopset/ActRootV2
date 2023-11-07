#ifndef ActCluster_h
#define ActCluster_h

#include "ActLine.h"
#include "ActTPCData.h"

#include "Math/Point3D.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
namespace ActCluster
{
    //! Basic class containing Line + Collection of voxels belonging to the cluster
    class Cluster
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;
        typedef std::pair<float, float> RangeType;

    private:
        // Line with fit parameters
        ActPhysics::Line fLine {};
        // Voxels belonging to clusters
        std::vector<ActRoot::Voxel> fVoxels {};
        // Ranges of each dimension
        RangeType fXRange {1111, -1};
        RangeType fYRange {1111, -1};
        RangeType fZRange {1111, -1};
        std::map<int, std::set<int>> fXYMap {};
        std::map<int, std::set<int>> fXZMap {};
        int fClusterID {};
        bool fIsBeamLike {false};
        bool fToMerge {true};
        bool fToDelete {false};
        bool fHasValidRP {false};

    public:
        Cluster() = default;
        Cluster(int id);
        Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels);
        ~Cluster() = default;

        // Getters and setters
        const ActPhysics::Line& GetLine() const { return fLine; }
        ActPhysics::Line& GetRefToLine() { return fLine; } // non-const: allows to change inner variable
        const std::vector<ActRoot::Voxel>& GetVoxels() const { return fVoxels; }
        std::vector<ActRoot::Voxel>& GetRefToVoxels() { return fVoxels; } // non-const: allows to change inner variable
        int GetSizeOfVoxels() const { return fVoxels.size(); }
        int GetClusterID() const { return fClusterID; }
        bool GetIsBeamLike() const { return fIsBeamLike; }
        bool GetToMerge() const { return fToMerge; }
        bool GetToDelete() const { return fToDelete; }
        bool GetHasValidRP() const { return fHasValidRP; }

        void SetLine(const ActPhysics::Line& line) { fLine = line; }
        void SetVoxels(const std::vector<ActRoot::Voxel>& voxels) { fVoxels = voxels; }
        void SetClusterID(int id) { fClusterID = id; }
        void SetBeamLike(bool isBeam) { fIsBeamLike = isBeam; }
        void SetToMerge(bool toMerge) { fToMerge = toMerge; }
        void SetToDelete(bool toDelete) { fToDelete = toDelete; }
        void SetHasValidRP(bool hasValidRP) { fHasValidRP = hasValidRP; }

        // Adders of voxels
        void AddVoxel(const ActRoot::Voxel& voxel); //! By copy in push_back
        void AddVoxel(ActRoot::Voxel&& voxel);      //! By moving in push_back

        // Get extents
        std::pair<float, float> GetXRange() const;
        std::pair<float, float> GetYRange() const;
        std::pair<float, float> GetZRange() const;
        const std::map<int, std::set<int>>& GetXYMap() const { return fXYMap; };
        const std::map<int, std::set<int>>& GetXZMap() const { return fXZMap; };

        XYZPoint GetGravityPointInRegion(double xmin, double xmax, double ymin = -1, double ymax = -1, double zmin = -1,
                                         double zmax = -1);

        XYZPoint GetGravityPointInXRange(double length); //! Compute grav point given percent of current XRange

        void ReFit();      //! Fit or redo fit of current voxels
        void ReFillSets(); //! Refill sets after an external operation modifies them

        // Display info function
        void Print() const;

    private:
        void UpdateRange(float val, RangeType& range);
        void FillSets(const ActRoot::Voxel& voxel);
        void FillSets();
    };

} // namespace ActCluster

#endif
