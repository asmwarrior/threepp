#include "threepp/threepp.hpp"

using namespace threepp;

#include <glad/glad.h> //gladLoadGL(), must be included before glcanvas.h
#include <wx/wx.h>
#include <wx/glcanvas.h>

class MyApp : public wxApp
{
public:
    MyApp() {}
    bool OnInit() wxOVERRIDE;
};

class OpenGLCanvas;

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString &title);

private:
    OpenGLCanvas *openGLCanvas{nullptr};
};

class OpenGLCanvas : public wxGLCanvas, public PeripheralsEventSource
{
public:
    OpenGLCanvas(MyFrame *parent, const wxGLAttributes &canvasAttrs);
    ~OpenGLCanvas();

    bool InitializeOpenGLFunctions();
    bool InitializeOpenGL();

    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);

    void OnMouseMove(wxMouseEvent& event);
    void OnMousePress(wxMouseEvent& event);
    void OnMouseRelease(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnKeyPress(wxKeyEvent& event);

    wxColour triangleColor{wxColour(255, 128, 51)};

    [[nodiscard]] virtual WindowSize size() const override;

private:
    wxGLContext *openGLContext;
    bool isOpenGLInitialized{false};

    //////////////////////////////////////////////////////////////////////////////
    std::shared_ptr<GLRenderer> renderer;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<PerspectiveCamera> camera;
    std::shared_ptr<OrbitControls> controls;
    std::shared_ptr<HUD> hud;

    FontLoader fontLoader;

    Font font1;
    Font font2;

    std::shared_ptr<TextGeometry::Options> opts1;
    std::shared_ptr<Text2D> hudText1;

    std::shared_ptr<TextGeometry::Options> opts2;
    std::shared_ptr<Text2D> hudText2;

    //////////////////////////////////////////////////////////////////////////////

};

auto createBox() {

    const auto boxGeometry = BoxGeometry::create();
    const auto boxMaterial = MeshBasicMaterial::create();
    boxMaterial->color.setRGB(1, 0, 0);
    boxMaterial->transparent = true;
    boxMaterial->opacity = 0.1f;
    auto box = Mesh::create(boxGeometry, boxMaterial);

    auto wiredBox = LineSegments::create(WireframeGeometry::create(*boxGeometry));
    wiredBox->material()->as<LineBasicMaterial>()->depthTest = false;
    wiredBox->material()->as<LineBasicMaterial>()->color = Color::gray;
    box->add(wiredBox);

    return box;
}

auto createSphere() {

    const auto sphereGeometry = SphereGeometry::create(0.5f);
    const auto sphereMaterial = MeshBasicMaterial::create();
    sphereMaterial->color.setHex(0x00ff00);
    sphereMaterial->wireframe = true;
    auto sphere = Mesh::create(sphereGeometry, sphereMaterial);
    sphere->position.setX(-1);

    return sphere;
}

auto createPlane() {

    const auto planeGeometry = PlaneGeometry::create(5, 5);
    const auto planeMaterial = MeshBasicMaterial::create();
    planeMaterial->color.setHex(Color::yellow);
    planeMaterial->transparent = true;
    planeMaterial->opacity = 0.5f;
    planeMaterial->side = Side::Double;
    auto plane = Mesh::create(planeGeometry, planeMaterial);
    plane->position.setZ(-2);

    return plane;
}

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    MyFrame *frame = new MyFrame("Hello ThreePP + wxWidgets");
    frame->Show(true);

    return true;
}

MyFrame::MyFrame(const wxString &title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxDefaultSize)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);

    wxGLAttributes vAttrs;
    vAttrs.PlatformDefaults().Defaults().EndList();

    if (wxGLCanvas::IsDisplaySupported(vAttrs))
    {
        openGLCanvas = new OpenGLCanvas(this, vAttrs);
        openGLCanvas->SetMinSize(FromDIP(wxSize(640, 480)));
        sizer->Add(openGLCanvas, 1, wxEXPAND);
    }

    SetSizerAndFit(sizer);
}

OpenGLCanvas::OpenGLCanvas(MyFrame *parent, const wxGLAttributes &canvasAttrs)
    : wxGLCanvas(parent, canvasAttrs)
{
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().CoreProfile().OGLVersion(3, 3).EndList();
    openGLContext = new wxGLContext(this, nullptr, &ctxAttrs);

    if (!openGLContext->IsOK())
    {
        wxMessageBox("This sample needs an OpenGL 3.3 capable driver.",
                     "OpenGL version error", wxOK | wxICON_INFORMATION, this);
        delete openGLContext;
        openGLContext = nullptr;
    }

    Bind(wxEVT_PAINT,       &OpenGLCanvas::OnPaint, this);
    Bind(wxEVT_SIZE,        &OpenGLCanvas::OnSize, this);

    Bind (wxEVT_MOTION,     &OpenGLCanvas::OnMouseMove, this);
    Bind (wxEVT_LEFT_DOWN,  &OpenGLCanvas::OnMousePress, this);
    Bind (wxEVT_RIGHT_DOWN, &OpenGLCanvas::OnMousePress, this);
    Bind (wxEVT_LEFT_UP,    &OpenGLCanvas::OnMouseRelease, this);
    Bind (wxEVT_RIGHT_UP,   &OpenGLCanvas::OnMouseRelease, this);
    Bind (wxEVT_MOUSEWHEEL, &OpenGLCanvas::OnMouseWheel, this);
    Bind (wxEVT_KEY_DOWN,   &OpenGLCanvas::OnKeyPress, this);

}

OpenGLCanvas::~OpenGLCanvas()
{
    delete openGLContext;
}

