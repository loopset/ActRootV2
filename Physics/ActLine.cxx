#include "ActLine.h"

#include "ActColors.h"
#include "ActTPCData.h"

#include "TEnv.h"
#include "TMath.h"
#include "TPolyLine.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

ActPhysics::Line::Line(XYZPoint point, XYZVector direction, float chi)
    : fPoint(point),
      fDirection(direction),
      fChi2(chi)
{
}

ActPhysics::Line::Line(const XYZPoint& p1, const XYZPoint& p2)
{
    SetPoint(p1);
    SetDirection(p1, p2);
}

double ActPhysics::Line::DistanceLineToPoint(const XYZPoint& point) const
{
    auto vec = point - fPoint;
    auto nD = fDirection.Cross(vec);
    double dist2 = nD.Mag2() / fDirection.Mag2();

    return std::sqrt(dist2);
}

void ActPhysics::Line::FitVoxels(const std::vector<ActRoot::Voxel>& voxels, bool qWeighted, double qThreshold,
                                 bool correctOffset)
{
    std::vector<XYZPoint> cloud;
    std::vector<double> charge;
    for(const auto& voxel : voxels)
    {
        if(qWeighted)
        {
            if(qThreshold != -1 && !(voxel.GetCharge() > qThreshold))
                continue;
            charge.push_back(voxel.GetCharge());
            cloud.push_back(voxel.GetPositionAs<float>());
        }
        else
            cloud.push_back(voxel.GetPositionAs<float>());
    }
    if(qWeighted)
        FitCloudWithThreshold(cloud, charge, correctOffset);
    else
        FitCloudWithThreshold(cloud, {}, correctOffset);
}

void ActPhysics::Line::FitCloud(const std::vector<XYZPoint>& cloud, bool correctOffset)
{
    FitCloudWithThreshold(cloud, {}, correctOffset);
}

