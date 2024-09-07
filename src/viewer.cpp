#include "viewer.h"

#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_Triangulation.hxx>
#include <Aspect_ScrollDelta.hxx>
#include <Aspect_VKeyFlags.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

#include <QFileDialog>
#include <QMessageBox>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

const std::string string1 = "Select curve: %1";

namespace
{
    //! Adjust the style of local selection.
    //! \param[in] context the AIS context.
    void AdjustSelectionStyle(const Handle(AIS_InteractiveContext)& context)
    {
        // Initialize style for sub-shape selection.
        Handle(Prs3d_Drawer) selDrawer = new Prs3d_Drawer;
        //
        selDrawer->SetLink(context->DefaultDrawer());
        selDrawer->SetFaceBoundaryDraw(true);
        selDrawer->SetDisplayMode(AIS_Shaded); // Shaded
        selDrawer->SetTransparency(0.5f);
        selDrawer->SetZLayer(Graphic3d_ZLayerId_Topmost);
        selDrawer->SetColor(Quantity_NOC_GOLD);
        selDrawer->SetBasicFillAreaAspect(new Graphic3d_AspectFillArea3d());

        // Adjust fill area aspect.
        const Handle(Graphic3d_AspectFillArea3d)& fillArea = selDrawer->BasicFillAreaAspect();
        //
        fillArea->SetInteriorColor(Quantity_NOC_GOLD);
        fillArea->SetBackInteriorColor(Quantity_NOC_GOLD);
        //
        fillArea->ChangeFrontMaterial().SetMaterialName(Graphic3d_NOM_NEON_GNC);
        fillArea->ChangeFrontMaterial().SetTransparency(0.4f);
        fillArea->ChangeBackMaterial().SetMaterialName(Graphic3d_NOM_NEON_GNC);
        fillArea->ChangeBackMaterial().SetTransparency(0.4f);

        selDrawer->UnFreeBoundaryAspect()->SetWidth(1.0);

        context->Activate(4, true); // faces
        context->Activate(2, true); // edges
        // Update AIS context.
        context->SetHighlightStyle(Prs3d_TypeOfHighlight_LocalSelected, selDrawer);
    }

    QString processString(const std::string& string, int num)
    {
        QString outputString = QString::fromStdString(string);
        QString sizeString = QString::number(num);
        outputString = outputString.arg(sizeString);
        return outputString;
    }

    Aspect_VKeyMouse QtMouseButtons2VKeyMouse(Qt::MouseButtons buttons)
    {
        Aspect_VKeyMouse vkey_mouse = Aspect_VKeyMouse_NONE;
        if ((buttons & Qt::LeftButton) != 0)
        {
            vkey_mouse |= Aspect_VKeyMouse_LeftButton;
        }
        if ((buttons & Qt::MiddleButton) != 0)
        {
            vkey_mouse |= Aspect_VKeyMouse_MiddleButton;
        }
        if ((buttons & Qt::RightButton) != 0)
        {
            vkey_mouse |= Aspect_VKeyMouse_RightButton;
        }
        return vkey_mouse;
    }

    Aspect_VKeyFlags QtKeyboardModifiers2VKeyFlags(Qt::KeyboardModifiers modifiers)
    {
        Aspect_VKeyFlags vkey_flags = Aspect_VKeyFlags_NONE;
        if ((modifiers & Qt::ShiftModifier) != 0)
        {
            vkey_flags |= Aspect_VKeyFlags_SHIFT;
        }
        if ((modifiers & Qt::ControlModifier) != 0)
        {
            vkey_flags |= Aspect_VKeyFlags_CTRL;
        }
        if ((modifiers & Qt::AltModifier) != 0)
        {
            vkey_flags |= Aspect_VKeyFlags_ALT;
        }
        return vkey_flags;
    }

