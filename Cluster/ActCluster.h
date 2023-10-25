#ifndef ActCluster_h
#define ActCluster_h

#include "ActLine.h"
#include "ActTPCData.h"

#include "Math/Point3Dfwd.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>
namespace ActCluster
{
    //! Basic class containing Line + Collection of voxels belonging to the cluster
    class Cluster
    {
    public:
        using XYZPoint = ROOT::Math::XYZPointF;

    private:
        ActPhysics::Line fLine {};
        std::vector<ActRoot::Voxel> fVoxels {};
        std::set<float> fXSet {};
        std::set<float> fYSet {};
        std::set<float> fZSet {};
        std::map<float, int> fYMap {};
        std::map<float, std::set<float>> fXYMap {};
        std::map<float, std::set<float>> fXZMap {};
        int fClusterID {};
        bool fIsBeamLike {};

    public:
        Cluster() = default;
        Cluster(int id);
        Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels);
        ~Cluster() = default;

        // Getters and setters
        const ActPhysics::Line& GetLine() const { return fLine; }
        const std::vector<ActRoot::Voxel>& GetVoxels() const { return fVoxels; }
        std::vector<ActRoot::Voxel>& GetRefToVoxels() { return fVoxels; }
        int GetSizeOfVoxels() const { return fVoxels.size(); }
        int GetClusterID() const { return fClusterID; }
        bool GetIsBeamLike() const { return fIsBeamLike; }

        void SetLine(const ActPhysics::Line& line) { fLine = line; }
        void SetVoxels(const std::vector<ActRoot::Voxel>& voxels) { fVoxels = voxels; }
        void SetClusterID(int id) { fClusterID = id; }
        void SetBeamLike(bool isBeam) { fIsBeamLike = isBeam; }

        // Adders of voxels
        void AddVoxel(const ActRoot::Voxel& voxel); //! By copy in push_back
        void AddVoxel(ActRoot::Voxel&& voxel);      //! By moving in push_back

        // Get extents
        std::pair<float, float> GetXRange() const { return {*fXSet.begin(), *fXSet.rbegin()}; }
        std::pair<float, float> GetYRange() const { return {*fYSet.begin(), *fYSet.rbegin()}; }
        std::pair<float, float> GetZRange() const { return {*fZSet.begin(), *fZSet.rbegin()}; }
        const std::map<float, int>& GetYMap() const { return fYMap; }
        const std::map<float, std::set<float>> GetXYMap() const {return fXYMap;};
        const std::map<float, std::set<float>> GetXZMap() const {return fXZMap;};

        XYZPoint GetGravityPointInRegion(double xmin, double xmax, double ymin = -1, double ymax = -1, double zmin = -1,
                                         double zmax = -1);

        XYZPoint GetGravityPointInXRange(double length); //! Compute grav point given percent of current XRange

        void ReFit();      //! Fit or redo fit of current voxels
        void ReFillSets(); //! Refill sets after an external operation modifies them


        // Display info function
        void Print() const;

    private:
        void FillSets(const ActRoot::Voxel& voxel);
        void FillSets();
    };

} // namespace ActCluster

#endif
