#ifndef ActSilParameters_h
#define ActSilParameters_h

#include "ActVParameters.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
namespace ActRoot
{
//! A class holding basic silicon parameters
/*!
  For now, just keeps sizes of silicons
  using strings as identifiers and VXI equivalences
*/
class SilParameters : public VParameters
{
private:
    std::unordered_map<std::string, int> fSizes; //!< Sizes of silicons based on string, for [0, max]
    std::map<int, std::pair<std::string, int>> fVXI;

public:
    // Setters
    void SetLayer(const std::string& key, int size) { fSizes[key] = size; }
    // Getters
    std::vector<std::string> GetKeys() const;
    int GetSizeOf(const std::string& key) { return fSizes[key]; }
    void Print() const override; //!< Dump info stored
    std::pair<std::string, int> GetSilIndex(int vxi);
    void
    ReadActions(const std::vector<std::string>& layers, const std::vector<std::string>& names, const std::string& file);
};
} // namespace ActRoot
#endif
