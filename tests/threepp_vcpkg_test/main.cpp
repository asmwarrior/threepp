
#include "threepp/extras/imgui/ImguiContext.hpp"
#include "threepp/threepp.hpp"

using namespace threepp;

namespace {

    auto createBox(const Vector3& pos, const Color& color) {
        auto geometry = BoxGeometry::create();
        auto material = MeshBasicMaterial::create();
        material->color.copy(color);
        auto mesh = Mesh::create(geometry, material);
        mesh->position.copy(pos);

        return mesh;
    }

}// namespace

int main() {

    Canvas canvas("threepp demo", {{"aa", 4}});
    GLRenderer renderer(canvas.size());
    renderer.autoClear = false; // hud

    auto camera = PerspectiveCamera::create();
    camera->position.z = 5;

    OrbitControls controls{*camera, canvas};

    auto scene = Scene::create();
    scene->background = Color::aliceblue;

    auto group = Group::create();
    group->add(createBox({-1, 0, 0}, Color::green));
    group->add(createBox({1, 0, 0}, Color::blue));
    scene->add(group);

    HUD hud;
    auto textHandle = HudText("data/fonts/helvetiker_bold.typeface.json", 4);
    textHandle.setText("Hello World!");
    textHandle.setPosition(1,1);
    textHandle.setVerticalAlignment(HudText::VerticalAlignment::TOP);
    textHandle.setHorizontalAlignment(HudText::HorizontallAlignment::RIGHT);
    hud.addText(textHandle);

    std::array<float, 3> posBuf{};
    ImguiFunctionalContext ui(canvas.windowPtr(), [&] {
        ImGui::SetNextWindowPos({0, 0}, 0, {0, 0});
        ImGui::SetNextWindowSize({230, 0}, 0);

        ImGui::Begin("Demo");
        ImGui::SliderFloat3("position", posBuf.data(), -1.f, 1.f);
        controls.enabled = !ImGui::IsWindowHovered();
        ImGui::End();
    });

    canvas.onWindowResize([&](WindowSize size) {
        camera->aspect = size.aspect();
        camera->updateProjectionMatrix();
        renderer.setSize(size);
    });

    Clock clock;
    float rotationSpeed = 1;
    canvas.animate([&] {
        auto dt = clock.getDelta();
        group->rotation.y += rotationSpeed * dt;

        renderer.clear(); //autoClear is false
        renderer.render(*scene, *camera);
        hud.apply(renderer);

        ui.render();
        group->position.fromArray(posBuf);
    });
}
