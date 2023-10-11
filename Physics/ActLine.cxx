#include "ActLine.h"

#include "ActColors.h"
#include "TEnv.h"
#include "TMath.h"
#include "TPolyLine.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

ActPhysics::Line::Line(XYZPoint point, XYZVector direction, float chi)
    : fPoint(point), fDirection(direction), fChi2(chi)
{}

ActPhysics::Line::Line(const XYZPoint& p1, const XYZPoint& p2)
{
    SetPoint(p1);
    SetDirection(p1, p2);
}

double ActPhysics::Line::DistanceLineToPoint(const XYZPoint &point) const
{
    auto vec = point - fPoint;
	auto nD = fDirection.Cross(vec);
	double dist2 = nD.Mag2() / fDirection.Mag2();

	return std::sqrt(dist2);
}

void ActPhysics::Line::FitVoxels(const std::vector<ActRoot::Voxel> &voxels, double qThreshold)
{
    std::vector<XYZPoint> cloud; std::vector<double> charge;
    for(const auto& voxel : voxels)
    {
        if(qThreshold != -1)//charge matters
            if(!(voxel.GetCharge() >= qThreshold))//not above threshold
                continue;
        cloud.push_back(voxel.GetPosition());
        charge.push_back(voxel.GetCharge());
    }
    if(qThreshold == -1)//charge does not matter: do not specify charge vector
        FitCloudWithThreshold(cloud, {});
    else
        FitCloudWithThreshold(cloud, charge);
}

void ActPhysics::Line::FitCloud(const std::vector<XYZPoint> &cloud)
{
    FitCloudWithThreshold(cloud, {});
}

