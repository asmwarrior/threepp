#ifndef OPENGLCANVAS_H_INCLUDED
#define OPENGLCANVAS_H_INCLUDED

#include "threepp/threepp.hpp"

using namespace threepp;

#include <glad/glad.h> //gladLoadGL(), must be included before glcanvas.h
#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "MyOrbitControls.hpp"


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

    wxColour triangleColor{wxColour(255, 128, 51)};

    WindowSize size();

private:
    wxGLContext *openGLContext;
    bool isOpenGLInitialized{false};

    //////////////////////////////////////////////////////////////////////////////
    std::shared_ptr<GLRenderer> renderer;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<PerspectiveCamera> camera;
    std::shared_ptr<MyOrbitControls> controls;
    //////////////////////////////////////////////////////////////////////////////

};


#endif // OPENGLCANVAS_H_INCLUDED
