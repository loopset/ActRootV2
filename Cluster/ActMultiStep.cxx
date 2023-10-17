#include "ActMultiStep.h"

#include "ActCluster.h"
#include "ActColors.h"
#include "ActInputParser.h"

#include "TEnv.h"
#include "TSystem.h"

#include <string>
#include <vector>

ActCluster::MultiStep::MultiStep(std::vector<ActCluster::Cluster>* clusters)
    : fClusters(clusters)
{
}

void ActCluster::MultiStep::ReadConfigurationFile(const std::string& infile)
{
    // automatically get project path from gEnv
    std::string envfile {gEnv->GetValue("ActRoot.ProjectHomeDir", "")};
    envfile += "/configs/filter.climb";
    std::string realfile {};
    if(!gSystem->AccessPathName(envfile.c_str()))
        realfile = envfile;
    else if(infile.length() > 0)
        realfile = infile;
    else
    {
        std::cout << BOLDMAGENTA << ".ransac config file not found -> Using built-in configuration" << '\n';
        return;
    }

    // Parse!
    ActRoot::InputParser parser {realfile};
    auto mb {parser.GetBlock("MultiStep")};
    if(mb->CheckTokenExists("ChiSquareThresh"))
        ;
}
