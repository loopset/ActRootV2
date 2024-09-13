#include "ActVAction.h"

#include "ActOptions.h"

#include <string>

ActAlgorithm::VAction::VAction(const std::string& actionID)
    : fActionID(actionID),
      fIsVerbose(ActRoot::Options::GetInstance()->GetIsVerbose())
{
}
