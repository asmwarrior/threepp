#include "threepp/threepp.hpp"

#include <threepp/materials/ShaderMaterial.hpp>
#include "threepp/materials/RawShaderMaterial.hpp"
#include <threepp/core/Uniform.hpp>
#include <threepp/objects/Points.hpp>
#include <threepp/core/BufferGeometry.hpp>
#include <threepp/materials/Material.hpp>
#include <threepp/math/Ray.hpp>
#include <threepp/math/Matrix4.hpp>
#include <threepp/core/BufferAttribute.hpp>
#include <threepp/core/Raycaster.hpp>

using namespace threepp;

#include <glad/glad.h> //gladLoadGL(), must be included before glcanvas.h
#include <wx/wx.h>
#include <wx/glcanvas.h>


// Now, undefine the problematic macros right after the include
#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif



//class CustomPoints : public threepp::Points {
//public:
//    using Ptr = std::shared_ptr<CustomPoints>;
//
//    static Ptr create(const std::shared_ptr<threepp::BufferGeometry>& geometry,
//                      const std::shared_ptr<threepp::Material>& material) {
//        return Ptr(new CustomPoints(geometry, material));
//    }
//
//    void raycast(const threepp::Raycaster& raycaster,
//                 std::vector<threepp::Intersection>& intersects) override {
//
//        auto material_ptr = std::dynamic_pointer_cast<threepp::RawShaderMaterial>(this->material());
//        if (!material_ptr) {
//             // You could check for PointsMaterial here as a fallback
//            auto points_material_ptr = std::dynamic_pointer_cast<threepp::PointsMaterial>(this->material());
//            if (points_material_ptr) {
//                 // Or just return and handle it in a different way
//            }
//            std::cout << "No valid material for raycasting, returning.\n";
//            return;
//        }
//
//        auto positions = geometry()->getAttribute<float>("position");
//        if (!positions) {
//            std::cout << "No position attribute, returning.\n";
//            return;
//        }
//
//        // Get pointSize uniform, or use a default
//        float pointSize_px = 0.0f;
//        if (material_ptr->uniforms.count("pointSize")) {
//            pointSize_px = material_ptr->uniforms.at("pointSize").value<float>();
//        } else {
//            pointSize_px = 1.0f;
//        }
//
//        // Create a temporary matrix for calculations
//        threepp::Matrix4 tempMatrix;
//        tempMatrix.copy(*this->matrixWorld).invert();
//
//        // Apply object's inverted world matrix to the ray to get it into local space
//        threepp::Ray localRay = raycaster.ray;
//        localRay.applyMatrix4(tempMatrix);
//
//        // This is a placeholder, you must get the actual height from your canvas/renderer
//        float rendererHeight = 600.0f; // REPLACE with the actual height of your OpenGL canvas
//
//        for (size_t i = 0; i < positions->count(); ++i) {
//            threepp::Vector3 point_local(
//                positions->getX(i),
//                positions->getY(i),
//                positions->getZ(i)
//            );
//
//            // Get the world-space position of the point
//            threepp::Vector3 point_world;
//            point_world.copy(point_local).applyMatrix4(*this->matrixWorld);
//
//            // Calculate the threshold dynamically
//            float threshold = 0.5f; // Fallback value
//
//            // Use dynamic_cast for raw pointers to check the camera type
//            auto perspectiveCamera = dynamic_cast<threepp::PerspectiveCamera*>(raycaster.camera);
//            if (perspectiveCamera) {
//                float distToCamera = point_world.distanceTo(raycaster.camera->position);
//                float fov_rad = perspectiveCamera->fov * M_PI / 180.0f;
//                threshold = (pointSize_px * distToCamera * tan(fov_rad * 0.5f)) / (rendererHeight / 2.0f);
//            }
//
//            float distToRay = raycaster.ray.distanceToPoint(point_world);
//
//            if (distToRay < threshold) {
//                // Find the closest point on the ray to the point in world space.
//                threepp::Vector3 intersectionPoint;
//                raycaster.ray.closestPointToPoint(point_world, intersectionPoint);
//
//                // Check if the intersection point is within the ray's near and far planes.
//                float distFromRayOrigin = raycaster.ray.origin.distanceTo(intersectionPoint);
//
//                // Use parentheses to prevent macro collision with `near` and `far`
//                if (distFromRayOrigin >= (raycaster.near) && distFromRayOrigin <= (raycaster.far)) {
//                    threepp::Intersection intersection;
//                    intersection.distance = distFromRayOrigin;
//                    intersection.object = this;
//                    intersection.point = intersectionPoint;
//                    intersection.index = static_cast<int>(i);
//                    intersects.push_back(intersection);
//                }
//            }
//        }
//    }
//
//protected:
//    CustomPoints(const std::shared_ptr<threepp::BufferGeometry>& geometry,
//                 const std::shared_ptr<threepp::Material>& material)
//        : threepp::Points(geometry, material) {}
//};


class CustomPoints : public threepp::Points {
public:
    using Ptr = std::shared_ptr<CustomPoints>;

    static Ptr create(
        const std::shared_ptr<threepp::BufferGeometry>& geometry,
        const std::shared_ptr<threepp::Material>& material) {
        return Ptr(new CustomPoints(geometry, material));
    }

