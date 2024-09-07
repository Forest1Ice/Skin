#pragma once

#include "utils.h"

#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>

class Skin
{
public:
	Skin(const std::vector<Handle(Geom_BSplineCurve)>& curves, int degree = 3);	// "degree" is the degree of B-spline at direction v
	
	// skin operation
	void skin();

	// get generated surface
	const Handle(Geom_BSplineSurface) getSurface() const;

private:
	// increase the degrees of all curves to the same
	void increaseDegree(std::vector<Handle(Geom_BSplineCurve)>& curves);

	// refine the knots of all curves to the same
	void refineKnots(std::vector<Handle(Geom_BSplineCurve)>& curves);

	// calculate parameters and knot vector at v direction
	void calculate();

	// construct generated B-spline skin surface																				// create generated B-spline skin surface
	void constructSurface();

private:
	int m_degreeU, m_degreeV;	// degrees of B-spline in derection of u and v
	int m_numCurves;	// number of section curves
	int m_numControlPointsU;	// number of control points on each section curve

	TColStd_Array1OfReal m_knotsU;	// knot vectors at u direction
	TColStd_Array1OfInteger m_multsU;	// multiplicities at u direction

	std::vector<double> m_knotsV;	// knot vectors at v direction
	std::vector<double> m_paramsV;	// parameters at v direction

	std::vector<TColgp_Array1OfPnt> m_ControlPointsV;	// control points of section curves arranged in v direction

	Handle(Geom_BSplineSurface) m_bsplineSurface;	// skinned surface
};