    Aspect_VKey QtKeys2VKeys(int key)
    {
        static const std::unordered_map<int, Aspect_VKey> keyMap =
        {
            {Qt::Key_unknown, Aspect_VKey_UNKNOWN},
            {Qt::Key_A, Aspect_VKey_A},
            {Qt::Key_B, Aspect_VKey_B},
            {Qt::Key_C, Aspect_VKey_C},
            {Qt::Key_D, Aspect_VKey_D},
            {Qt::Key_E, Aspect_VKey_E},
            {Qt::Key_F, Aspect_VKey_F},
            {Qt::Key_G, Aspect_VKey_G},
            {Qt::Key_H, Aspect_VKey_H},
            {Qt::Key_I, Aspect_VKey_I},
            {Qt::Key_J, Aspect_VKey_J},
            {Qt::Key_K, Aspect_VKey_K},
            {Qt::Key_L, Aspect_VKey_L},
            {Qt::Key_M, Aspect_VKey_M},
            {Qt::Key_N, Aspect_VKey_N},
            {Qt::Key_O, Aspect_VKey_O},
            {Qt::Key_P, Aspect_VKey_P},
            {Qt::Key_Q, Aspect_VKey_Q},
            {Qt::Key_R, Aspect_VKey_R},
            {Qt::Key_S, Aspect_VKey_S},
            {Qt::Key_T, Aspect_VKey_T},
            {Qt::Key_U, Aspect_VKey_U},
            {Qt::Key_V, Aspect_VKey_V},
            {Qt::Key_W, Aspect_VKey_W},
            {Qt::Key_X, Aspect_VKey_X},
            {Qt::Key_Y, Aspect_VKey_Y},
            {Qt::Key_Z, Aspect_VKey_Z},
            {Qt::Key_Delete, Aspect_VKey_Delete}
        };

        auto it = keyMap.find(key);
        return (it != keyMap.end()) ? it->second : Aspect_VKey_UNKNOWN;
    }
} // namespace

Viewer::Viewer(QWidget* parent)
    : QWidget{ parent }
{

    this->setMouseTracking(true);   // Needed to generate mouse events
    this->setAutoFillBackground(false); // Avoid Qt background clears to improve resizing speed
    this->setFocusPolicy(Qt::StrongFocus);  // set focus policy to to handle keyboard events
    this->setAttribute(Qt::WA_NoSystemBackground, true);
    this->setAttribute(Qt::WA_PaintOnScreen, true);

    init();
}

Viewer::~Viewer()
{
}

void Viewer::init()
{
    Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();

    // Create OCCT viewer
    Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection, false);
    m_viewer = new V3d_Viewer(graphicDriver);

    // Lightning
    Handle(V3d_DirectionalLight) lightDir = new V3d_DirectionalLight(V3d_Zneg, Quantity_Color(Quantity_NOC_GRAY97), 1);
    Handle(V3d_AmbientLight) lightAmb = new V3d_AmbientLight();
    lightDir->SetDirection(1.0, -2.0, -10.0);
    m_viewer->AddLight(lightDir);
    m_viewer->AddLight(lightAmb);
    m_viewer->SetLightOn(lightDir);
    m_viewer->SetLightOn(lightAmb);

    // AIS context
    m_context = new AIS_InteractiveContext(m_viewer);

    // Configure some global props
    const Handle(Prs3d_Drawer)& contextDrawer = m_context->DefaultDrawer();
    //
    if (!contextDrawer.IsNull())
    {
        const Handle(Prs3d_ShadingAspect)& SA = contextDrawer->ShadingAspect();
        const Handle(Graphic3d_AspectFillArea3d)& FA = SA->Aspect();
        contextDrawer->SetFaceBoundaryDraw(true); // Draw edges.
        FA->SetEdgeOff();

        // Fix for infinite lines has been reduced to 1000 from its default value 500000.
        contextDrawer->SetMaximalParameterValue(1000);
    }

    // Main view creation
    m_view = m_viewer->CreateView();
    m_view->SetImmediateUpdate(false);

    // Aspect window creation, here use WNT_Window
    WId windowHandle = winId();
    m_wntWindow = new WNT_Window((Aspect_Handle)windowHandle);
    m_view->SetWindow(m_wntWindow, nullptr);
    if (!m_wntWindow->IsMapped())
    {
        m_wntWindow->Map();
    }

    // View settings
    SetAllowRotation(Standard_True);
    m_view->SetBackgroundColor(Quantity_Color(0.20, 0.20, 0.40, Quantity_TOC_RGB));
    m_view->SetShadingModel(V3d_PHONG);

    // Configure rendering parameters
    Graphic3d_RenderingParams& RenderParams = m_view->ChangeRenderingParams();
    RenderParams.IsAntialiasingEnabled = true;
    RenderParams.NbMsaaSamples = 8; // Anti-aliasing by multi-sampling
    RenderParams.IsShadowEnabled = false;
    RenderParams.CollectedStats = Graphic3d_RenderingParams::PerfCounters_NONE;
}