    void raycast(const threepp::Raycaster& raycaster,
                               std::vector<threepp::Intersection>& intersects) {

        auto material_ptr = std::dynamic_pointer_cast<threepp::RawShaderMaterial>(this->material());
        if (!material_ptr) {
            return;
        }

        auto positions = geometry()->getAttribute<float>("position");
        if (!positions) {
            return;
        }

        float pointSize_px = 0.0f;
        if (material_ptr->uniforms.count("pointSize")) {
            pointSize_px = material_ptr->uniforms.at("pointSize").value<float>();
        } else {
            pointSize_px = 1.0f;
        }

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        float rendererWidth = static_cast<float>(viewport[2]);
        float rendererHeight = static_cast<float>(viewport[3]);

        threepp::Matrix4 pvm;
        // --- FIX FOR THE 'OPERATOR*' ERROR ---
        // Use .get() to retrieve the raw pointer and then dereference it.
        // This bypasses the ambiguity the compiler is facing.
        pvm.multiplyMatrices(raycaster.camera->projectionMatrix, raycaster.camera->matrixWorldInverse);

        // --- FIX FOR THE SIGNEDNESS WARNING ---
        // Cast the count() return value to size_t to make the comparison safe.
        for (size_t i = 0; i < static_cast<size_t>(positions->count()); ++i) {
            threepp::Vector3 point_local(
                positions->getX(i),
                positions->getY(i),
                positions->getZ(i)
            );

            threepp::Vector3 point_world;
            point_world.copy(point_local).applyMatrix4(*this->matrixWorld);

            threepp::Vector4 clipSpacePoint;
            clipSpacePoint.set(point_world.x, point_world.y, point_world.z, 1.0f);
            clipSpacePoint.applyMatrix4(pvm);

            threepp::Vector3 ndcPoint(
                clipSpacePoint.x / clipSpacePoint.w,
                clipSpacePoint.y / clipSpacePoint.w,
                clipSpacePoint.z / clipSpacePoint.w
            );

            threepp::Vector2 screenPoint(
                (ndcPoint.x * 0.5f + 0.5f) * rendererWidth,
                (1.0f - (ndcPoint.y * 0.5f + 0.5f)) * rendererHeight
            );

            threepp::Vector2 screenMouse(
                (m_mouse.x * 0.5f + 0.5f) * rendererWidth,
                (1.0f - (m_mouse.y * 0.5f + 0.5f)) * rendererHeight
            );

            float pixelDistance = screenPoint.distanceTo(screenMouse);

            if (pixelDistance <= pointSize_px / 2.0f) {
                if (ndcPoint.z >= -1.0f && ndcPoint.z <= 1.0f) {
                    threepp::Intersection intersection;
                    intersection.distance = raycaster.ray.origin.distanceTo(point_world);
                    intersection.object = this;
                    intersection.point = point_world;
                    intersection.index = static_cast<int>(i);
                    intersects.push_back(intersection);
                }
            }
        }
    }

    // You need a way to set the mouse position, as the raycaster does not store it.
    void setMousePosition(float x, float y) {
        m_mouse.set(x, y);
    }

protected:
    // Protected constructor to enforce use of the static create method
    CustomPoints(const std::shared_ptr<threepp::BufferGeometry>& geometry,
                 const std::shared_ptr<threepp::Material>& material)
        : threepp::Points(geometry, material) {}

private:
    threepp::Vector2 m_mouse;
};


// =============================================================
// Helper Functions (outside the class)
// =============================================================

// Example height function
float f(float x, float y, float tx = 0.0f)
{
    float dx = x - 0.5f;
    float dy = y - 0.5f;
    float r = std::sqrt(dx * dx + dy * dy);
    return std::sin((r + tx) * 8.0f * 3.1415926f) * 0.5f;
}

// Generate grid like your old code
void generate_grid(int N, std::vector<threepp::Vector3>& vertices, std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    for (int j = 0; j <= N; ++j)
    {
        for (int i = 0; i <= N; ++i)
        {
            float x = (float)i / (float)N;
            float y = (float)j / (float)N;
            float z = f(x, y);
            vertices.push_back({x, y, z});
        }
    }

    for (int j = 0; j < N; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            int row1 = j * (N + 1);
            int row2 = (j + 1) * (N + 1);

            // triangle 1
            indices.push_back(row1 + i);
            indices.push_back(row1 + i + 1);
            indices.push_back(row2 + i + 1);

            // triangle 2
            indices.push_back(row1 + i);
            indices.push_back(row2 + i + 1);
            indices.push_back(row2 + i);
        }
    }
}


// Set this to 0 to use the simplified ShaderMaterial
#define TEST 0

#if not TEST

class SurfaceRenderer
{
public:
    SurfaceRenderer()
    {
        m_Geometry = std::make_shared<threepp::BufferGeometry>();
        m_Material = threepp::RawShaderMaterial::create();
        m_Material->side = threepp::Side::Double;

        m_Material->lights = false;     // no lighting
        m_Material->transparent = false;
        m_Material->depthTest = true;
        m_Material->depthWrite = true;



//        m_Material->lights = false;       // don’t inject lighting uniforms/chunks
//        m_Material->fog = false;          // don’t inject fog
//        m_Material->toneMapped = false;   // disable tone mapping
//        m_Material->depthTest = true;     // (optional, control manually)
//        m_Material->depthWrite = true;    // (optional, control manually)
//        m_Material->transparent = false;  // (avoid blending)


        // Use the simplified shaders below
        m_Material->vertexShader = m_VertexShader;
        m_Material->fragmentShader = m_FragmentShader;

        // Correctly initialize uniforms required by the shaders
        m_Material->uniforms = {
            {"ZL", threepp::Uniform(-0.5f)}, // Example default value
            {"ZH", threepp::Uniform(0.5f)}  // Example default value
        };

        m_Mesh = std::make_shared<threepp::Mesh>(m_Geometry, m_Material);
        m_Mesh->frustumCulled = false;
    }

    void SetData(const std::vector<threepp::Vector3>& vertices,
                 const std::vector<unsigned int>& indices)
    {
        std::vector<float> verticesData;
        verticesData.reserve(vertices.size() * 3);
        for (const auto& v : vertices)
        {
            verticesData.push_back(v.x);
            verticesData.push_back(v.y);
            verticesData.push_back(v.z);
        }

        auto positionsUnique = threepp::TypedBufferAttribute<float>::create(verticesData, 3);
        m_Geometry->setAttribute("position", std::move(positionsUnique));
        m_Geometry->setIndex(indices);
        m_Geometry->computeVertexNormals();
        m_Geometry->computeBoundingSphere();
        m_Geometry->computeBoundingBox();
    }

    void SetZRange(float zl, float zh)
    {
        if(m_Material)
        {
            m_Material->uniforms["ZL"] = threepp::Uniform(zl);
            m_Material->uniforms["ZH"] = threepp::Uniform(zh);
        }
    }