void ActPhysics::Line::FitCloudWithThreshold(const std::vector<XYZPoint>& points, const std::vector<double>& charge)
{
    //------3D Line Regression
    //----- adapted from: http://fr.scribd.com/doc/31477970/Regressions-et-trajectoires-3D
    int R, C;
    double Q;
    float Xm, Ym, Zm;
    float Xh, Yh, Zh;
    double a, b;
    double Sxx, Sxy, Syy, Sxz, Szz, Syz;
    double theta;
    double K11, K22, K12, K10, K01, K00;
    double c0, c1, c2;
    double p, q, r, dm2;
    double rho, phi;
  
    Q = Xm = Ym = Zm = 0.;
    double total_charge = 0;
    Sxx = Syy = Szz = Sxy = Sxz = Syz = 0.;
    bool doChargeWeight = (points.size() == charge.size());
  
    for (int i = 0; i < points.size(); ++i) {
        const auto hitQ = doChargeWeight ? charge[i] : 1.;
        const auto &pos = points[i];
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
  
    theta = 0.5 * atan((2. * Sxy) / (Sxx - Syy));
  
    K11 = (Syy + Szz) * pow(cos(theta), 2) + (Sxx + Szz) * pow(sin(theta), 2) - 2. * Sxy * cos(theta) * sin(theta);
    K22 = (Syy + Szz) * pow(sin(theta), 2) + (Sxx + Szz) * pow(cos(theta), 2) + 2. * Sxy * cos(theta) * sin(theta);
    // K12 = -Sxy * (pow(cos(theta), 2) - pow(sin(theta), 2)) + (Sxx - Syy) * cos(theta) * sin(theta);
    K10 = Sxz * cos(theta) + Syz * sin(theta);
    K01 = -Sxz * sin(theta) + Syz * cos(theta);
    K00 = Sxx + Syy;
  
    c2 = -K00 - K11 - K22;
    c1 = K00 * K11 + K00 * K22 + K11 * K22 - K01 * K01 - K10 * K10;
    c0 = K01 * K01 * K11 + K10 * K10 * K22 - K00 * K11 * K22;
  
    p = c1 - pow(c2, 2) / 3.;
    q = 2. * pow(c2, 3) / 27. - c1 * c2 / 3. + c0;
    r = pow(q / 2., 2) + pow(p, 3) / 27.;
  
    if (r > 0)
        dm2 = -c2 / 3. + pow(-q / 2. + sqrt(r), 1. / 3.) + pow(-q / 2. - sqrt(r), 1. / 3.);
    else {
        rho = sqrt(-pow(p, 3) / 27.);
        phi = acos(-q / (2. * rho));
        dm2 = std::min(-c2 / 3. + 2. * pow(rho, 1. / 3.) * cos(phi / 3.),
                       std::min(-c2 / 3. + 2. * pow(rho, 1. / 3.) * cos((phi + 2. * TMath::Pi()) / 3.),
                                -c2 / 3. + 2. * pow(rho, 1. / 3.) * cos((phi + 4. * TMath::Pi()) / 3.)));
    }
  
    a = -K10 * cos(theta) / (K11 - dm2) + K01 * sin(theta) / (K22 - dm2);
    b = -K10 * sin(theta) / (K11 - dm2) - K01 * cos(theta) / (K22 - dm2);
  
    Xh = ((1. + b * b) * Xm - a * b * Ym + a * Zm) / (1. + a * a + b * b);
    Yh = ((1. + a * a) * Ym - a * b * Xm + b * Zm) / (1. + a * a + b * b);
    Zh = ((a * a + b * b) * Zm + a * Xm + b * Ym) / (1. + a * a + b * b);
  
    XYZPoint Pm = {Xm, Ym, Zm};//gravity point
    XYZPoint Ph = {Xh, Yh, Zh};//second point

	//temporary bug fix: detect if we have NaN values here dont redefine Line!
	if(std::isnan(dm2))
	{
        // std::cout<<"c2 "<<c2<<'\n';
        // std::cout<<"q "<<q<<'\n';
        // std::cout<<"r "<<r<<'\n';
        // std::cout<<"rho "<<rho<<'\n';
        // std::cout<<"phi "<<phi<<'\n';
		// std::cout<<BOLDMAGENTA<<"Warning: dm2 is NaN is ActLine::FitLineToCloud due to pad saturation -> Not fitting"<<RESET<<'\n';
		//std::cout<<"Remaining line direction X: "<<GetDirection().X()<<" Y: "<<GetDirection().Y()<< " Z: "<<GetDirection().Z()<<'\n';
		return;
	}
	SetPoint(Pm);
	SetDirection(Pm, Ph);
	SetChi2(fabs(dm2 / Q));
	//WARNING: Something in this func returns nan sometimes! It is in variable dm2!
}

std::shared_ptr<TPolyLine> ActPhysics::Line::GetPolyLine(TString proj,
                                                         int maxX, int maxY, int maxZ) const
{
    //Check proj key is right
    if(!(proj.Contains("xy") || proj.Contains("xz") || proj.Contains("yz")))
        throw std::runtime_error("Wrong passed projection key, allowed values are: xy, xz and yz");
    
    //Parametrize line in X
    auto slope3DXY {fDirection.Y() / fDirection.X()};
    auto slope3DXZ {fDirection.Z() / fDirection.X()};
    //Slope in X can be 0 if cluster is from pad saturation -> skip drawing of that line
    if(!isfinite(slope3DXY) || !std::isfinite(slope3DXZ))
    {
        if(gEnv->GetValue("ActRoot.Verbose", false))
            std::cout<<BOLDMAGENTA<<"PolyLine with slope X = 0 -> skip drawing"<<RESET<<'\n';
        return {};
    }
    //Compute slopes for the different projections
    auto offsetXY {fPoint.Y() - slope3DXY * fPoint.X()};
    auto slopeXY  {slope3DXY};

    auto offsetXZ {fPoint.Z() - slope3DXZ * fPoint.X()};
    auto slopeXZ  {slope3DXZ};

    //Build vectors
    int npoints {300};
    std::vector<double> vx, vy, vz;
    //Set step
    double x0 {0};
    double step {1. * static_cast<double>(maxX) / npoints};
    for(int p = 0; p < npoints; p++)
    {
        auto yval {offsetXY + slopeXY * x0};
        auto zval {offsetXZ + slopeXZ * x0};
        //filling according to projection
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
        else//yz
        {

            if(IsInRange(yval, 0, maxY) && IsInRange(zval, 0, maxZ))
            {
                vy.push_back(yval);
                vz.push_back(zval);
            }
        }
        //add step
        x0 += step;
    }
    //Return
    if(proj.Contains("xy"))
        return std::make_shared<TPolyLine>(vx.size(), &(vx[0]), &(vy[0]));
    else if(proj.Contains("xz"))
        return std::make_shared<TPolyLine>(vx.size(), &(vx[0]), &(vz[0]));
    else//yz
        return std::make_shared<TPolyLine>(vy.size(), &(vy[0]), &(vz[0]));
}
