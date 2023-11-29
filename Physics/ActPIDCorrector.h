#ifndef ActPIDCorrector_h
#define ActPIDCorrector_h

#include "TH2.h"
#include "TProfile.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ActPhysics
{
    class PIDCorrection
    {
    private:
        std::string fName {};
        double fOffset {-1};
        double fSlope {-1};

    public:
        PIDCorrection() = default;
        PIDCorrection(const std::string& name, double off, double slope);

        std::string GetName() const { return fName; }
        void Print() const;
        double Apply(double q, double spz);
        void Write(const std::string& file);
    };
    class PIDCorrector
    {
    private:
        std::unordered_map<std::string, TH2*> fHistos {};
        std::unordered_map<std::string, TProfile*> fProfs {};
        std::string fName {};

    public:
        PIDCorrector(const std::string& name, const std::vector<std::string> keys, TH2* hModel);

        void FillHisto(const std::string& key, double z, double q, double silE, double minE, double maxE);
        void GetProfiles();
        void FitProfiles(double xmin = 0, double xmax = 0, const std::vector<std::string> keys = {});
        void Draw();
        PIDCorrection GetCorrection(const std::string& key = "");
    };
} // namespace ActPhysics

#endif // !ActPIDCorrector_h
