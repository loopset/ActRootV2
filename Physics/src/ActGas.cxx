#include "ActGas.h"

#include "ActInputParser.h"

#include <iostream>
#include <memory>

void ActPhysics::Gas::ReadConfiguration(std::shared_ptr<ActRoot::InputBlock> block)
{
    fWorkFunc = block->GetDouble("WorkFunc");
    fVDrift = block->GetDouble("VDrift");
    fTransDiff = block->GetDouble("TransDiff");
}

void ActPhysics::Gas::ReadConfiguration(const std::string& file)
{
    ActRoot::InputParser parser {file};
    ReadConfiguration(parser.GetBlock("Gas"));
}

void ActPhysics::Gas::Print() const
{
    std::cout << "---- Physics::Gas ----" << '\n';
    std::cout << "-> WorkFunc : " << fWorkFunc << " eV" << '\n';
    std::cout << "-> VDrift   : " << fVDrift << " mm / us" << '\n';
    std::cout << "-> TransDiff: " << fTransDiff << " um / cm" << '\n';
}