    std::shared_ptr<threepp::Mesh> GetMesh()
    {
        return m_Mesh;
    }

private:
    std::shared_ptr<threepp::BufferGeometry> m_Geometry;
    std::shared_ptr<threepp::ShaderMaterial> m_Material;
    std::shared_ptr<threepp::Mesh> m_Mesh;

//    // Simplified Vertex Shader
//    const std::string m_VertexShader = R"(#version 330 core
//        layout(location = 0) in vec3 position;
//        uniform mat4 projectionMatrix;
//        uniform mat4 modelViewMatrix;
//
//        void main() {
//            gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
//        }
//    )";
//
//    // Simplified Fragment Shader that outputs a solid color
//    const std::string m_FragmentShader = R"(#version 330 core
//        out vec4 color;
//
//        void main() {
//            color = vec4(1.0, 0.0, 0.0, 1.0); // Solid red color
//        }
//    )";



//    const std::string m_VertexShader = R"(#version 330 core
//layout(location = 0) in vec3 position;
//uniform mat4 projectionMatrix;
//uniform mat4 modelViewMatrix;
//
//out vec3 pos;
//
//void main()
//{
//    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
//    pos = position;
//})";







//    const std::string m_FragmentShader = R"(#version 330 core
//uniform float ZL;
//uniform float ZH;
//
//in vec3 pos;
//out vec4 color;
//
//vec3 jet(float t)
//{
//    return clamp(vec3(1.5) - abs(4.0 * vec3(t) + vec3(-3, -2, -1)),
//                 vec3(0), vec3(1));
//}
//
//void main()
//{
//    float param = (pos.z - ZL) / (ZH - ZL);
//    vec3 c = jet(param);
//    color = vec4(c, 1.0);
//
//})";



// Use a multi-line const char* for reliable compilation
// Corrected Vertex Shader: No #version at the top
const std::string m_VertexShader = R"(
#version 330 core
layout(location = 0) in vec3 position;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

out vec3 pos;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    pos = position;
}
)";

const std::string m_FragmentShader = R"(
#version 330 core
uniform float ZL;
uniform float ZH;

in vec3 pos;
out vec4 color;

vec3 jet(float t)
{
    return clamp(vec3(1.5) - abs(4.0 * vec3(t) + vec3(-3, -2, -1)),
                 vec3(0), vec3(1));
}

void main()
{
    float param = (pos.z - ZL) / (ZH - ZL);
    vec3 c = jet(param);
    color = vec4(c, 1.0);
}
)";




};


#else


// =============================================================
// TEST MODE: MeshBasicMaterial-based Renderer
// =============================================================
class SurfaceRenderer
{
public:

    SurfaceRenderer()
    {
        m_Geometry = std::make_shared<threepp::BufferGeometry>();
        m_Material = threepp::MeshBasicMaterial::create();
        m_Material->color = threepp::Color::red;
        m_Mesh = std::make_shared<threepp::Mesh>(m_Geometry, m_Material);
    }

    void SetData(const std::vector<threepp::Vector3>& vertices,
                 const std::vector<unsigned int>& indices)
    {
        std::vector<float> verticesData;
        verticesData.reserve(vertices.size() * 3);
        for (auto& v : vertices)
        {
            verticesData.push_back(v.x);
            verticesData.push_back(v.y);
            verticesData.push_back(v.z);
        }

        auto positionsUnique = threepp::TypedBufferAttribute<float>::create(verticesData, 3);
        m_Geometry->setAttribute("position", std::move(positionsUnique));
        m_Geometry->setIndex(indices);
        m_Geometry->computeVertexNormals();
        m_Geometry->computeBoundingSphere();
        m_Geometry->computeBoundingBox();
    }

    void SetZRange(float /*zl*/, float /*zh*/)
    {
        // ignored for testing
    }

    std::shared_ptr<threepp::Mesh> GetMesh()
    {
        return m_Mesh;
    }

private:

    std::shared_ptr<threepp::BufferGeometry> m_Geometry;
    std::shared_ptr<threepp::MeshBasicMaterial> m_Material;
    std::shared_ptr<threepp::Mesh> m_Mesh;
    const std::string m_VertexShader = "";
    const std::string m_FragmentShader = "";
};


#endif



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

    void OnButtonAddPointClicked(wxCommandEvent& event);

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

    std::shared_ptr<Sprite> CreateAnimalSprite(const std::string& texturePath);

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

    std::shared_ptr<Scene> GetSceneObject()
    {
        return scene;
    }

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


    private:
    // For object picking
    Raycaster raycaster;
    Vector2 mouse{-Infinity<float>, -Infinity<float>}; // Normalized device coords
    std::shared_ptr<Points> m_SelectionMarkerPointCircle; // e.g. a small sphere to show hit point

    std::shared_ptr<threepp::Text2D> m_SelectionMarkerTextLabel;

    std::shared_ptr<CustomPoints> m_TrackPoints; // Your custom object

    std::shared_ptr<SurfaceRenderer> surface;  // <-- keep it alive

    std::shared_ptr<Sprite> m_Sprite[4]; // keep it alive

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

/**
 * @brief Creates and configures a shared_ptr to a Sprite object.
 *
 * This function loads a texture, creates a material, and then constructs a sprite.
 * It includes fixes for the compile errors encountered.
 *
 * @return std::shared_ptr<Sprite> A shared pointer to the created sprite.
 */
