#include "ActLine.h"

#include "ActColors.h"
#include "ActUtils.h"
#include "ActVoxel.h"

#include "TEnv.h"
#include "TMath.h"
#include "TMathBase.h"
#include "TPolyLine.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
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

void ActPhysics::Line::Scale(float xy, float z)
{
    // Point
    fPoint.SetX(fPoint.X() * xy);
    fPoint.SetY(fPoint.Y() * xy);
    fPoint.SetZ(fPoint.Z() * z);
    // Direction
    fDirection.SetX(fDirection.X() * xy);
    fDirection.SetY(fDirection.Y() * xy);
    fDirection.SetZ(fDirection.Z() * z);
}

void ActPhysics::Line::AlignUsingPoint(const XYZPoint& rp, bool isRecoil)
{
    XYZVector dir {};
    if(isRecoil)
        dir = fPoint - rp;
    else
        dir = rp - fPoint;
    // Set same signs in direction as the previous vector
    // Usually used to set the direction of propagation after finding a RP
    // TMath::Sign(a, b) returns a (fDirection) with the same sign as b (dir using rp)
    fDirection.SetX(TMath::Sign(fDirection.X(), dir.X()));
    fDirection.SetY(TMath::Sign(fDirection.Y(), dir.Y()));
    fDirection.SetZ(TMath::Sign(fDirection.Z(), dir.Z()));
}

double ActPhysics::Line::DistanceLineToPoint(const XYZPoint& point) const
{
    auto vec = point - fPoint;
    auto nD = fDirection.Cross(vec);
    double dist2 = nD.Mag2() / fDirection.Mag2();

    return std::sqrt(dist2);
}

ActPhysics::Line::XYZPoint ActPhysics::Line::ProjectionPointOnLine(const XYZPoint& point) const
{
    auto vToPoint {point - fPoint};
    auto vInLine {fDirection * (fDirection.Dot(vToPoint) / fDirection.Mag2())};
    return fPoint + vInLine;
}

ActPhysics::Line::XYZPoint ActPhysics::Line::MoveToX(float x) const
{
    auto t {(x - fPoint.X()) / fDirection.X()};
    return {x, fPoint.Y() + fDirection.Y() * t, fPoint.Z() + fDirection.Z() * t};
}

void ActPhysics::Line::FitVoxels(const std::vector<ActRoot::Voxel>& voxels, bool qWeighted, bool correctOffset)
{
    DoFit(voxels, qWeighted, correctOffset);
}

void ActPhysics::Line::DoFit(const std::vector<ActRoot::Voxel>& voxels, bool qWeighted, bool correctOffset)
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

    for(const auto& voxel : voxels)
    {
        const auto hitQ = (qWeighted ? voxel.GetCharge() : 1.);
        auto pos = voxel.GetPosition();
        if(correctOffset)
            pos += XYZVector {0.5, 0.5, 0.5};
        Q += hitQ;
        Xm += pos.X() * hitQ;
        Ym += pos.Y() * hitQ;
        Zm += pos.Z() * hitQ;
        Sxx += pos.X() * pos.X() * hitQ;
        Syy += pos.Y() * pos.Y() * hitQ;
        Szz += pos.Z() * pos.Z() * hitQ;
        Sxy += pos.X() * pos.Y() * hitQ;
        Sxz += pos.X() * pos.Z() * hitQ;
        Syz += pos.Y() * pos.Z() * hitQ;
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

    // Write basic values altough fit might not be doable
    fPoint = {static_cast<float>(Xm), static_cast<float>(Ym), static_cast<float>(Zm)};
    fSigmas = {static_cast<float>(std::sqrt(std::abs(Sxx))), static_cast<float>(std::sqrt(std::abs(Syy))),
               static_cast<float>(std::sqrt(std::abs(Szz)))};
    // Undoable 3D fit -> fallbacking to 2D
    if(ActRoot::IsEqZero(Sxx) || ActRoot::IsEqZero(Syy) || ActRoot::IsEqZero(Szz))
    {
        if(ActRoot::IsEqZero(Sxx) && !ActRoot::IsEqZero(Syy) && !ActRoot::IsEqZero(Szz))
            Fit2Dfrom3D(Ym, Zm, Syy, Szz, Syz, Q, "x");
        else if(ActRoot::IsEqZero(Syy) && !ActRoot::IsEqZero(Sxx) && !ActRoot::IsEqZero(Szz))
            Fit2Dfrom3D(Xm, Zm, Sxx, Szz, Sxz, Q, "y");
        else if(ActRoot::IsEqZero(Szz) && !ActRoot::IsEqZero(Sxx) && !ActRoot::IsEqZero(Syy))
            Fit2Dfrom3D(Xm, Ym, Sxx, Syy, Sxy, Q, "z");
        else // handle case with more than one Sii == 0 -> return bad fit
        {
            fDirection = {std::nanf("bad fit"), std::nanf("bad fit"), std::nanf("bad fit")};
            return;
        }
        // and recompute Chi2 after fitting
        Chi2Dfrom3D(voxels, correctOffset);
        return;
    }
    // Doable 3D fit
    theta = 0.5 * std::atan((2. * Sxy) / (Sxx - Syy));
    K11 = (Syy + Szz) * std::pow(std::cos(theta), 2) + (Sxx + Szz) * std::pow(std::sin(theta), 2) -
          2. * Sxy * std::cos(theta) * std::sin(theta);
    K22 = (Syy + Szz) * std::pow(std::sin(theta), 2) + (Sxx + Szz) * std::pow(std::cos(theta), 2) +
          2. * Sxy * std::cos(theta) * std::sin(theta);
    // K12 = -Sxy * (std::pow(std::cos(theta), 2) - std::pow(sin(theta), 2)) + (Sxx - Syy) * std::cos(theta) *
    // sin(theta)cos;
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

    // std::cout << "Sxx : " << Sxx << " Syy : " << Syy << " Szz : " << Szz << '\n';
    // std::cout << "Sxy : " << Sxy << " Sxz : " << Sxz << " Syz : " << Syz << '\n';
    // std::cout << " K11 : " << K11 << " K22 : " << K22 << '\n';
    // std::cout << "K10 : " << K10 << " K01 : " << K01 << " K00 : " << K00 << '\n';
    // std::cout << "c2 : " << c2 << " c1 : " << c1 << " c0 : " << c0 << '\n';
    // std::cout << "p : " << p << " q : " << q << " r : " << r << '\n';
    // std::cout << "rho : " << rho << " phi : " << phi << " dm2 : " << dm2 << '\n';
    // std::cout << "a : " << a << " b : " << b << '\n';
    // std::cout << "=======" << '\n';

    Xh = ((1. + b * b) * Xm - a * b * Ym + a * Zm) / (1. + a * a + b * b);
    Yh = ((1. + a * a) * Ym - a * b * Xm + b * Zm) / (1. + a * a + b * b);
    Zh = ((a * a + b * b) * Zm + a * Xm + b * Ym) / (1. + a * a + b * b);

    XYZPoint Ph = {static_cast<float>(Xh), static_cast<float>(Yh), static_cast<float>(Zh)}; // second point
    SetDirection(fPoint, Ph);
    fChi2 = std::fabs(dm2); // do not divide by charge!
}

