#pragma once

#include "skin.h"

#include <AIS_SequenceOfInteractive.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <V3d_View.hxx>
#include <WNT_Window.hxx>

#include <QWidget>
#include <QStatusBar>

class Viewer : public QWidget, protected AIS_ViewController
{
	Q_OBJECT
public:
	Viewer(QWidget* parent = nullptr);
	~Viewer();

	void setStatusBar(QStatusBar* statusBar);

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

	QPaintEngine* paintEngine() const;


public slots:
	// File
	void open();	// open file
	void save();	// save file

	// Edit
	void clear();	// clear all models

	// Surface
	void skin();

	void fitView();
	void shadingView();	// shading pattern
	void wireframeView(); // wireframe pattern

private:
	void init();	// initialize
	void updateView();
	TopoDS_Shape getShape();	// detect the currently chosen shape
	void operator<<(const TopoDS_Shape& shape);
	void showMessage(const QString& message); // show message at status bar

private:
	std::vector<TopoDS_Shape> m_shapes;
	std::vector<Handle(Geom_BSplineCurve)> m_bsplineCurves;

	Handle(V3d_Viewer) m_viewer;
	Handle(V3d_View) m_view;
	Handle(AIS_InteractiveContext) m_context;
	Handle(WNT_Window) m_wntWindow;

	QStatusBar* m_statusBar;
};