std::shared_ptr<Sprite> OpenGLCanvas::CreateAnimalSprite(const std::string& texturePath) {

        // Load texture
        TextureLoader loader;
        auto texture = loader.load(texturePath);

        texture->needsUpdate();  // Mark texture for update

        // Create sprite material
        auto material = SpriteMaterial::create();
        material->map = texture;
        material->transparent = true;
        material->map->offset.set(0.5, 0.5);
        material->blending = Blending::Normal;  // Use correct enum value

        // Create sprite
        auto sprite = Sprite::create(material);

        // Set scale based on image aspect ratio (replace with actual ratio)
        float aspect = 1.0f;  // width/height ratio of your image
        float desiredSize = 0.01f;
        sprite->scale.set(0.5f * aspect * desiredSize, 0.5f * desiredSize, 1.0f);

        // Position the sprite
        sprite->position.set(0, 0, 0);

        return sprite;
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

    auto bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    auto addPointButton = new wxButton(this, wxID_ANY, "Add new points to line");

    bottomSizer->Add(addPointButton, 0, wxALL | wxALIGN_CENTER, FromDIP(15));
    bottomSizer->AddStretchSpacer(1);

    sizer->Add(bottomSizer, 0, wxEXPAND);

    SetSizerAndFit(sizer);

    // Bind button click events to event handlers
    addPointButton->Bind(wxEVT_BUTTON, &MyFrame::OnButtonAddPointClicked, this);
}

#include <random>

