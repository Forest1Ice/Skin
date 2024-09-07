#include "utils.h"

#include <algorithm>
#include <string>
#include <Eigen/Dense>
#include <BSplCLib.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <StlAPI_Reader.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>

double nurbs::getTotalChordLength(const TColgp_Array1OfPnt& points)
{
	double length = 0.0;

	for (int i = points.Lower() + 1; i <= points.Upper(); ++i)
	{
		length += points[i].Distance(points[i - 1]);
	}

	return length;
}

void nurbs::getChordParameterization(const TColgp_Array1OfPnt& points, std::vector<double>& params)
{
	int size = points.Length();
	int n = size - 1;
	double d = getTotalChordLength(points);

	// initialization
	params.resize(size, 0.0);
	params[n] = 1.0;

	for (int i = 1; i <= n - 1; ++i)
	{
		params[i] = params[i - 1] + points[i + points.Lower()].Distance(points[i + points.Lower() - 1]) / d;
	}
}

void nurbs::getChordParameterization(const std::vector<TColgp_Array1OfPnt>& points, std::vector<double>& params)
{
	int number = points.size();	// number of groups
	int size = points[0].Length();	// number of points of each group

	params.resize(size, 0.0);
	std::vector<double> temp;

	for (int i = 0; i < number; ++i)
	{
		getChordParameterization(points[i], temp);
		for (int j = 0; j < size; ++j)
		{
			params[j] += temp[j];
		}
	}

	for (int j = 0; j < size; ++j)
	{
		params[j] /= number;
	}
}

void nurbs::averageKnotVector(int degree, const std::vector<double>& params, std::vector<double>& knots)
{
	int size = params.size();
	int n = size - 1;
	int m = n + degree + 1;

	knots.resize(m + 1, 0.0);
	for (int i = m - degree; i <= m; ++i)
	{
		knots[i] = 1.0;
	}

	for (int i = 1; i <= n - degree; ++i)
	{
		double sum = 0.0;
		for (int j = i; j <= i + degree - 1; ++j)
		{
			sum += params[j];
		}
		knots[i + degree] = (1.0 / degree) * sum;
	}
}

int nurbs::findSpan(int degree, const std::vector<double>& knots, double u)
{
	int m = static_cast<int>(knots.size() - 1);
	int n = m - degree - 1;

	if (u == knots[knots.size() - degree - 1])
	{
		return n;
	}

	int mid = std::upper_bound(knots.begin() + degree, knots.begin() + n + 1, u) - knots.begin() - 1;

	return mid;
}

void nurbs::calcBasisFunctions(int span, int degree, const std::vector<double>& knots, double u, std::vector<double>& basisFuns)
{
	basisFuns.resize(degree + 1);
	std::vector<double> left(degree + 1), right(degree + 1);
	basisFuns[0] = 1.0;

	for (int j = 1; j <= degree; j++)
	{
		left[j] = u - knots[span + 1 - j];
		right[j] = knots[span + j] - u;
		double saved = 0.0;
		for (int r = 0; r < j; r++)
		{
			double temp = basisFuns[r] / (right[r + 1] + left[j - r]);
			basisFuns[r] = saved + right[r + 1] * temp;
			saved = left[j - r] * temp;
		}
		basisFuns[j] = saved;
	}
}

void nurbs::curveInterpolation(const std::vector<double>& params, const std::vector<double>& knots, const TColgp_Array1OfPnt& points, TColgp_Array1OfPnt& controlPoints)
{
	int m = knots.size() - 1;
	int n = points.Length() - 1;
	int degree = m - n - 1;

	// create coefficents matrix and vectors
	Eigen::MatrixXd A = Eigen::MatrixXd::Zero((n + 1) * 3, (n + 1) * 3);
	Eigen::MatrixXd C = Eigen::MatrixXd::Zero(n + 1, n + 1);
	Eigen::VectorXd P((n + 1) * 3);
	Eigen::VectorXd Q((n + 1) * 3);

	// fill matrix and vectors
	int spanIndex;
	std::vector<double> values;
	for (int i = 0; i <= n; ++i)
	{
		spanIndex = findSpan(degree, knots, params[i]);
		calcBasisFunctions(spanIndex, degree, knots, params[i], values);

		C.row(i).segment(spanIndex - degree, degree + 1) = Eigen::Map<Eigen::VectorXd>(values.data(), values.size());
		Q(i, 0) = points[i + points.Lower()].X();
		Q(i + (n + 1), 0) = points[i + points.Lower()].Y();
		Q(i + (n + 1) * 2, 0) = points[i + points.Lower()].Z();
	}
	A.block(0, 0, n + 1, n + 1) = C;
	A.block(n + 1, n + 1, n + 1, n + 1) = C;
	A.block((n + 1) * 2, (n + 1) * 2, n + 1, n + 1) = C;

	/**
	* solve the equation which is
	* [C, 0, 0]	   [Qx]
	* [0, C, 0]P = [Qy]
	* [0, 0, C]	   [Qz]
	*/

	// LU decomposition
	Eigen::FullPivLU<Eigen::MatrixXd> lu(A);

	P = lu.solve(Q);

	for (int i = 0; i <= n; ++i)
	{
		controlPoints.SetValue(i + controlPoints.Lower(), gp_Pnt(P(i), P(i + (n + 1)), P(i + (n + 1) * 2)));
	}
}

