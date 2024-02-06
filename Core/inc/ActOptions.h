#ifndef ActOptions_h
#define ActOptions_h

#include "ActTypes.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace ActRoot
{
class Options
{
private:
    static std::shared_ptr<Options> fInstance;

    // mode conversion
    static std::unordered_map<ModeType, std::string> fModeTable;
    bool fIsMT {};
    std::string fProjDir {};
    std::string fDetFile {};
    std::string fCalFile {};
    std::string fInFile {};
    std::string fOutFile {};
    ModeType fMode {ModeType::ENone};
    bool fIsVerbose {};

    // make constructors and copy/move operators private
    Options() = default;
    Options(int argc, char** argv);

public:
    Options(const Options&) = delete;
    void operator=(const Options&) = delete;

    // Main method to get instance
    static std::shared_ptr<Options> GetInstance(int argc = 0, char** = nullptr);

    // Getters
    ModeType GetMode() const { return fMode; }
    std::string GetModeStr() const { return fModeTable[fMode]; }
    static std::string GetModeStr(ModeType mode) { return fModeTable[mode]; }
    ModeType ConvertToMode(const std::string& mode);
    std::string GetDetFile() const { return GetConfigDir() + fDetFile; }
    std::string GetCalFile() const { return GetConfigDir() + fCalFile; }
    std::string GetInputFile() const { return GetConfigDir() + fInFile; }
    std::string GetOutputFile() const { return GetConfigDir() + fOutFile; }
    std::string GetProjectDir() const;
    std::string GetConfigDir() const { return GetProjectDir() + "/configs/"; }
    std::string GetRunFile() const { return GetConfigDir() + fInFile; }
    bool GetIsMT() const { return fIsMT; }
    bool GetIsVerbose() const { return fIsVerbose; }

    // Setters
    void SetMode(ModeType mode) { fMode = mode; }
    void SetDetFile(const std::string& file) { fDetFile = file; }
    void SetCalFile(const std::string& file) { fCalFile = file; }
    void SetInputFile(const std::string& file) { fInFile = file; }
    void SetOutputFile(const std::string& file) { fOutFile = file; }
    void SetIsMT(bool mt) { fIsMT = mt; }
    void SetIsVerbose() { fIsVerbose = !fIsVerbose; }

    // Others
    void Help() const;
    void Print() const;

private:
    void Parse(int argc, char** argv);
};
} // namespace ActRoot

#endif // !ActOptions_h
