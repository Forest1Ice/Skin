#pragma once

#include "viewer.h"
#include <vector>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>

class Window: public QMainWindow
{
	Q_OBJECT

public:
	Window(QWidget* parent = nullptr, int left = 200, int top = 100, int width = 1000, int height = 700);
	~Window();

private:
	void init();
	void process(std::vector<QString>& actionNames, std::vector<QAction*>& actions, QMenu* menu);

private:
	Viewer* m_viewer;

	QStatusBar* m_statusBar;
	QToolBar* m_toolBar;
	QMenu* m_fileMenu;
	QMenu* m_editMenu;
	QMenu* m_surfaceMenu;

	std::vector<QAction*> m_fileActions;
	std::vector<QAction*> m_editActions;
	std::vector<QAction*> m_surfaceActions;
};