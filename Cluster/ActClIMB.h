#ifndef ActClIMB_h
#define ActClIMB_h

#include "ActCluster.h"
#include "ActVoxel.h"

#include <string>
#include <tuple>
#include <vector>

//forward declaration to avoid circular dependencies
namespace ActRoot
{
    class TPCParameters;
}

namespace ActCluster
{
    //! Implementation of a continuity-based cluster algorithm by J. Lois-Fuentes
    class ClIMB
    {
    private:
        std::vector<std::vector<std::vector<int>>> fMatrix;//!< 3D matrix to locate clusters in space
        std::vector<ActRoot::Voxel> fVoxels;//!< Local copy of vector to be treated
        std::vector<int> fIndexes;
        ActRoot::TPCParameters* fTPC {};//!< Pointer to TPC parameters needed to define algorithm parameters
        int fMinPoints;//!< Minimum of points to form a cluster
    public:
        ClIMB() = default;
        ClIMB(ActRoot::TPCParameters* tpc, int minPoints);
        ~ClIMB() = default;

        //Read config file
        void ReadConfigurationFile(const std::string& infile = "");

        //Setters and getters
        void SetMinPoints(int minPoints){fMinPoints = minPoints;}
        int GetMinPoints() const {return fMinPoints;}
        void SetTPCParameters(ActRoot::TPCParameters* tpc){fTPC = tpc; InitMatrix();}

        //Main method
        std::vector<ActCluster::Cluster> Run(const std::vector<ActRoot::Voxel>& voxels);

        //Print
        void Print() const;

    private:
        void InitMatrix();
        void InitIndexes();
        void FillMatrix();
        std::vector<int> ScanNeighborhood(const std::vector<int>& gen0);
        std::tuple<int, int, int> GetCoordinates(int index);
        void MaskVoxelsInMatrix(int index);
        void MaskVoxelsInIndex(int index);
        template<typename T>
        bool IsInCage(T x, T y, T z);
    };
}

#endif
