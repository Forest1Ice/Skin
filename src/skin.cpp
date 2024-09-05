#include "skin.h"

#include <Standard_Failure.hxx>

Skin::Skin(const std::vector<Handle(Geom_BSplineCurve)>& curves, int degree)
	: m_degreeV{degree}, m_numCurves{ static_cast<int>(curves.size())},
	m_knotsU{ 1, curves.empty() ? 1 : curves[0]->Knots().Length() }, // Initialize m_knotsU with appropriate size
	m_multsU{ 1, curves.empty() ? 1 : curves[0]->Multiplicities().Length() } // Initialize m_multsU with appropriate size
{
	if (curves.empty()) {
		throw Standard_Failure("Curves vector is empty!");
	}

	if (m_degreeV >= m_numCurves)
	{
		throw Standard_Failure("Invalid argument m_degreeV!");
	}

	// suppose the degrees and knot vectors of different splines are the same
	m_degreeU = curves[0]->Degree();
	m_knotsU = curves[0]->Knots();
	m_multsU = curves[0]->Multiplicities();
	m_numControlPointsU = curves[0]->NbPoles();

	std::vector<TColgp_Array1OfPnt> controlPointsU(m_numCurves, TColgp_Array1OfPnt(1, m_numControlPointsU));
	for (int i = 0; i < m_numCurves; ++i)
	{
		controlPointsU[i] = curves[i]->Poles();
	}

	m_ControlPointsV.resize(m_numControlPointsU, TColgp_Array1OfPnt(1, m_numCurves));
	for (int i = 0; i < m_numControlPointsU; ++i)
	{
		for (int j = 0; j < m_numCurves; ++j)
		{
			m_ControlPointsV[i][j + 1] = controlPointsU[j].Value(i + 1);
		}
	}
}

void Skin::skin()
{
	// calculate parameters and knot vector at v direction
	calculate();

	// construct generated B-spline skin surface
	constructSurface();
}

void Skin::calculate()
{
	// calculate parameters at v direction with chord length parameterization method
	nurbs::getChordParameterization(m_ControlPointsV, m_paramsV);

	// calculate knot vector at v direction
	nurbs::averageKnotVector(m_degreeV, m_paramsV, m_knotsV);
}

void Skin::constructSurface()
{
	// calculate control points of B-spline surface
	TColgp_Array2OfPnt poles(1, m_numControlPointsU, 1, m_numCurves);
	
	for (int i = 1; i <= m_numControlPointsU; ++i)
	{
		TColgp_Array1OfPnt controlPoints(1, m_numCurves);
		nurbs::curveInterpolation(m_paramsV, m_knotsV, m_ControlPointsV[i - 1], controlPoints);

		for (int j = 1; j <= m_numCurves; ++j)
		{
			poles.SetValue(i, j, controlPoints.Value(j));
		}
	}

	// convert m_knotsV to OCC form
	TColStd_Array1OfReal geom_knotsV;
	TColStd_Array1OfInteger geom_multsV;
	util::convertKnots(m_knotsV, geom_knotsV, geom_multsV);

	// construct skinning surface
	m_bsplineSurface = new Geom_BSplineSurface(poles, m_knotsU, geom_knotsV, m_multsU, geom_multsV, m_degreeU, m_degreeV);
}

const Handle(Geom_BSplineSurface) Skin::getSurface() const
{
	return m_bsplineSurface;
}