void MyFrame::OnButtonAddPointClicked(wxCommandEvent& event)
{
    auto scene = openGLCanvas->GetSceneObject();
    auto line = scene->getObjectByName("line3d");
    std::shared_ptr<BufferGeometry> geometry = line->geometry();
    const auto position = geometry->getAttribute<float>("position");

    auto& array = position->array();

    // Use a random device to seed the random number generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // Define a distribution to generate floating-point numbers in a range
    // Adjust the range to control how far the new points can be from the last one.
    const float range = 10.0f; // Example: new points will be within 10 units of the last one
    std::uniform_real_distribution<float> distrib(-range, range);

    // Get the coordinates of the last point
    // We need to check if the array has points before we can read from it
    float lastX = 0.0f;
    float lastY = 0.0f;
    float lastZ = 0.0f;

    if (!array.empty()) {
        const size_t numPoints = array.size() / 3;
        const size_t lastPointIndex = (numPoints - 1) * 3;
        lastX = array[lastPointIndex];
        lastY = array[lastPointIndex + 1];
        lastZ = array[lastPointIndex + 2];
    }

    // Calculate new point coordinates by adding random offsets to the last point
    float newX = lastX + distrib(gen);
    float newY = lastY + distrib(gen);
    float newZ = lastZ + distrib(gen);

    // Add the new random point to the array
    array.push_back(newX);
    array.push_back(newY);
    array.push_back(newZ);

    // Update the geometry
    geometry->setAttribute("position", threepp::FloatBufferAttribute::create(array, 3));
    position->needsUpdate();
    geometry->computeBoundingSphere();
    Refresh(false);
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
    if(!openGLContext)
    {
        return false;
    }

    SetCurrent(*openGLContext);

    if(!InitializeOpenGLFunctions())
    {
        wxMessageBox("Error: Could not initialize OpenGL function pointers.",
                     "OpenGL initialization error", wxOK | wxICON_INFORMATION, this);
        return false;
    }

    wxLogDebug("OpenGL version: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    wxLogDebug("OpenGL vendor: %s", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

    glEnable(GL_PROGRAM_POINT_SIZE);

    auto viewPortSize = GetSize() * GetContentScaleFactor();
    WindowSize size{viewPortSize.x, viewPortSize.y};
    renderer = std::make_shared<GLRenderer>(size);

    renderer->checkShaderErrors = true;
    renderer->autoClear = false;
    renderer->outputEncoding = threepp::Encoding::Linear;

    scene = Scene::create();
    scene->background = Color::aliceblue;
    scene->name = "scene";

    camera = PerspectiveCamera::create(75, size.aspect(), 0.1f, 1000);
    camera->position.z = 5;
    camera->name = "mainCamera";

    controls = std::make_shared<OrbitControls>(*camera, *this);

#if 0
    auto box = createBox();
    box->name = "box";
    scene->add(box);

    auto sphere = createSphere();
    sphere->name = "sphereInsideBox";
    box->add(sphere);

    auto plane = createPlane();
    plane->name = "plane";
    auto planeMaterial = plane->material()->as<MeshBasicMaterial>();
    scene->add(plane);
#endif // 0

    WindowSize s = this->size();

    hud = std::make_shared<HUD>(s);

    font1 = fontLoader.defaultFont();
    font2 = *fontLoader.load("data/fonts/helvetiker_regular.typeface.json");

    opts1 = std::make_shared<TextGeometry::Options>(font1, 40);
    hudText1 = std::make_shared<Text2D>(*opts1, "Hello World!");
    hudText1->setColor(Color::black);
    hudText1->name = "hudText1";
    hud->add(*hudText1, HUD::Options());

    opts2 = std::make_shared<TextGeometry::Options>(font2, 10, 1);
    hudText2 = std::make_shared<Text2D>(*opts1, "");
    hudText2->setColor(Color::red);
    hudText2->name = "hudText2";
    hud->add(*hudText2, HUD::Options()
             .setNormalizedPosition({1, 1})
             .setHorizontalAlignment(threepp::HUD::HorizontalAlignment::RIGHT)
             .setVerticalAlignment(threepp::HUD::VerticalAlignment::TOP));

    hudText2->setText("Delta=1.23456789", *opts2);
    hud->needsUpdate(*hudText2);

    m_Sprite[0] = CreateAnimalSprite("bird.png");
    m_Sprite[1] = CreateAnimalSprite("dog.png");
    scene->add(m_Sprite[0]);
    scene->add(m_Sprite[1]);


    {
#if 1

    // billboard text labels
    float textSize = 0.02;
    std::string displayText = "threepp!";

    const auto textLabelMaterial = SpriteMaterial::create();
    textLabelMaterial->side = Side::Double;
    textLabelMaterial->color = Color::green;
    textLabelMaterial->sizeAttenuation = false;

    textMesh2dArray.push_back(Text2D::create(TextGeometry::Options(font2, textSize), displayText, textLabelMaterial));
    textMesh2dArray.push_back(Text2D::create(TextGeometry::Options(font2, textSize), displayText, textLabelMaterial));

    textMesh2dArray[0]->position.z = 5;
    textMesh2dArray[1]->position.z = -5;

    textMesh2dArray[0]->geometry()->center();
    textMesh2dArray[1]->geometry()->center();

    textMesh2dArray[0]->name = "textLabelFront";
    textMesh2dArray[1]->name = "textLabelBack";

    scene->add(*(textMesh2dArray[0]));
    scene->add(*(textMesh2dArray[1]));

#endif // 1
    }


#if 0
    // add 3D lines
    auto lineMaterial = threepp::LineBasicMaterial::create();
    lineMaterial->color.setRGB(1, 0, 0);

    auto lineGeometry = threepp::BufferGeometry::create();
    std::vector<float> lineVertices =
    {
        -1, 0, 0,
            1, 0, 0,
            0, -1, 0,
            0, 1, 0
        };
    lineGeometry->setAttribute("position", threepp::FloatBufferAttribute::create(lineVertices, 3));

    auto line = threepp::LineSegments::create(lineGeometry, lineMaterial);
    line->name = "crossLines";
    scene->add(line);


    auto longLineGeometry = threepp::BufferGeometry::create();
    std::vector<float> longLineVertices =
    {
        0, 0, 0,
        1, 1, 2,
        2, 3, 4,
        6, 8, 9
    };
    longLineGeometry->setAttribute("position", threepp::FloatBufferAttribute::create(longLineVertices, 3));

    auto line2 = threepp::Line::create(longLineGeometry, lineMaterial);
    line2->name = "longLine";
    scene->add(line2);

#endif // 0

// --- NEW: Create a 3D coordinate system with ticks and labels for all axes ---
    {
        // Line material for the main axes
        auto xAxisMaterial = threepp::LineBasicMaterial::create();
        xAxisMaterial->color = threepp::Color::red; // Make the x-axis red
        xAxisMaterial->linewidth = 2;

        auto yAxisMaterial = threepp::LineBasicMaterial::create();
        yAxisMaterial->color = threepp::Color::green; // Make the y-axis green
        yAxisMaterial->linewidth = 2;

        auto zAxisMaterial = threepp::LineBasicMaterial::create();
        zAxisMaterial->color = threepp::Color::blue; // Make the z-axis blue
        zAxisMaterial->linewidth = 2;

        // Axis lines from -5 to 5
        auto xAxisGeometry = threepp::BufferGeometry::create();
        xAxisGeometry->setAttribute("position", threepp::FloatBufferAttribute::create({-5, 0, 0, 5, 0, 0}, 3));
        auto xAxis = threepp::LineSegments::create(xAxisGeometry, xAxisMaterial);
        xAxis->name = "xAxis";
        scene->add(xAxis);

        auto yAxisGeometry = threepp::BufferGeometry::create();
        yAxisGeometry->setAttribute("position", threepp::FloatBufferAttribute::create({0, -5, 0, 0, 5, 0}, 3));
        auto yAxis = threepp::LineSegments::create(yAxisGeometry, yAxisMaterial);
        yAxis->name = "yAxis";
        scene->add(yAxis);

        auto zAxisGeometry = threepp::BufferGeometry::create();
        zAxisGeometry->setAttribute("position", threepp::FloatBufferAttribute::create({0, 0, -5, 0, 0, 5}, 3));
        auto zAxis = threepp::LineSegments::create(zAxisGeometry, zAxisMaterial);
        zAxis->name = "zAxis";
        scene->add(zAxis);

        // Ticks and labels
        auto tickMaterial = threepp::LineBasicMaterial::create();
        tickMaterial->color = threepp::Color::gray;

        float tickLength = 0.2f;
        float labelOffset = 0.3f;
        float textSize = 0.02;

        const auto textLabelMaterial = SpriteMaterial::create();
        textLabelMaterial->side = Side::Double;
        textLabelMaterial->color = Color::green;
        textLabelMaterial->sizeAttenuation = false;

        // X-axis ticks and labels
        for(float i = -5; i <= 5; i += 1.0f)
        {
            auto xTickGeometry = threepp::BufferGeometry::create();
            xTickGeometry->setAttribute("position", threepp::FloatBufferAttribute::create({i, -tickLength, 0, i, tickLength, 0}, 3));
            auto xTick = threepp::LineSegments::create(xTickGeometry, tickMaterial);
            xTick->name = "xTick_" + std::to_string(static_cast<int>(i));
            scene->add(xTick);

            auto xLabel = threepp::Text2D::create(TextGeometry::Options(font2, textSize),
                                                     std::to_string(static_cast<int>(i)), textLabelMaterial);
            xLabel->position = {i, -labelOffset, 0};
            xLabel->name = "xLabel_" + std::to_string(static_cast<int>(i));
            scene->add(xLabel);
        }

        // Y-axis ticks and labels
        for(float i = -5; i <= 5; i += 1.0f)
        {
            auto yTickGeometry = threepp::BufferGeometry::create();
            yTickGeometry->setAttribute("position", threepp::FloatBufferAttribute::create({-tickLength, i, 0, tickLength, i, 0}, 3));
            auto yTick = threepp::LineSegments::create(yTickGeometry, tickMaterial);
            yTick->name = "yTick_" + std::to_string(static_cast<int>(i));
            scene->add(yTick);

            auto yLabel = threepp::Text2D::create(TextGeometry::Options(font2, textSize),
                                                     std::to_string(static_cast<int>(i)), textLabelMaterial);
            yLabel->position = {-labelOffset, i, 0};
            yLabel->name = "yLabel_" + std::to_string(static_cast<int>(i));
            scene->add(yLabel);
        }

        // Z-axis ticks and labels
        for(float i = -5; i <= 5; i += 1.0f)
        {
            auto zTickGeometry = threepp::BufferGeometry::create();
            zTickGeometry->setAttribute("position", threepp::FloatBufferAttribute::create({0, -tickLength, i, 0, tickLength, i}, 3));
            auto zTick = threepp::LineSegments::create(zTickGeometry, tickMaterial);
            zTick->name = "zTick_" + std::to_string(static_cast<int>(i));
            scene->add(zTick);

            auto zLabel = threepp::Text2D::create(TextGeometry::Options(font2, textSize),
                                                     std::to_string(static_cast<int>(i)), textLabelMaterial);
            zLabel->position = {0, -labelOffset, i};
            zLabel->name = "zLabel_" + std::to_string(static_cast<int>(i));
            scene->add(zLabel);
        }
    }




// ---- sample data: 5 points with RawShaderMaterial ----
    {
//    std::vector<float> vertices = {
//        0.0f, 0.0f, 0.0f,
//        1.0f, 0.0f, 0.0f,
//        0.0f, 1.0f, 0.0f,
//        0.0f, 0.0f, 1.0f,
//       -1.0f,-1.0f, 0.0f
//    };


        std::vector<float> vertices =
        {
            // NEW: A 3D spiral-like path with more points
            -0.0f, 0.0f, 0.0f,
            0.5f, 0.0f, 0.5f,
            0.8f, 0.5f, 1.0f,
            0.7f, 1.0f, 1.5f,
            0.0f, 1.2f, 2.0f,
            -0.7f, 1.0f, 2.5f,
            -0.8f, 0.5f, 3.0f,
            -0.5f, 0.0f, 3.5f,
            -0.0f, -0.5f, 4.0f,
            0.5f, -0.7f, 4.5f
        };


        std::vector<float> colors =
        {
            // Matching the number of vertices
            1, 0, 0, 1,
            0, 1, 0, 1,
            0, 0, 1, 1,
            1, 1, 0, 1,
            1, 0, 1, 1,
            1, 0, 0, 1,
            0, 1, 0, 1,
            0, 0, 1, 1,
            1, 1, 0, 1,
            1, 0, 1, 1
        };

        auto geometry = BufferGeometry::create();
        geometry->setAttribute("position", FloatBufferAttribute::create(vertices, 3));
        geometry->setAttribute("color", FloatBufferAttribute::create(colors, 4));

        auto material = RawShaderMaterial::create();
        material->vertexShader = R"(
        #version 330 core
        #define attribute in
        #define varying out
        uniform mat4 modelViewMatrix;
        uniform mat4 projectionMatrix;
        uniform float pointSize;  // uniform to control circle radius
        attribute vec3 position;
        attribute vec4 color;
        varying vec4 vColor;
        void main() {
            vColor = color;
            gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
            gl_PointSize = pointSize;   // use uniform instead of fixed value
        }
    )";

        material->fragmentShader = R"(
        #version 330 core
        #define varying in
        out vec4 pc_fragColor;
        #define gl_FragColor pc_fragColor
        varying vec4 vColor;
        void main() {
            vec2 coord = 2.0 * gl_PointCoord - 1.0;
            if(dot(coord, coord) > 1.0) discard;
            gl_FragColor = vColor;
        }
    )";

        material->side = Side::Double;
        material->transparent = false;

        // set initial point size
        material->uniforms["pointSize"] = threepp::Uniform(10.0f);  // programmer can modify this

        //auto points = Points::create(geometry, material);

        // --- create our CustomPoints object ---
        m_TrackPoints = CustomPoints::create(geometry, material);

        scene->add(m_TrackPoints);

        // --- NEW: Add lines to connect the points ---
        // Create a new geometry for the lines. We can reuse the same vertices.
        auto lineGeometry = BufferGeometry::create();
        lineGeometry->setAttribute("position", FloatBufferAttribute::create(vertices, 3));

        // Create a basic material for the lines
        auto lineMaterial = threepp::LineBasicMaterial::create();
        lineMaterial->color = threepp::Color::red; // A clear color to stand out
        lineMaterial->transparent = false;

        // Create the line object from the geometry and material
        // The Line class will draw a segment between each consecutive pair of vertices.
        auto line = threepp::Line::create(lineGeometry, lineMaterial);

        // Add the line to the scene
        scene->add(line);



    }