void Viewer::setStatusBar(QStatusBar* statusBar)
{
    m_statusBar = statusBar;
}

void Viewer::paintEvent(QPaintEvent* event)
{
    // Very Necessary!!!
    if (!m_view.IsNull())
    {
        m_view->Invalidate();
        FlushViewEvents(m_context, m_view, true);
    }
}

void Viewer::resizeEvent(QResizeEvent* event)
{
    if (!m_view.IsNull())
    {
        m_view->MustBeResized();
    }
}

void Viewer::keyPressEvent(QKeyEvent* event)
{
    Aspect_VKey vkey = ::QtKeys2VKeys(event->key());
    if (vkey != Aspect_VKey_UNKNOWN)
    {
        double timeStamp = EventTime();
        KeyDown(vkey, timeStamp);
    }
}

void Viewer::keyReleaseEvent(QKeyEvent* event)
{
    Aspect_VKey vkey = ::QtKeys2VKeys(event->key());
    if (vkey != Aspect_VKey_UNKNOWN)
    {
        double timeStamp = EventTime();
        KeyUp(vkey, timeStamp);
    }

    auto modifiers = myKeys.Modifiers();
    vkey = vkey | modifiers;

    switch (vkey)
    {
    case Aspect_VKey_F:
    {
        if (m_context->NbSelected() > 0)
        {
            m_context->FitSelected(m_view);
        }
        else
        {
            fitView();
        }
        break;
    }
    case Aspect_VKey_S:
    {
        shadingView();
        break;
    }
    case Aspect_VKey_W:
    {
        wireframeView();
        break;
    }
    default:
        break;
    }
}

void Viewer::mousePressEvent(QMouseEvent* event)
{
    Qt::MouseButtons buttons = event->buttons();
    Aspect_VKeyMouse vbuttons = ::QtMouseButtons2VKeyMouse(buttons);
    auto dpi = this->devicePixelRatio();
    QPoint pos = event->pos();
    const Graphic3d_Vec2i pnt(pos.x() * dpi, pos.y() * dpi);
    const Aspect_VKeyFlags flags = ::QtKeyboardModifiers2VKeyFlags(event->modifiers());
    if (!m_view.IsNull() && UpdateMouseButtons(pnt, vbuttons, flags, false))
    {
        update();
    }

    if (vbuttons == Aspect_VKeyMouse_RightButton)  // add curve/surface
    {
        QString message;
        TopoDS_Shape shape = getShape();
        if (shape.IsNull()) {
            return;
        }
        auto type = shape.ShapeType();

        if (type == TopAbs_EDGE)
        {
            Handle(Geom_BSplineCurve) bsplineCurve;
            TopoDS_Edge edge;
            bool isEdge = util::convertToBSplineCurve(shape, edge, bsplineCurve);
            if (isEdge)
            {
                m_bsplineCurves.emplace_back(bsplineCurve);
                message = ::processString(string1, m_bsplineCurves.size());
                showMessage(message);
            }
        }
    }
}

void Viewer::mouseReleaseEvent(QMouseEvent* event)
{
    Qt::MouseButtons buttons = event->buttons();
    Aspect_VKeyMouse vbuttons = ::QtMouseButtons2VKeyMouse(buttons);
    auto dpi = this->devicePixelRatio();
    QPoint pos = event->pos();
    const Graphic3d_Vec2i pnt(pos.x() * dpi, pos.y() * dpi);
    const Aspect_VKeyFlags flags = ::QtKeyboardModifiers2VKeyFlags(event->modifiers());
    if (!m_view.IsNull() && UpdateMouseButtons(pnt, vbuttons, flags, false))
    {
        update();
    }
}

void Viewer::mouseMoveEvent(QMouseEvent* event)
{
    Qt::MouseButtons mouse_buttons = event->buttons();
    auto dpi = this->devicePixelRatio();
    QPoint pos = event->pos();
    const Graphic3d_Vec2i new_pos(pos.x() * dpi, pos.y() * dpi);
    if (!m_view.IsNull() && UpdateMousePosition(new_pos, ::QtMouseButtons2VKeyMouse(mouse_buttons),
        ::QtKeyboardModifiers2VKeyFlags(event->modifiers()), false))
    {
        update();
    }
}

