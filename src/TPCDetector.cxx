#include "TPCDetector.h"

#include "InputParser.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

void ActRoot::TPCDetector::ReadConfiguration(std::shared_ptr<InputBlock> config)
{
    std::vector<std::string> keys {"Type"};
    std::cout<<"Read ActTPCDetector Type = "<<config->GetString("Type")<<'\n';
}

void ActRoot::TPCDetector::InitInputRawData()
{
    std::cout<<"Initializing TPC input raw data data"<<'\n';
}