void ActPhysics::Line::FitCloudWithThreshold(const std::vector<XYZPoint>& points, const std::vector<double>& charge,
                                             bool correctOffset)
{
    //------3D Line Regression
    //----- adapted from: http://fr.scribd.com/doc/31477970/Regressions-et-trajectoires-3D
    // Paramount importance: use doubles to avoid floating point errors that lead to nans in dim2!!!!
    // static_cast back to float when writing XYZThings
    // Default-initialize everything!
    double Q {};
    double Xm {};
    double Ym {};
    double Zm {};
    double Xh {};
    double Yh {};
    double Zh {};
    double a {};
    double b {};
    double Sxx {};
    double Sxy {};
    double Syy {};
    double Sxz {};
    double Szz {};
    double Syz {};
    double theta {};
    double K11 {};
    double K22 {};
    double K12 {};
    double K10 {};
    double K01 {};
    double K00 {};
    double c0 {};
    double c1 {};
    double c2 {};
    double p {};
    double q {};
    double r {};
    double dm2 {};
    double rho {};
    double phi {};

    bool doChargeWeight = (points.size() == charge.size());

    for(int i = 0, maxsize = points.size(); i < maxsize; ++i)
    {
        const auto hitQ = doChargeWeight ? charge[i] : 1.;
        auto pos = points[i];
        if(correctOffset)
            pos += XYZVector {0.5, 0.5, 0.5};
        Q += hitQ / 10.;
        Xm += pos.X() * hitQ / 10.;
        Ym += pos.Y() * hitQ / 10.;
        Zm += pos.Z() * hitQ / 10.;
        Sxx += pos.X() * pos.X() * hitQ / 10.;
        Syy += pos.Y() * pos.Y() * hitQ / 10.;
        Szz += pos.Z() * pos.Z() * hitQ / 10.;
        Sxy += pos.X() * pos.Y() * hitQ / 10.;
        Sxz += pos.X() * pos.Z() * hitQ / 10.;
        Syz += pos.Y() * pos.Z() * hitQ / 10.;
    }

    Xm /= Q;
    Ym /= Q;
    Zm /= Q;
    Sxx /= Q;
    Syy /= Q;
    Szz /= Q;
    Sxy /= Q;
    Sxz /= Q;
    Syz /= Q;
    Sxx -= (Xm * Xm);
    Syy -= (Ym * Ym);
    Szz -= (Zm * Zm);
    Sxy -= (Xm * Ym);
    Sxz -= (Xm * Zm);
    Syz -= (Ym * Zm);

    theta = 0.5 * std::atan((2. * Sxy) / (Sxx - Syy));

    // Bugfix: when we have pad saturation, Sxx = Syy = 0 -> theta = nan! Then, we can return a vertical line!
    if(!std::isfinite(theta))
    {
        SetPoint(XYZPoint(static_cast<float>(Xm), static_cast<float>(Ym), static_cast<float>(Zm)));
        SetDirection(XYZVector(0, 0, 1));
        SetSigmas(XYZPoint(0, 0, 0));
        SetChi2(-1);
        return;
    }

    K11 = (Syy + Szz) * std::pow(std::cos(theta), 2) + (Sxx + Szz) * std::pow(std::sin(theta), 2) -
          2. * Sxy * std::cos(theta) * std::sin(theta);
    K22 = (Syy + Szz) * std::pow(std::sin(theta), 2) + (Sxx + Szz) * std::pow(std::cos(theta), 2) +
          2. * Sxy * std::cos(theta) * std::sin(theta);
    // K12 = -Sxy * (std::pow(std::cos(theta), 2) - std::pow(sin(theta), 2)) + (Sxx - Syy) * std::cos(theta) *
    // sin(theta);
    K10 = Sxz * std::cos(theta) + Syz * std::sin(theta);
    K01 = -Sxz * std::sin(theta) + Syz * std::cos(theta);
    K00 = Sxx + Syy;

    c2 = -K00 - K11 - K22;
    c1 = K00 * K11 + K00 * K22 + K11 * K22 - K01 * K01 - K10 * K10;
    c0 = K01 * K01 * K11 + K10 * K10 * K22 - K00 * K11 * K22;

    p = c1 - std::pow(c2, 2) / 3.;
    q = 2. * std::pow(c2, 3) / 27. - c1 * c2 / 3. + c0;
    r = std::pow(q / 2., 2) + std::pow(p, 3) / 27.;

    if(r >= 0)
        dm2 = -c2 / 3. + std::pow(-q / 2. + std::sqrt(r), 1. / 3.) + std::pow(-q / 2. - std::sqrt(r), 1. / 3.);
    else
    {
        rho = std::sqrt(-std::pow(p, 3) / 27.);
        phi = std::acos(-q / (2. * rho));
        dm2 = std::min(-c2 / 3. + 2. * std::pow(rho, 1. / 3.) * std::cos(phi / 3.),
                       std::min(-c2 / 3. + 2. * std::pow(rho, 1. / 3.) * std::cos((phi + 2. * TMath::Pi()) / 3.),
                                -c2 / 3. + 2. * std::pow(rho, 1. / 3.) * std::cos((phi + 4. * TMath::Pi()) / 3.)));
    }

    a = -K10 * std::cos(theta) / (K11 - dm2) + K01 * std::sin(theta) / (K22 - dm2);
    b = -K10 * std::sin(theta) / (K11 - dm2) - K01 * std::cos(theta) / (K22 - dm2);

    Xh = ((1. + b * b) * Xm - a * b * Ym + a * Zm) / (1. + a * a + b * b);
    Yh = ((1. + a * a) * Ym - a * b * Xm + b * Zm) / (1. + a * a + b * b);
    Zh = ((a * a + b * b) * Zm + a * Xm + b * Ym) / (1. + a * a + b * b);

    XYZPoint Pm = {static_cast<float>(Xm), static_cast<float>(Ym), static_cast<float>(Zm)}; // gravity point
    XYZPoint Ph = {static_cast<float>(Xh), static_cast<float>(Yh), static_cast<float>(Zh)}; // second point
    XYZPoint Sigmas = {
        static_cast<float>(std::sqrt(std::abs(Sxx))), static_cast<float>(std::sqrt(std::abs(Syy))),
        static_cast<float>(std::sqrt(std::abs(Szz)))}; // sigmas are computed from matrix elements directly
    // Still one more check of NaN; return default parameters in this case
    if(std::isnan(dm2))
        return;
    SetPoint(Pm);
    SetDirection(Pm, Ph);
    SetSigmas(Sigmas);
    SetChi2(fabs(dm2)); // do not divide by charge!
}

