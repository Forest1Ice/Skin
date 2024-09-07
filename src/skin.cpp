#include "skin.h"

#include <map>
#include <Standard_Failure.hxx>

Skin::Skin(const std::vector<Handle(Geom_BSplineCurve)>& curves, int degree)
	: m_degreeV{degree}, m_numCurves{ static_cast<int>(curves.size())},
	m_knotsU{ 1, curves.empty() ? 1 : curves[0]->Knots().Length() }, // Initialize m_knotsU with appropriate size
	m_multsU{ 1, curves.empty() ? 1 : curves[0]->Multiplicities().Length() } // Initialize m_multsU with appropriate size
{
	if (curves.empty()) {
		try 
		{
			throw Standard_Failure("Curves vector is empty!");
		}
		catch (Standard_Failure& failure) 
		{
			std::cerr << "Caught error: " << failure.GetMessageString() << std::endl;
		}
	}

	if (m_degreeV >= m_numCurves)
	{
		try
		{
			throw Standard_Failure("Invalid argument m_degreeV!");
		}
		catch (Standard_Failure& failure)
		{
			std::cerr << "Caught error: " << failure.GetMessageString() << std::endl;
		}
	}

	// Increase Degree
	std::vector<Handle(Geom_BSplineCurve)> newCurves = curves;
	increaseDegree(newCurves);

	// Knot refinements
	refineKnots(newCurves);

	// Now the degrees and knot sequences of different curves are the same
	m_degreeU = newCurves[0]->Degree();
	m_knotsU = newCurves[0]->Knots();
	m_multsU = newCurves[0]->Multiplicities();
	m_numControlPointsU = newCurves[0]->NbPoles();

	std::vector<TColgp_Array1OfPnt> controlPointsU(m_numCurves, TColgp_Array1OfPnt(1, m_numControlPointsU));
	for (int i = 0; i < m_numCurves; ++i)
	{
		controlPointsU[i] = newCurves[i]->Poles();
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

void Skin::increaseDegree(std::vector<Handle(Geom_BSplineCurve)>& curves)
{
	Standard_Integer maxDegree = 0;
	for (auto curve : curves)
	{
		maxDegree = maxDegree < curve->Degree() ? curve->Degree() : maxDegree;
	}

	for (auto curve : curves)
	{
		curve->IncreaseDegree(maxDegree);
	}
}

void Skin::refineKnots(std::vector<Handle(Geom_BSplineCurve)>& curves)
{
	// Suppose each curve is defined in the same knot interval, such as [0,1].
	
	// Merge knots of all curves to obtain a common knot sequence and mult sequence.
	// Store knots and corresponding maximum mutiplicities with map.
	std::map<Standard_Real, Standard_Integer> knotMap;

	for (auto& curve : curves)
	{
		// Obtain the knots and multiplicities.
		TColStd_Array1OfReal curveKnots = curve->Knots();
		TColStd_Array1OfInteger curveMults = curve->Multiplicities();

		for (Standard_Integer i = curveKnots.Lower(); i <= curveKnots.Upper(); ++i)
		{
			Standard_Real knot = curveKnots.Value(i);
			Standard_Integer mult = curveMults.Value(i);

			// If knot is existent, update its maximum mult.
			if (knotMap.find(knot) != knotMap.end())
			{
				knotMap[knot] = std::max(knotMap[knot], mult);
			}
			else
			{
				// The knot is not in map, insert it and its mult.
				knotMap[knot] = mult;
			}
		}
	}

	// Now knotMap stores all emerging knots and their maximum mults.
	// Note that map sort automatically according to key values, i.e. knot values here.
	Standard_Integer nbKnots = static_cast<Standard_Integer>(knotMap.size());
	TColStd_Array1OfReal knots = TColStd_Array1OfReal(1, nbKnots);
	TColStd_Array1OfInteger mults = TColStd_Array1OfInteger(1, nbKnots);

	// Copy the values in knotMap into knots and mults.
	Standard_Integer index = 1;
	for (const auto& [knot, mult] : knotMap)
	{
		knots.SetValue(index, knot);
		mults.SetValue(index, mult);
		++index;
	}

	// Refinement for all curves.
	for (auto& curve : curves)
	{
		curve->InsertKnots(knots, mults);
	}
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


