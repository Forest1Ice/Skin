#pragma once

#include <vector>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TopoDS_Edge.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <STEPControl_StepModelType.hxx>
#include <Geom_BSplineCurve.hxx>

namespace nurbs
{
	// The total chord length
	double getTotalChordLength(const TColgp_Array1OfPnt& points);

	// The chord length parameterization
	void getChordParameterization(const TColgp_Array1OfPnt& points, std::vector<double>& params);

	// The chord length parameterization for several groups of points
	void getChordParameterization(const std::vector<TColgp_Array1OfPnt>& points, std::vector<double>& params);

	// Technique of averaging
	void averageKnotVector(int degree, const std::vector<double>& params, std::vector<double>& knots);

	// Find the span of the given parameter in the knot vector
	int findSpan(int degree, const std::vector<double>& knots, double u);

	// Compute the nonvanishing basis functions.
	void calcBasisFunctions(int span, int degree, const std::vector<double>& knots, double u,
		std::vector<double>& basisFuns);

	// B-spline curve interpolation
	void curveInterpolation(const std::vector<double>& params, const std::vector<double>& knots, const TColgp_Array1OfPnt& points, TColgp_Array1OfPnt& controlPoints);
};


namespace util
{
	//// construct complete knot vector from knots and multiplicities of OCC
	//void constructKnots(const TColStd_Array1OfReal& geom_knots, const TColStd_Array1OfInteger & geom_mults, std::vector<double>& knots);

	// convert complete knot vector to OCC form
	void convertKnots(const std::vector<double>& knots, TColStd_Array1OfReal& geom_knots, TColStd_Array1OfInteger& geom_mults);

	// convert edge to B-spline curve
	bool convertToBSplineCurve(const TopoDS_Shape& shape, TopoDS_Edge& edge, Handle(Geom_BSplineCurve)& bsplineCurve);
};


namespace io
{
	// read step file and save models
	void readModel(const Standard_CString filename, Handle(TopTools_HSequenceOfShape)& hSequenceOfShape);

	/*
	 * translate models and save step file
	 **/
	void saveStep(const Standard_CString filename,
		const Handle(TopTools_HSequenceOfShape)& hSequenceOfShape,
		const STEPControl_StepModelType mode);
};