//    float sphereRadius = 0.1f;
//    // auto sphereGeometry = SphereGeometry::create(sphereRadius);
//    auto sphereGeometry = threepp::BoxGeometry::create(1.0f, 1.0f, 1.0f);
//    auto sphereMaterial = MeshBasicMaterial::create();
//    sphereMaterial->wireframe = true;
//    sphereMaterial->color = Color::red;
//
//
//    selectionMarker = Mesh::create(sphereGeometry, sphereMaterial);
//    selectionMarker->name = "selectionMarker";
//    selectionMarker->visible = false;
//    scene->add(selectionMarker);


// --- Create a geometry for a single point ---
    std::vector<float> vertices = { 0.0f, 0.0f, 0.0f };
    auto markerGeometry = threepp::BufferGeometry::create();
    markerGeometry->setAttribute("position", threepp::FloatBufferAttribute::create(vertices, 3));

// --- Create a material for the marker
// You can reuse your existing RawShaderMaterial and just change its uniforms
// It's probably best to create a new instance to avoid affecting your main point cloud
    auto markerMaterial = threepp::RawShaderMaterial::create();
    markerMaterial->vertexShader = R"(
    #version 330 core
    #define attribute in
    #define varying out
    uniform mat4 modelViewMatrix;
    uniform mat4 projectionMatrix;
    uniform float pointSize;
    attribute vec3 position;
    void main() {
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
        gl_PointSize = pointSize;
    }
)";
    markerMaterial->fragmentShader = R"(
    #version 330 core
    out vec4 pc_fragColor;
    #define gl_FragColor pc_fragColor
    uniform vec4 markerColor;
    uniform float ringThickness; // New uniform to control the ring thickness
    void main() {
        vec2 coord = 2.0 * gl_PointCoord - 1.0;
        float dist = dot(coord, coord);
        float outerRadius = 1.0; // The outer edge of the point
        float innerRadius = outerRadius - ringThickness;

        // Discard fragments outside the outer radius
        if (dist > outerRadius * outerRadius) discard;

        // Discard fragments inside the inner radius
        if (dist < innerRadius * innerRadius) discard;

        gl_FragColor = markerColor;
    }
)";
    markerMaterial->uniforms["pointSize"] = threepp::Uniform(20.0f); // Make it a bit larger
    // Explicitly create a 4-component vector for the uniform
    markerMaterial->uniforms["markerColor"] = threepp::Uniform(threepp::Vector4(0.0f, 0.0f, 0.0f, 1.0f)); // black with full opacity
    markerMaterial->uniforms["ringThickness"] = threepp::Uniform(0.2f); // Adjust the thickness (0.0 to 1.0)

    // --- Create the marker as a Points object ---
    m_SelectionMarkerPointCircle = threepp::Points::create(markerGeometry, markerMaterial);
    m_SelectionMarkerPointCircle->visible = false;
    scene->add(m_SelectionMarkerPointCircle);


    // --- NEW: Create the dynamic text label
    // Create the dynamic text label
    const auto textLabelMaterial = SpriteMaterial::create();
    textLabelMaterial->side = Side::Double;
    textLabelMaterial->color = Color::black; // Match the marker color
    textLabelMaterial->sizeAttenuation = false;

    // Disable depth testing and writing for the material (this is correct)
    textLabelMaterial->depthTest = false;
    textLabelMaterial->depthWrite = false;


    m_SelectionMarkerTextLabel = Text2D::create(TextGeometry::Options(font2, 0.02f), "Ready", textLabelMaterial);
    m_SelectionMarkerTextLabel->position.set(0, 0.2f, 0);
    m_SelectionMarkerTextLabel->visible = false;

    // --- CORRECT: Set renderOrder on the object itself, not the material
    m_SelectionMarkerTextLabel->renderOrder = 999;

    // Make the label a child of the marker so it moves with it
    // selectionMarker->add(*textLabel);
    scene->add(*m_SelectionMarkerTextLabel); // Add the label to the scene as a separate object





        // Create and keep the instance alive
        surface = std::make_shared<SurfaceRenderer>();

        std::vector<threepp::Vector3> vertices1;
        std::vector<unsigned int> indices1;
        generate_grid(50, vertices1, indices1);

        surface->SetData(vertices1, indices1);

        float zl = +std::numeric_limits<float>::max();
        float zh = -std::numeric_limits<float>::max();
        for (auto &v: vertices1)
        {
            if(v.z < zl) zl = v.z;
            if(v.z > zh) zh = v.z;
        }
        surface->SetZRange(zl, zh);

        scene->add(surface->GetMesh());















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


    wxSize sz = GetSize();
    int x = event.GetX();
    int y = event.GetY();

    mouse.x = (static_cast<float>(x) / sz.GetWidth()) * 2.f - 1.f;
    mouse.y = -(static_cast<float>(y) / sz.GetHeight()) * 2.f + 1.f;

    event.Skip();
}

