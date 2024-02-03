#ifndef ActOptions_h
#define ActOptions_h

#include "ActTypes.h"

#include <string>
#include <unordered_map>

namespace ActRoot
{
class Options
{
private:
    // mode conversion
    static std::unordered_map<ModeType, std::string> fModeTable;
    bool fIsMT {};
    std::string fProjDir {};
    std::string fDetFile {};
    std::string fCalFile {};
    std::string fInFile {};
    std::string fOutFile {};
    ModeType fMode {ModeType::ENone};

public:
    Options() = default;
    Options(int argc, char** argv);

    // Getters
    ModeType GetMode() const { return fMode; }
    std::string GetModeStr() const { return fModeTable[fMode]; }
    static std::string GetModeStr(ModeType mode) { return fModeTable[mode]; }
    ModeType ConvertToMode(const std::string& mode);
    bool GetIsMT() const { return fIsMT; }
    std::string GetDetFile() const { return fDetFile; }
    std::string GetCalFile() const { return fCalFile; }
    std::string GetInputFile() const { return fInFile; }
    std::string GetOutputFile() const { return fOutFile; }
    std::string GetProjectDir() const;

    // Setters
    void SetMode(ModeType mode) { fMode = mode; }
    void SetIsMT(bool mt) { fIsMT = mt; }
    void SetDetFile(const std::string& file) { fDetFile = file; }
    void SetCalFile(const std::string& file) { fCalFile = file; }
    void SetInputFile(const std::string& file) { fInFile = file; }
    void SetOutputFile(const std::string& file) { fOutFile = file; }

    // Others
    void Help() const;
    void Print() const;

private:
    void Parse(int argc, char** argv);
};
} // namespace ActRoot

#endif // !ActOptions_h