std::shared_ptr<TPolyLine> ActPhysics::Line::GetPolyLine(TString proj, int maxX, int maxY, int maxZ) const
{
    // Check proj key is right
    if(!(proj.Contains("xy") || proj.Contains("xz") || proj.Contains("yz")))
        throw std::runtime_error("Wrong passed projection key, allowed values are: xy, xz and yz");

    // Parametrize line in X
    auto slope3DXY {fDirection.Y() / fDirection.X()};
    auto slope3DXZ {fDirection.Z() / fDirection.X()};
    // Slope in X can be 0 if cluster is from pad saturation -> skip drawing of that line
    if(!isfinite(slope3DXY) || !std::isfinite(slope3DXZ))
    {
        if(gEnv->GetValue("ActRoot.Verbose", false))
            std::cout << BOLDMAGENTA << "PolyLine with slope X = 0 -> skip drawing" << RESET << '\n';
        return {TreatSaturationLine(proj, maxZ)}; // return empty polyline
    }
    // Compute slopes for the different projections
    auto offsetXY {fPoint.Y() - slope3DXY * fPoint.X()};
    auto slopeXY {slope3DXY};

    auto offsetXZ {fPoint.Z() - slope3DXZ * fPoint.X()};
    auto slopeXZ {slope3DXZ};

    // Build vectors
    int npoints {300};
    std::vector<double> vx, vy, vz;
    // Set step
    double x0 {0};
    double step {1. * static_cast<double>(maxX) / npoints};
    for(int p = 0; p < npoints; p++)
    {
        auto yval {offsetXY + slopeXY * x0};
        auto zval {offsetXZ + slopeXZ * x0};
        // filling according to projection
        if(proj.Contains("xy"))
        {
            if(IsInRange(yval, 0, maxY))
            {
                vx.push_back(x0);
                vy.push_back(yval);
            }
        }
        else if(proj.Contains("xz"))
        {
            if(IsInRange(zval, 0, maxZ))
            {
                vx.push_back(x0);
                vz.push_back(zval);
            }
        }
        else // yz
        {

            if(IsInRange(yval, 0, maxY) && IsInRange(zval, 0, maxZ))
            {
                vy.push_back(yval);
                vz.push_back(zval);
            }
        }
        // add step
        x0 += step;
    }
    // Return
    if(proj.Contains("xy"))
        return std::make_shared<TPolyLine>(vx.size(), &(vx[0]), &(vy[0]));
    else if(proj.Contains("xz"))
        return std::make_shared<TPolyLine>(vx.size(), &(vx[0]), &(vz[0]));
    else // yz
        return std::make_shared<TPolyLine>(vy.size(), &(vy[0]), &(vz[0]));
}

std::shared_ptr<TPolyLine> ActPhysics::Line::TreatSaturationLine(TString proj, int maxZ) const
{
    // So for this line the slope is 0 in both X and Y
    int npoints {300};
    std::vector<double> vx, vy, vz;
    // Set step
    double z0 {0};
    double step {1. * static_cast<double>(maxZ) / npoints};
    for(int i = 0; i < npoints; i++)
    {
        if(IsInRange(z0, 0, maxZ))
        {
            vx.push_back(fPoint.X());
            vy.push_back(fPoint.Y());
            vz.push_back(z0);
        }
        z0 += step;
    }
    // Return
    std::shared_ptr<TPolyLine> ret;
    if(proj.Contains("xy"))
        ret = std::make_shared<TPolyLine>(vx.size(), &(vx[0]), &(vy[0]));
    else if(proj.Contains("xz"))
        ret = std::make_shared<TPolyLine>(vx.size(), &(vx[0]), &(vz[0]));
    else // yz
        ret = std::make_shared<TPolyLine>(vy.size(), &(vy[0]), &(vz[0]));
    // To identify these events!
    ret->SetLineStyle(2);
    return ret;
}

void ActPhysics::Line::Print() const
{
    std::cout << BOLDGREEN << "---- Line parameters ----" << '\n';
    std::cout << "-> Point      : " << fPoint << '\n';
    std::cout << "-> Sigmas     : " << fSigmas << '\n';
    std::cout << "-> UDirection : " << fDirection.Unit() << '\n';
    std::cout << "-> Chi2       : " << fChi2 << '\n';
    std::cout << "-----------------------------" << RESET << '\n';
}
