#include "window.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{   
    /*std::vector<Handle(Geom_BSplineCurve)> curves;

    double knots[] = { 0, 0.3, 0.7, 1 };
    int mults[] = { 4, 1, 1, 4 };
    int degree = 3;

    TColStd_Array1OfReal knotsU(1, 4);
    for (int i = 1; i <= 4; ++i) {
        knotsU.SetValue(i, knots[i - 1]);
    }

    TColStd_Array1OfInteger multsU(1, 4);
    for (int i = 1; i <= 4; ++i) {
        multsU.SetValue(i, mults[i - 1]);
    }

    std::vector<TColgp_Array1OfPnt> points(5, TColgp_Array1OfPnt(1, 6));

    for (int i = 0; i < 5; ++i)
    {
        points[i].SetValue(1, gp_Pnt(-3, i, 1));
        points[i].SetValue(2, gp_Pnt(-2, i, 2));
        points[i].SetValue(3, gp_Pnt(-1, i, 1));
        points[i].SetValue(4, gp_Pnt(1, i, 1));
        points[i].SetValue(5, gp_Pnt(2, i, 2));
        points[i].SetValue(6, gp_Pnt(3, i, 1));

        Handle(Geom_BSplineCurve) curve = new Geom_BSplineCurve(points[i], knotsU, multsU, degree);
        curves.emplace_back(curve);
    }

    Skin skin(curves, 3);
    skin.skin();

    Handle(Geom_BSplineSurface) surface = skin.getSurface();*/


    QApplication app(argc, argv);
    Window window;
    window.show();
    return app.exec();
}