void ActPhysics::Line::Fit2Dfrom3D(double Mi, double Mj, double Sii, double Sjj, double Sij, double w,
                                   const std::string& degenerated)
{
    // Based on: https://stackoverflow.com/questions/11449617/how-to-fit-the-2d-scatter-data-with-a-line-with-c
    // and in:
    // https://math.stackexchange.com/questions/2037610/given-the-equation-of-a-straight-line-how-would-i-find-a-direction-vector
    // Revert all variable to their state before /= Q and -= MiMj
    Sij += Mi * Mj;
    Sjj += Mj * Mj;
    Sii += Mi * Mi;
    //////////////
    Sij *= w;
    Sjj *= w;
    Sii *= w;
    Mj *= w;
    Mi *= w;
    //////////////
    double denom {w * Sii - std::pow(Mi, 2)};
    if(std::isnan(1. / denom)) // NaN!! vertical line
    {
        fDirection = {std::nanf("bad fit"), std::nanf("bad fit"), std::nanf("bad fit")};
        return;
    }
    double m {-(w * Sij - Mi * Mj) / denom};
    //// Write
    if(degenerated == "x")
        fDirection = {0, 1, (float)-m};
    else if(degenerated == "y")
        fDirection = {1, 0, (float)-m};
    else
        fDirection = {1, (float)-m, 0};
}

void ActPhysics::Line::Chi2Dfrom3D(const std::vector<ActRoot::Voxel>& voxels, bool correctOffset)
{
    // Check fit is good
    if(std::isnan(fDirection.Z()))
        return;
    // We use the built-in function
    double dm2 {};
    for(const auto& voxel : voxels)
    {
        auto pos {voxel.GetPosition()};
        if(correctOffset)
            pos += XYZVector {0.5, 0.5, 0.5};
        dm2 += std::pow(DistanceLineToPoint(pos), 2);
    }
    dm2 /= voxels.size();
    fChi2 = std::sqrt(dm2);
}

std::shared_ptr<TPolyLine> ActPhysics::Line::GetPolyLine(TString proj, int maxX, int maxY, int maxZ, int rebinZ) const
{
    // Check proj key is right
    if(!(proj.Contains("xy") || proj.Contains("xz") || proj.Contains("yz")))
        throw std::runtime_error("Wrong passed projection key, allowed values are: xy, xz and yz");

    // Parametrize line in X
    auto slope3DXY {fDirection.Y() / fDirection.X()};
    auto slope3DXZ {fDirection.Z() * rebinZ / fDirection.X()};
    // Slope in X can be 0 if cluster is from pad saturation -> skip drawing of that line
    if(!std::isfinite(slope3DXY) || !std::isfinite(slope3DXZ))
    {
        if(gEnv->GetValue("ActRoot.Verbose", false))
            std::cout << BOLDMAGENTA << "PolyLine with slope X = 0 -> skip drawing" << RESET << '\n';
        return {TreatSaturationLine(proj, maxZ, rebinZ)}; // return empty polyline
    }
    // Compute slopes for the different projections
    auto offsetXY {fPoint.Y() - slope3DXY * fPoint.X()};
    auto slopeXY {slope3DXY};

    auto offsetXZ {fPoint.Z() * rebinZ - slope3DXZ * fPoint.X()};
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

std::shared_ptr<TPolyLine> ActPhysics::Line::TreatSaturationLine(TString proj, int maxZ, int rebinZ) const
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
