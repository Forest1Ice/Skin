#include "window.h"

Window::Window(QWidget* parent, int left, int top, int width, int height)
	: QMainWindow{ parent }
{
	setGeometry(left, top, width, height);
	init();
}

Window::~Window()
{
	delete m_viewer;
}

void Window::init()
{
	// add menus
	m_fileMenu = menuBar()->addMenu(tr("File"));
	m_editMenu = menuBar()->addMenu(tr("Edit"));
	m_surfaceMenu = menuBar()->addMenu(tr("Surface"));

	// add tool bar
	m_toolBar = addToolBar(tr("Tool"));

	// add status bar
	m_statusBar = new QStatusBar(this);
	m_statusBar->setObjectName(tr("statusBar"));
	setStatusBar(m_statusBar);

	// create viewer
	m_viewer = new Viewer(this);
	setCentralWidget(m_viewer);
	m_viewer->setStatusBar(m_statusBar);	// share the same status bar

	// create actions
	std::vector<QString> file_actionNames =
	{"Open", "Save"};
	std::vector<QString> edit_actionNames =
	{"Clear"};
	std::vector<QString> surface_actionNames =
	{"Skin"};
	
	process(file_actionNames, m_fileActions, m_fileMenu);
	process(edit_actionNames, m_editActions, m_editMenu);
	process(surface_actionNames, m_surfaceActions, m_surfaceMenu);

	// connect signals and slots
	connect(m_fileActions[0], &QAction::triggered, m_viewer, &Viewer::open);
	connect(m_fileActions[1], &QAction::triggered, m_viewer, &Viewer::save);
	connect(m_editActions[0], &QAction::triggered, m_viewer, &Viewer::clear);
	connect(m_surfaceActions[0], &QAction::triggered, m_viewer, &Viewer::skin);
}


void Window::process(std::vector<QString>& actionNames, std::vector<QAction*>& actions, QMenu* menu)
{
	for (int i = 0; i < actionNames.size(); ++i)
	{
		actions.emplace_back(
			new QAction(actionNames[i], this)
		);
		menu->addAction(actions[i]);
		actions[i]->setStatusTip(actionNames[i]);
	}
}
