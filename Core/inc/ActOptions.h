#ifndef ActOptions_h
#define ActOptions_h

#include "ActTypes.h"

#include "TString.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace ActRoot
{
class Options
{
private:
    // Singleton model
    static std::shared_ptr<Options> fInstance;

    // Mode to string conversion
    static std::unordered_map<ModeType, std::string> fModeTable;

    // ActRoot options (with default values)
    bool fIsMT {true};
    std::string fProjDir {};
    std::string fDetFile {"detector.conf"};
    std::string fCalFile {"calibration.conf"};
    std::string fDataFile {"data.conf"};
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
    std::string GetDataFile() const { return GetConfigDir() + fDataFile; }
    std::string GetProjectDir() const;
    std::string GetConfigDir() const { return GetProjectDir() + "/configs/"; }
    bool GetIsMT() const { return fIsMT; }
    bool GetIsVerbose() const { return fIsVerbose; }

    // Setters
    void SetMode(ModeType mode) { fMode = mode; }
    void SetDetFile(const std::string& file) { fDetFile = file; }
    void SetCalFile(const std::string& file) { fCalFile = file; }
    void SetDataFile(const std::string& file) { fDataFile = file; }
    void SetIsMT(bool mt) { fIsMT = mt; }
    void SetIsVerbose(bool verb = true) { fIsVerbose = verb; }

    // Others
    void Help() const;
    void Print() const;

private:
    void Parse(int argc, char** argv);
    void CheckConfigDirectory();
    ModeType ReadFlagToMode(TString flag);
};
} // namespace ActRoot

#endif // !ActOptions_h