bool OpenGLCanvas::InitializeOpenGLFunctions()
{
    gladLoadGL();
    return true;
}

bool OpenGLCanvas::InitializeOpenGL()
{
    if (!openGLContext)
    {
        return false;
    }

    SetCurrent(*openGLContext);

    if (!InitializeOpenGLFunctions())
    {
        wxMessageBox("Error: Could not initialize OpenGL function pointers.",
                     "OpenGL initialization error", wxOK | wxICON_INFORMATION, this);
        return false;
    }

    wxLogDebug("OpenGL version: %s", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
    wxLogDebug("OpenGL vendor: %s", reinterpret_cast<const char *>(glGetString(GL_VENDOR)));


    //////////////////////////////////////////////////////////////////////////////////////

    auto viewPortSize = GetSize() * GetContentScaleFactor();
    WindowSize size{viewPortSize.x, viewPortSize.y};
    renderer = std::make_shared<GLRenderer>(size);
    renderer->autoClear = false;

    scene = Scene::create();
    scene->background = Color::aliceblue;
    camera = PerspectiveCamera::create(75, size.aspect(), 0.1f, 1000);
    camera->position.z = 5;

    controls = std::make_shared<OrbitControls>(*camera, *this); // add this line

    auto box = createBox();
    scene->add(box);

    auto sphere = createSphere();
    box->add(sphere);

    auto plane = createPlane();
    auto planeMaterial = plane->material()->as<MeshBasicMaterial>();
    scene->add(plane);

    WindowSize s = this->size();

    hud = std::make_shared<HUD>(s);

    font1 = fontLoader.defaultFont();
    font2 = *fontLoader.load("data/fonts/helvetiker_regular.typeface.json");

    opts1 = std::make_shared<TextGeometry::Options>(font1, 40);
    hudText1 = std::make_shared<Text2D>(*opts1, "Hello World!");
    hudText1->setColor(Color::black);
    hud->add(*hudText1, HUD::Options());

    opts2 = std::make_shared<TextGeometry::Options>(font2, 10, 1);
    hudText2 = std::make_shared<Text2D>(*opts1, "");
    hudText2->setColor(Color::red);
    hud->add(*hudText2, HUD::Options()
                              .setNormalizedPosition({1, 1})
                              .setHorizontalAlignment(threepp::HUD::HorizontalAlignment::RIGHT)
                              .setVerticalAlignment(threepp::HUD::VerticalAlignment::TOP));

    hudText2->setText("Delta=1.23456789", *opts2);
    hud->needsUpdate(*hudText2);

    //////////////////////////////////////////////////////////////////////////////////////

    isOpenGLInitialized = true;

    return true;
}

void OpenGLCanvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    wxPaintDC dc(this);

    if (!isOpenGLInitialized)
    {
        return;
    }

    SetCurrent(*openGLContext);

    renderer->clear();
    renderer->render(*scene, *camera);
    hud->apply(*renderer);

    SwapBuffers();
}

void OpenGLCanvas::OnSize(wxSizeEvent &event)
{
    bool firstApperance = IsShownOnScreen() && !isOpenGLInitialized;

    if (firstApperance)
    {
        InitializeOpenGL();
    }

    if (isOpenGLInitialized)
    {
        auto viewPortSize = event.GetSize() * GetContentScaleFactor();
        glViewport(0, 0, viewPortSize.x, viewPortSize.y);

        WindowSize size{viewPortSize.x, viewPortSize.y};

        camera->aspect = size.aspect();
        camera->updateProjectionMatrix();
        renderer->setSize(size);

        hud->setSize(size);
    }

    event.Skip();
}

WindowSize OpenGLCanvas::size() const
{
    auto viewPortSize = GetSize() * GetContentScaleFactor();
    WindowSize size{viewPortSize.x, viewPortSize.y};
    return size;
}

void OpenGLCanvas::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    Vector2 mousePos(static_cast<float>(pos.x), static_cast<float>(pos.y));
    onMouseMoveEvent(mousePos);
    Refresh (false);
    event.Skip();
}

void OpenGLCanvas::OnMousePress(wxMouseEvent& event)
{
    int buttonFlag = event.GetButton();
    wxPoint pos = event.GetPosition();
    int button = 0;
    if (wxMOUSE_BTN_LEFT == buttonFlag)
        button = 0;
    else if (wxMOUSE_BTN_RIGHT == buttonFlag)
        button = 1;
    Vector2 p{pos.x,pos.y};
    onMousePressedEvent(button, p, PeripheralsEventSource::MouseAction::PRESS);
    Refresh (false);
    event.Skip();
}

void OpenGLCanvas::OnMouseRelease(wxMouseEvent& event)
{
    int buttonFlag = event.GetButton();
    wxPoint pos = event.GetPosition();
    int button = 0;
    if (wxMOUSE_BTN_LEFT == buttonFlag)
        button = 0;
    else if (wxMOUSE_BTN_RIGHT == buttonFlag)
        button = 1;
    Vector2 p{pos.x,pos.y};
    onMousePressedEvent(button, p, PeripheralsEventSource::MouseAction::RELEASE);
    Refresh (false);
    event.Skip();
}

void OpenGLCanvas::OnMouseWheel(wxMouseEvent& event)
{
    int direction = event.GetWheelRotation()/120; // 1 or -1
    int xoffset = 0;
    int yoffset = direction;

    // call the PeripheralsEventSource's member function
    onMouseWheelEvent({static_cast<float>(xoffset), static_cast<float>(yoffset)});

    Refresh (false);
    event.Skip();
}

void OpenGLCanvas::OnKeyPress(wxKeyEvent& event)
{
    // Code to handle key press event
}