void util::convertKnots(const std::vector<double>& knots, TColStd_Array1OfReal& geom_knots, TColStd_Array1OfInteger& geom_mults)
{
	TColStd_Array1OfReal knotsSeq(1, knots.size());
	for (int i = 0; i < knots.size(); ++i)
	{
		knotsSeq.SetValue(i + 1, knots[i]);
	}

	Standard_Integer length = BSplCLib::KnotsLength(knotsSeq);
	geom_knots.Resize(1, length, false);
	geom_mults.Resize(1, length, false);
	BSplCLib::Knots(knotsSeq, geom_knots, geom_mults);
}

bool util::convertToBSplineCurve(const TopoDS_Shape& shape, TopoDS_Edge& edge, Handle(Geom_BSplineCurve)& bsplineCurve)
{
	if (shape.IsNull())
	{
		return false;
	}

	TopExp_Explorer explorer;
	explorer.Init(shape, TopAbs_EDGE);
	edge = TopoDS::Edge(explorer.Current());
	if (edge.IsNull())
	{
		return false;
	}

	Standard_Real First, Last;
	Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, First, Last);

	bsplineCurve = Handle(Geom_BSplineCurve)::DownCast(curve);
	if (bsplineCurve.IsNull())
	{
		return false;
	}

	return true;
}

void io::readModel(const Standard_CString filename, Handle(TopTools_HSequenceOfShape)& hSequenceOfShape)
{
	hSequenceOfShape->Clear();
	std::string fileStr(filename);
	std::string extension = fileStr.substr(fileStr.find_last_of('.') + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // convert to lower case

	IFSelect_ReturnStatus status = IFSelect_RetError;

	if (extension == "step" || extension == "stp")
	{
		// read step file
		STEPControl_Reader reader;
		status = reader.ReadFile(filename);
		if (status != IFSelect_RetDone)
		{ 
			return;
		}
			
		// set trace level for outputting messages
		reader.WS()->TransferReader()->TransientProcess()->SetTraceLevel(2);
		Standard_Boolean failsonly = Standard_False;
		// check loaded data
		reader.PrintCheckLoad(failsonly, IFSelect_ItemsByEntity);

		// convert step file
		Standard_Integer nbr = reader.NbRootsForTransfer();
		reader.PrintCheckTransfer(failsonly, IFSelect_ItemsByEntity);
		for (Standard_Integer i = 1; i <= nbr; ++i)
		{
			reader.TransferRoot(i);
		}

		// number of read models
		Standard_Integer nbs = reader.NbShapes();
		if (nbs == 0)
		{
			return;
		}
		// save read models
		for (Standard_Integer i = 1; i <= nbs; ++i)
		{
			hSequenceOfShape->Append(reader.Shape(i));
		}
	}
	else if (extension == "iges" || extension == "igs")
	{
		IGESControl_Reader reader;
		status = reader.ReadFile(filename);
		if (status != IFSelect_RetDone) 
		{
			return;
		}

		reader.TransferRoots();
		Standard_Integer nbs = reader.NbShapes();
		for (Standard_Integer i = 1; i <= nbs; ++i) 
		{
			hSequenceOfShape->Append(reader.Shape(i));
		}
	}
	else if (extension == "stl")
	{
		StlAPI_Reader reader;
		TopoDS_Shape shape;
		if (!reader.Read(shape, filename)) {
			return;
		}
		hSequenceOfShape->Append(shape);
	}
	else if (extension == "brep") 
	{
		TopoDS_Shape shape;
		std::ifstream brep_file(filename);
		if (!brep_file) 
		{
			return;
		}
		BRepTools::Read(shape, brep_file, BRep_Builder());
		hSequenceOfShape->Append(shape);
	}
	else 
	{
		std::cerr << "Unsupported file format: " << extension << std::endl;
		return;
	}
}

void io::saveStep(const Standard_CString filename, const Handle(TopTools_HSequenceOfShape)& hSequenceOfShape, const STEPControl_StepModelType mode)
{
	STEPControl_Writer writer;
	IFSelect_ReturnStatus status;
	for (int i = 1; i <= hSequenceOfShape->Length(); ++i)
	{
		status = writer.Transfer(hSequenceOfShape->Value(i), mode);
		if (status != IFSelect_RetDone)
		{
			return;
		}
	}

	writer.Write(filename);
}