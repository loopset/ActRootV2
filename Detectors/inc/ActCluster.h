#ifndef ActCluster_h
#define ActCluster_h

#include "ActLine.h"
#include "ActRegion.h"
#include "ActVoxel.h"

#include "Math/Point3Dfwd.h"

#include <map>
#include <set>
#include <utility>
#include <vector>

namespace ActRoot
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
    std::map<int, std::set<int>> fXYMap {}; //!
    std::map<int, std::set<int>> fXZMap {}; //!
    int fClusterID {};
    bool fIsBeamLike {false};
    bool fIsRecoil {false};
    bool fToMerge {true};
    bool fToDelete {false};
    bool fIsBreak {false};
    bool fIsSplit {false};
    bool fHasRP {false};
    ActAlgorithm::RegionType fRegion {ActAlgorithm::RegionType::ENone};

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
    std::vector<ActRoot::Voxel>* GetPtrToVoxels() { return &fVoxels; }
    int GetSizeOfVoxels() const { return fVoxels.size(); }
    int GetClusterID() const { return fClusterID; }
    bool GetIsBeamLike() const { return fIsBeamLike; }
    bool GetIsRecoil() const { return fIsRecoil; }
    bool GetToMerge() const { return fToMerge; }
    bool GetToDelete() const { return fToDelete; }
    bool GetIsBreakBeam() const { return fIsBreak; }
    bool GetIsSplitRP() const { return fIsSplit; }
    ActAlgorithm::RegionType GetRegionType() const { return fRegion; }
    bool GetHasRP() const { return fHasRP; }

    void SetLine(const ActPhysics::Line& line) { fLine = line; }
    void SetVoxels(const std::vector<ActRoot::Voxel>& voxels) { fVoxels = voxels; }
    void SetVoxels(std::vector<ActRoot::Voxel>&& voxels) { fVoxels = std::move(voxels); }
    void SetClusterID(int id) { fClusterID = id; }
    void SetBeamLike(bool isBeam) { fIsBeamLike = isBeam; }
    void SetIsRecoil(bool isRec) { fIsRecoil = isRec; }
    void SetToMerge(bool toMerge) { fToMerge = toMerge; }
    void SetToDelete(bool toDelete) { fToDelete = toDelete; }
    void SetIsBreakBeam(bool hasValidRP) { fIsBreak = hasValidRP; }
    void SetIsSplitRP(bool split) { fIsSplit = split; }
    void SetRegionType(ActAlgorithm::RegionType type) { fRegion = type; }
    void SetHasRP(bool has) { fHasRP = has; }

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

} // namespace ActRoot

#endif
