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
    void OnKeyUp(wxKeyEvent& event);

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

    std::vector<std::shared_ptr<Text2D>> textMesh2dArray;

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
    Bind (wxEVT_KEY_UP,     &OpenGLCanvas::OnKeyUp, this);

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

    // billboard text labels
    float textSize = 0.02;
    std::string displayText = "threepp!";

    const auto material1 = SpriteMaterial::create();
    material1->side = Side::Double;
    material1->color = Color::green;
    material1->sizeAttenuation = false;


    textMesh2dArray.push_back(Text2D::create(TextGeometry::Options(font2, textSize), displayText, material1));
    textMesh2dArray.push_back(Text2D::create(TextGeometry::Options(font2, textSize), displayText, material1));

    textMesh2dArray[0]->position.z = 5;
    textMesh2dArray[1]->position.z = -5;

//    textMesh2dArray[0]->scale.set(0.05,0.05,0.05);
//    textMesh2dArray[1]->scale.set(0.05,0.05,0.05);


    textMesh2dArray[0]->geometry()->center();
    textMesh2dArray[1]->geometry()->center();

    scene->add(*(textMesh2dArray[0]));
    scene->add(*(textMesh2dArray[1]));

    // add 3D lines
    // Create a material for the lines
    auto lineMaterial = threepp::LineBasicMaterial::create();
    lineMaterial->color.setRGB(1, 0, 0);

    // Create geometry for the lines
    auto lineGeometry = threepp::BufferGeometry::create();
    std::vector<float> lineVertices = {
        -1, 0, 0,  // Start point of line 1
        1, 0, 0,   // End point of line 1
        0, -1, 0,  // Start point of line 2
        0, 1, 0    // End point of line 2
    };
    lineGeometry->setAttribute("position", threepp::FloatBufferAttribute::create(lineVertices, 3));

    // Create the line object
    auto line = threepp::LineSegments::create(lineGeometry, lineMaterial);
    scene->add(line);

    // axis

    auto axis = threepp::AxesHelper::create(5);
    scene->add(axis);

    // points
    const int numParticles = 5;
    std::vector<float> positions(numParticles * 3);
    std::vector<float> colors(numParticles * 3);

    const float n = 10;
    const float n2 = n / 2;

    for (int i = 0; i < numParticles; i += 3) {
        positions[i] = (math::randFloat() * n - n2);
        positions[i + 1] = (math::randFloat() * n - n2);
        positions[i + 2] = (math::randFloat() * n - n2);

        colors[i] = ((positions[i] / n) + 0.5f);
        colors[i + 1] = ((positions[i + 1] / n) + 0.5f);
        colors[i + 2] = ((positions[i + 2] / n) + 0.5f);
    }

    auto geometry = BufferGeometry::create();
    geometry->setAttribute("position", FloatBufferAttribute::create(positions, 3));
    geometry->setAttribute("color", FloatBufferAttribute::create(colors, 3));

    geometry->computeBoundingSphere();

    auto material = PointsMaterial::create();
    material->size = 0.2;
    material->vertexColors = true;

    auto points = Points::create(geometry, material);
    scene->add(points);

    //


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

Key wxKeyCodeToKey(int wx_keycode)
{
    // defined in \include\wx-3.2\wx\defs.h
    switch (wx_keycode) {
        case '0': return Key::NUM_0;
        case '1': return Key::NUM_1;
        case '2': return Key::NUM_2;
        case '3': return Key::NUM_3;
        case '4': return Key::NUM_4;
        case '5': return Key::NUM_5;
        case '6': return Key::NUM_6;
        case '7': return Key::NUM_7;
        case '8': return Key::NUM_8;
        case '9': return Key::NUM_9;

        case WXK_F1: return Key::F1;
        case WXK_F2: return Key::F2;
        case WXK_F3: return Key::F3;
        case WXK_F4: return Key::F4;
        case WXK_F5: return Key::F5;
        case WXK_F6: return Key::F6;
        case WXK_F7: return Key::F7;
        case WXK_F8: return Key::F8;
        case WXK_F9: return Key::F9;
        case WXK_F10: return Key::F10;
        case WXK_F11: return Key::F11;
        case WXK_F12: return Key::F12;

        case 'A': return Key::A;
        case 'B': return Key::B;
        case 'C': return Key::C;
        case 'D': return Key::D;
        case 'E': return Key::E;
        case 'F': return Key::F;
        case 'G': return Key::G;
        case 'H': return Key::H;
        case 'I': return Key::I;
        case 'J': return Key::J;
        case 'K': return Key::K;
        case 'L': return Key::L;
        case 'M': return Key::M;
        case 'N': return Key::N;
        case 'O': return Key::O;
        case 'P': return Key::P;
        case 'Q': return Key::Q;
        case 'R': return Key::R;
        case 'S': return Key::S;
        case 'T': return Key::T;
        case 'U': return Key::U;
        case 'V': return Key::V;
        case 'W': return Key::W;
        case 'X': return Key::X;
        case 'Y': return Key::Y;
        case 'Z': return Key::Z;

        case WXK_UP: return Key::UP;
        case WXK_DOWN: return Key::DOWN;
        case WXK_LEFT: return Key::LEFT;
        case WXK_RIGHT: return Key::RIGHT;

        case WXK_SPACE: return Key::SPACE;
//        case WXK_COMMA: return Key::COMMA;
//        case WXK_MINUS: return Key::MINUS;
//        case WXK_PERIOD: return Key::PERIOD;
//        case WXK_SLASH: return Key::SLASH;

        case WXK_RETURN: return Key::ENTER;
        case WXK_TAB: return Key::TAB;
        case WXK_BACK: return Key::BACKSLASH;
        case WXK_INSERT: return Key::INSERT;

// DELETE was defined in mingw, see:
// /mingw64/include/winnt.h:3009
// #define DELETE (__MSABI_LONG(0x00010000))
//        case WXK_DELETE: return Key::DELETE;

        default: return Key::UNKNOWN;
    }
}

void OpenGLCanvas::OnKeyPress(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    int mods = event.GetModifiers();
    int scancode = key; // not sure what does the scancode mean
    KeyEvent evt{wxKeyCodeToKey(key), scancode, mods};
    onKeyEvent(evt, PeripheralsEventSource::KeyAction::PRESS);
}

void OpenGLCanvas::OnKeyUp(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    int mods = event.GetModifiers();
    int scancode = key; // not sure what does the scancode mean
    KeyEvent evt{wxKeyCodeToKey(key), scancode, mods};
    onKeyEvent(evt, PeripheralsEventSource::KeyAction::RELEASE);
}