void Viewer::wheelEvent(QWheelEvent* event)
{
    // get the roller rolling increase
    QPoint delta = event->angleDelta();
    double deltaF = delta.y() / 8.0;
    // get mouse position
    QPointF position = event->position();
    // convert to Graphic3d_Vec2i type
    Graphic3d_Vec2i pos(position.x(), position.y());

    if (!m_view.IsNull() && UpdateZoom(Aspect_ScrollDelta(pos, deltaF)))
    {
        update();
    }
}

QPaintEngine* Viewer::paintEngine() const
{
    return nullptr; // Indicate we use OpenGL for rendering
}

void Viewer::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", 
        tr("All Files (*);;Step Files (*.stp *.step);;Iges Files(*.igs *.iges)"));
    if (fileName.isEmpty())
    {
        return;
    }
    std::string filename_s = fileName.toStdString(); // convert to std::string type

    // read step file
    Handle(TopTools_HSequenceOfShape) hSequenceOfShape = new TopTools_HSequenceOfShape();
    io::readModel(filename_s.c_str(), hSequenceOfShape);

    this->clear();

    for (int ix = 1; ix <= hSequenceOfShape->Length(); ix++)
    {
        TopoDS_Shape shape = hSequenceOfShape->Value(ix);
        this->operator<<(shape);
    }
    updateView();
}

void Viewer::save()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save STEP File"), "", tr("Step Files (*.stp *.step)"));
    if (fileName.isEmpty())
    {
        return;
    }
    // if not ended with ".stp" or ".step"
    if (!fileName.endsWith(".stp", Qt::CaseInsensitive) && !fileName.endsWith(".step", Qt::CaseInsensitive))
    {
        fileName += ".stp";
    }
    std::string filename_s = fileName.toStdString();

    Handle(TopTools_HSequenceOfShape) hSequenceOfShape = new TopTools_HSequenceOfShape();
    for (auto& shape : m_shapes)
    {
        hSequenceOfShape->Append(shape);
    }
    io::saveStep(filename_s.c_str(), hSequenceOfShape, STEPControl_AsIs);
}

void Viewer::clear()
{
    m_bsplineCurves.clear();
    m_shapes.clear();
    m_context->RemoveAll(Standard_True);    // remove visualization objects
}

void Viewer::skin()
{
    if (m_bsplineCurves.empty())
    {
        QMessageBox::warning(this, "Warning", "Section curves empty! Please select curve!");
        return;
    }

    // skin
    int degree = 3;
    Skin skin(m_bsplineCurves, degree);
    skin.skin();
    Handle(Geom_BSplineSurface) surface = skin.getSurface();
    if (!surface.IsNull())
    {
        std::cout << "successful!" << std::endl;
    }
    else
    {
        std::cout << "failed!" << std::endl;
    }
    TopoDS_Shape face = BRepBuilderAPI_MakeFace(surface, Precision::Confusion());

    this->operator<<(face);

    updateView();
}

void Viewer::updateView()
{
    m_context->RemoveAll(Standard_True);
    for (auto& sh : m_shapes)
    {
        Handle(AIS_Shape) shape = new AIS_Shape(sh);
        m_context->SetDisplayMode(shape, AIS_Shaded, Standard_True);
        m_context->Display(shape, Standard_True);
    }

    fitView();
    m_view->MustBeResized();
    ::AdjustSelectionStyle(m_context);
}

TopoDS_Shape Viewer::getShape()
{
    Standard_Boolean flag = m_context->HasDetected();
    if (!flag)
    {
        return TopoDS_Shape();
    }

    TopoDS_Shape shape = m_context->DetectedShape();
    return shape;
}

void Viewer::operator<<(const TopoDS_Shape& shape)
{
    m_shapes.emplace_back(shape);
}

void Viewer::fitView()
{
    if (!m_view.IsNull())
    {
        m_view->FitAll();
    }
}

void Viewer::shadingView()
{
    if (m_context->NbSelected() > 0)
    {
        m_context->InitSelected();
        m_context->SetDisplayMode(m_context->SelectedInteractive(), AIS_Shaded, Standard_True);
    }
    else
    {
        m_context->SetDisplayMode(AIS_Shaded, Standard_True);
    }
}

void Viewer::wireframeView()
{
    if (m_context->NbSelected() > 0)
    {
        m_context->InitSelected();
        m_context->SetDisplayMode(m_context->SelectedInteractive(), AIS_WireFrame, Standard_True);
    }
    else
    {
        m_context->SetDisplayMode(AIS_WireFrame, Standard_True);
    }
}

void Viewer::showMessage(const QString& message)
{
    m_statusBar->showMessage(message);
}