void OpenGLCanvas::OnMousePress(wxMouseEvent& event)
{

    int buttonFlag = event.GetButton();
    wxPoint pos = event.GetPosition();
    int button = 0;
    if(wxMOUSE_BTN_LEFT == buttonFlag)
        button = 0;
    else if(wxMOUSE_BTN_RIGHT == buttonFlag)
        button = 1;

    Vector2 p{static_cast<float>(pos.x), static_cast<float>(pos.y)};
    onMousePressedEvent(button, p, PeripheralsEventSource::MouseAction::PRESS);
    Refresh(false);

     // Convert mouse coordinates to normalized device coordinates (-1..1)
    int mouseX = event.GetX();
    int mouseY = event.GetY();
    int w, h;
    GetSize(&w, &h);

    threepp::Vector2 ndcMouse(
        (2.0f * mouseX) / static_cast<float>(w) - 1.0f,
        -((2.0f * mouseY) / static_cast<float>(h) - 1.0f)
    );

    // --- NEW: Feed the custom points object the mouse position
    // This is the one line that enables your custom raycasting logic to work
    if (auto customPoints = std::dynamic_pointer_cast<CustomPoints>(m_TrackPoints)) {
        customPoints->setMousePosition(ndcMouse.x, ndcMouse.y);
    }

    // Setup raycaster from camera
    raycaster.setFromCamera(ndcMouse, *camera);

    m_SelectionMarkerPointCircle->visible = false;

    // --- OPTIMIZATION: Raycast only against the custom points object
    // Create a vector of RAW pointers to pass to the raycaster
    std::vector<threepp::Object3D*> objectsToRaycast = { m_TrackPoints.get() };

    // Pass the vector of raw pointers to intersectObjects
    auto intersects = raycaster.intersectObjects(objectsToRaycast, true);

    if(!intersects.empty())
    {
        // The rest of your code remains the same as it correctly filters from the `intersects` vector.
        const threepp::Intersection* firstValidIntersect = &intersects.front();

        if(firstValidIntersect != nullptr)
        {
            const auto& intersect = *firstValidIntersect;

            // Move selection marker
            m_SelectionMarkerPointCircle->position.copy(intersect.point);
            m_SelectionMarkerPointCircle->visible = true;


            // Get the coordinates of the selected point and add color
            std::stringstream ss;
            ss << "x:" << std::fixed << std::setprecision(2) << intersect.point.x;
            ss << "\ny:" << std::fixed << std::setprecision(2) << intersect.point.y;
            ss << "\nz:" << std::fixed << std::setprecision(2) << intersect.point.z;

            int pointIndex = 0;

            // --- NEW: Retrieve and add the color of the selected point
            if(auto points = dynamic_cast<threepp::Points*>(intersect.object))
            {
                if(intersect.index.has_value())
                {
                    int idx = intersect.index.value();
                    pointIndex = idx;
                    if(auto* colAttr = dynamic_cast<threepp::FloatBufferAttribute*>(points->geometry()->getAttribute("color")))
                    {
                        float r = colAttr->getX(idx);
                        float g = colAttr->getY(idx);
                        float b = colAttr->getZ(idx);
                        float a = colAttr->getW(idx); // Your attribute has 4 components

                        ss << "\nColor: (" << std::fixed << std::setprecision(2) << r << ", "
                           << std::fixed << std::setprecision(2) << g << ", "
                           << std::fixed << std::setprecision(2) << b << ", "
                           << std::fixed << std::setprecision(2) << a << ")";
                    }
                }
            }


            std::string text = ss.str();

            // --- Update the label's text
            m_SelectionMarkerTextLabel->setText(text); // This is an assumed method. Check your threepp docs for Text2D
            m_SelectionMarkerTextLabel->visible = true; // Make the label visible

            // --- NEW: Calculate the label's position with a fixed PIXEL offset ---

            // Get the dimensions of your canvas in pixels
            int w, h;
            GetSize(&w, &h);

                // 1. Create a Vector3 from the intersection point
                threepp::Vector3 projectedPoint = intersects.front().point;

                // 2. Project the 3D point to Normalized Device Coordinates (NDC)
                projectedPoint.project(*camera);

                // 3. Convert NDC to pixel coordinates and add the desired pixel offset
                // NOTE: positive X is right, negative Y is down
                float labelPixelOffsetX = 20.0f; // Adjust these pixel values as needed
                float labelPixelOffsetY = -20.0f;

                float labelPixelX = ((projectedPoint.x + 1.0f) * 0.5f) * static_cast<float>(w) + labelPixelOffsetX;
                float labelPixelY = ((projectedPoint.y + 1.0f) * 0.5f) * static_cast<float>(h) + labelPixelOffsetY;

                // 4. Convert back to NDC coordinates for the text label
                threepp::Vector3 unprojectedLabelPoint;
                unprojectedLabelPoint.x = (labelPixelX / static_cast<float>(w)) * 2.0f - 1.0f;
                unprojectedLabelPoint.y = (labelPixelY / static_cast<float>(h)) * 2.0f - 1.0f;
                unprojectedLabelPoint.z = -0.9f; // Keep a constant z to maintain a fixed size and visibility

                // 5. Unproject the new NDC vector to get its 3D world position
                unprojectedLabelPoint.unproject(*camera);

                // 6. Set the label's position to the newly calculated position
                m_SelectionMarkerTextLabel->position.copy(unprojectedLabelPoint);

                // Now, calculate the position for the sprite using a different pixel offset
                float spritePixelOffsetX = 20.0f;
                float spritePixelOffsetY = 20.0f; // A larger negative value moves the sprite higher.

                float spritePixelX = ((projectedPoint.x + 1.0f) * 0.5f) * static_cast<float>(w) + spritePixelOffsetX;
                float spritePixelY = ((projectedPoint.y + 1.0f) * 0.5f) * static_cast<float>(h) + spritePixelOffsetY;

                // Convert back to NDC coordinates for the sprite
                threepp::Vector3 unprojectedSpritePoint;
                unprojectedSpritePoint.x = (spritePixelX / static_cast<float>(w)) * 2.0f - 1.0f;
                unprojectedSpritePoint.y = (spritePixelY / static_cast<float>(h)) * 2.0f - 1.0f;
                unprojectedSpritePoint.z = -0.9f; // Keep a constant z to maintain a fixed size and visibility

                // Unproject the new NDC vector to get its 3D world position
                unprojectedSpritePoint.unproject(*camera);

                // Set the sprite's position to the newly calculated position
                m_Sprite[0]->position.copy(unprojectedSpritePoint);
                m_Sprite[1]->position.copy(unprojectedSpritePoint);

                if (pointIndex % 2 == 0)
                {
                    m_Sprite[0]->visible = true;
                    m_Sprite[1]->visible = false;
                }
                else
                {
                    m_Sprite[0]->visible = false;
                    m_Sprite[1]->visible = true;
                }



            std::cout << "Hit object: " << intersect.object->name;


            if(intersect.index.has_value())
            {
                std::cout << " index: " << intersect.index.value();
            }

            std::cout << " point: " << intersect.point;


            // --- Highlight the clicked object ---
            threepp::Object3D* selectedObject = intersect.object;


            std::cout << "Clicked object: "
                              << (selectedObject->name.empty() ? "<unnamed>" : selectedObject->name)
                              << " (type: " << typeid(*selectedObject).name() << ")"
                              << std::endl;


            if(auto mesh = dynamic_cast<threepp::Mesh*>(selectedObject))
            {
                // Cast to MeshBasicMaterial safely
                if(auto mat = std::dynamic_pointer_cast<threepp::MeshBasicMaterial>(mesh->material()))
                {
                    std::cout << "Mesh clicked. Color: "
                              << mat->color.r << ", "
                              << mat->color.g << ", "
                              << mat->color.b << std::endl;

                    mat->color = threepp::Color::yellow; // highlight on click
                }
            }
            else if(auto points = dynamic_cast<threepp::Points*>(selectedObject))
            {
                // --- FIX: Cast to RawShaderMaterial, not PointsMaterial ---
                if(auto mat = std::dynamic_pointer_cast<threepp::RawShaderMaterial>(points->material()))
                {
                    // The pointSize is a uniform, not a direct material property
                    float size = mat->uniforms.at("pointSize").value<float>();
                    std::cout << "Points clicked. Size: " << size << std::endl;

                    if(intersect.index.has_value())
                    {
                        int idx = intersect.index.value();

                        // --- Get position attribute ---
                        if(auto* posAttr = dynamic_cast<threepp::FloatBufferAttribute*>(points->geometry()->getAttribute("position")))
                        {
                            float x = posAttr->getX(idx);
                            float y = posAttr->getY(idx);
                            float z = posAttr->getZ(idx);

                            std::cout << "Clicked point index: " << idx
                                             << " -> position(" << x << ", " << y << ", " << z << ")";
                        }

                        // --- Get color attribute ---
                        if(auto* colAttr = dynamic_cast<threepp::FloatBufferAttribute*>(points->geometry()->getAttribute("color")))
                        {
                            float r = colAttr->getX(idx);
                            float g = colAttr->getY(idx);
                            float b = colAttr->getZ(idx);
                            float a = colAttr->getW(idx); // Your attribute has 4 components

                            std::cout << "  color(" << r << ", " << g << ", " << b << ", " << a << ")" << std::endl;
                        }

                        std::cout << std::endl;
                    }
                }

                Refresh(true);
            }
        }
    }
    else
    {
        // If no intersection was found, hide the marker and the label
        m_SelectionMarkerPointCircle->visible = false;
        m_SelectionMarkerTextLabel->visible = false;

    }

    event.Skip(); // allow other handlers to run
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
