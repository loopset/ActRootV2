#ifndef ActCluster_h
#define ActCluster_h

#include "ActLine.h"
#include "ActTPCData.h"
#include "Math/Point3Dfwd.h"

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
        int fClusterID {};

    public:
        Cluster() = default;
        Cluster(int id, const ActPhysics::Line& line, const std::vector<ActRoot::Voxel>& voxels);
        ~Cluster() = default;

        //Getters and settes
        const ActPhysics::Line& GetLine() const {return fLine;}
        const std::vector<ActRoot::Voxel> GetVoxels() const {return fVoxels;}
        int GetClusterID() const {return fClusterID;}
        
        void SetLine(const ActPhysics::Line& line) {fLine = line;}
        void SetVoxels(const std::vector<ActRoot::Voxel>& voxels){fVoxels = voxels;}
        void SetClusterID(int id){fClusterID = id;}

        //Basic funtions to MultiStep algorithm
        XYZPoint GetGravityPoint();
    };

}

#endif
