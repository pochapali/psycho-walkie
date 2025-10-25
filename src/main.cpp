#include <stdint.h>

#include <psyqo/advancedpad.hh>
#include <psyqo/application.hh>
#include <psyqo/fixed-point.hh>
#include <psyqo/font.hh>
#include <psyqo/gpu.hh>
#include <psyqo/scene.hh>
#include <psyqo/trigonometry.hh>
#include <psyqo/primitives.hh>


#include "EASTL/algorithm.h"
#include "camera.hh"
#include "navmesh.hh"
#include "psyqo/vector.hh"
#include "renderer.hh"
#include "splashpack.hh"

#include "rand.cpp"


// Data from the splashpack
extern uint8_t _binary_output_bin_start[];

namespace {

using namespace psyqo::fixed_point_literals;
using namespace psyqo::trig_literals;

class PSXSplash final : public psyqo::Application {
    void prepare() override;
    void createScene() override;

  public:
    psxsplash::SplashPackLoader m_loader;

    psyqo::Font<> m_font;

    Rand m_rand;

    psyqo::AdvancedPad m_input;
    static constexpr uint8_t m_stickDeadzone = 0x30;
};

class MainScene final : public psyqo::Scene {
    void frame() override;
    void start(StartReason reason) override;

    psxsplash::Camera m_mainCamera;
    psyqo::Angle camRotX, camRotY, camRotZ;

    psyqo::Trig<> m_trig;
    uint32_t m_lastFrameCounter;

    static constexpr psyqo::FixedPoint<12> moveSpeed = 0.002_fp;
    static constexpr psyqo::Angle rotSpeed = 0.01_pi;

    bool m_sprinting = false;
    static constexpr psyqo::FixedPoint<12> sprintSpeed = 0.01_fp;

    bool m_freecam = false;
    psyqo::FixedPoint<12> pheight = 0.0_fp;

    bool m_renderSelect = false;
};


class MainScreen final : public psyqo::Scene  {
    void start(StartReason reason) override;
    void frame() override;
    void teardown(TearDownReason reason) override;

    void showText();

    bool m_buttonPressed = false;
};

PSXSplash app;
MainScene mainScene;
MainScreen mainScreen;

}  // namespace

void PSXSplash::prepare() {
    psyqo::GPU::Configuration config;
    config.set(psyqo::GPU::Resolution::W320)
        .set(psyqo::GPU::VideoMode::AUTO)
        .set(psyqo::GPU::ColorMode::C15BITS)
        .set(psyqo::GPU::Interlace::PROGRESSIVE);
    gpu().initialize(config);

    // Initialize the Renderer singleton
    psxsplash::Renderer::Init(gpu());
}

void PSXSplash::createScene() {
    m_font.uploadSystemFont(gpu());
    m_input.initialize();
    pushScene(&mainScreen);
    
    //pushScene(&mainScene);
}

void MainScene::start(StartReason reason) {
    app.m_loader.LoadSplashpack(_binary_output_bin_start);
    psxsplash::Renderer::GetInstance().SetCamera(m_mainCamera);

    m_mainCamera.SetPosition(static_cast<psyqo::FixedPoint<12>>(app.m_loader.playerStartPos.x),
                             static_cast<psyqo::FixedPoint<12>>(app.m_loader.playerStartPos.y),
                             static_cast<psyqo::FixedPoint<12>>(app.m_loader.playerStartPos.z));

    pheight = psyqo::FixedPoint<12>(app.m_loader.playerHeight);

    app.m_input.setOnEvent(
        eastl::function<void(psyqo::AdvancedPad::Event)>{[this](const psyqo::AdvancedPad::Event& event) {
            if (event.pad != psyqo::AdvancedPad::Pad::Pad1a) return;
            if (app.m_loader.navmeshes.empty()) return;
            if (event.type == psyqo::AdvancedPad::Event::ButtonPressed) {
                if (event.button == psyqo::AdvancedPad::Button::Triangle) {
                    m_freecam = !m_freecam;
                } else if (event.button == psyqo::AdvancedPad::Button::Square) {
                    m_renderSelect = !m_renderSelect;
                }
            }
        }});

    if (app.m_loader.navmeshes.empty()) {
        m_freecam = true;
    }
}





//main screen shit 
void MainScreen::start(StartReason reason){
    app.m_input.setOnEvent([this](const psyqo::AdvancedPad::Event& event) {
        if (event.type == psyqo::AdvancedPad::Event::ButtonReleased) {
            m_buttonPressed = true;
        }
    });
}

void MainScreen::frame(){

    gpu().clear();
    showText();

    if (m_buttonPressed) {
        m_buttonPressed = false;
        pushScene(&mainScene);
    }
}

void MainScreen::teardown(TearDownReason reason){
    app.m_input.setOnEvent(nullptr);
}

struct MyFragment {
    uint32_t head;
    psyqo::Prim::Rectangle rects[5];
    size_t getActualFragmentSize() const {
        return sizeof(rects) /  sizeof(uint32_t);
    }
};


void MainScreen::showText(){
    auto& font = app.m_font;


    MyFragment fragment;
    fragment.rects[0].position = {{.x = app.m_rand.rand<50>(), .y = app.m_rand.rand<50>()}};
    fragment.rects[0].size = {{.w = 48, .h = 48}};
    fragment.rects[0].setColor({{.r = gpu().getFrameCount() % 255, .g = gpu().getFrameCount() % 255, .b = gpu().getFrameCount() % 255}});
    fragment.rects[1].position = {{.x = 72, .y = gpu().getFrameCount() % app.m_rand.rand<50>()}};
    fragment.rects[1].size = {{.w = gpu().getFrameCount() % app.m_rand.rand<50>(), .h = 96}};
    fragment.rects[1].setColor({{.r = gpu().getFrameCount() % 255, .g = gpu().getFrameCount() % 255, .b = gpu().getFrameCount() % 255}});
    fragment.rects[2].position = {{.x = app.m_rand.rand<200>(), .y = app.m_rand.rand<144>()}};
    fragment.rects[2].size = {{.w = 192, .h = gpu().getFrameCount() % app.m_rand.rand<50>()}};
    fragment.rects[2].setColor({{.r = gpu().getFrameCount() % 255, .g = gpu().getFrameCount() % 255, .b = gpu().getFrameCount() % 255}});
    
    fragment.rects[3].position = {{.x = app.m_rand.rand<50>(), .y = app.m_rand.rand<50>()}};
    fragment.rects[3].size = {{.w = gpu().getFrameCount() % 10, .h = app.m_rand.rand<50>()}};
    fragment.rects[3].setColor({{.r = gpu().getFrameCount() % 255, .g = 124, .b = 79}});
    fragment.rects[4].position = {{.x = app.m_rand.rand<50>(), .y = app.m_rand.rand<50>()}};
    fragment.rects[4].size = {{.w = 96, .h = gpu().getFrameCount() % app.m_rand.rand<50>()}};
    fragment.rects[4].setColor({{.r = gpu().getFrameCount() % 255, .g = gpu().getFrameCount() % 255, .b = gpu().getFrameCount() % 255}});
    gpu().sendFragment(fragment);
    
    font.print(gpu(), "TURBO DRUGS", {{.x = 25, .y = 25}}, {{.r = 0xff, .g = 0x00, .b = 0x00}});
    font.print(gpu(), "press any button", {{.x = 25, .y = 45}}, {{.r = 0xff, .g = 0xff, .b = 0xff}});
}








void MainScene::frame() {
    uint32_t beginFrame = gpu().now();
    auto currentFrameCounter = gpu().getFrameCount();
    auto deltaTime = currentFrameCounter - mainScene.m_lastFrameCounter;

    // Unlike the torus example, this DOES happen...
    if (deltaTime == 0) {
        return;
    }

    mainScene.m_lastFrameCounter = currentFrameCounter;

    uint8_t rightX = app.m_input.getAdc(psyqo::AdvancedPad::Pad::Pad1a, 0);
    uint8_t rightY = app.m_input.getAdc(psyqo::AdvancedPad::Pad::Pad1a, 1);

    uint8_t leftX = app.m_input.getAdc(psyqo::AdvancedPad::Pad::Pad1a, 2);
    uint8_t leftY = app.m_input.getAdc(psyqo::AdvancedPad::Pad::Pad1a, 3);

    int16_t rightXOffset = (int16_t)rightX - 0x80;
    int16_t rightYOffset = (int16_t)rightY - 0x80;
    int16_t leftXOffset = (int16_t)leftX - 0x80;
    int16_t leftYOffset = (int16_t)leftY - 0x80;

    if (__builtin_abs(leftXOffset) < app.m_stickDeadzone &&
        __builtin_abs(leftYOffset) < app.m_stickDeadzone) {
        m_sprinting = false;
    }

    /*
    if (app.m_input.isButtonPressed(psyqo::AdvancedPad::Pad::Pad1a, psyqo::AdvancedPad::Button::L3)) {
        m_sprinting = true;
    }*/

    psyqo::FixedPoint<12> speed = m_sprinting ? sprintSpeed : moveSpeed;

    if (__builtin_abs(rightXOffset) > app.m_stickDeadzone) {
        camRotY += (rightXOffset * rotSpeed * deltaTime) >> 7;
    }
    if (__builtin_abs(rightYOffset) > app.m_stickDeadzone) {
        camRotX -= (rightYOffset * rotSpeed * deltaTime) >> 7;
        camRotX = eastl::clamp(camRotX, -0.5_pi, 0.5_pi);
    }
    m_mainCamera.SetRotation(camRotX, camRotY, camRotZ);

    if (__builtin_abs(leftYOffset) > app.m_stickDeadzone) {
        psyqo::FixedPoint<12> forward = -(leftYOffset * speed * deltaTime) >> 7;
        m_mainCamera.MoveX((m_trig.sin(camRotY) * forward));
        m_mainCamera.MoveZ((m_trig.cos(camRotY) * forward));
    }
    if (__builtin_abs(leftXOffset) > app.m_stickDeadzone) {
        psyqo::FixedPoint<12> strafe = -(leftXOffset * speed * deltaTime) >> 7;
        m_mainCamera.MoveX(-(m_trig.cos(camRotY) * strafe));
        m_mainCamera.MoveZ((m_trig.sin(camRotY) * strafe));
    }

    /*
    if (app.m_input.isButtonPressed(psyqo::AdvancedPad::Pad::Pad1a, psyqo::AdvancedPad::Button::L1)) {
        m_mainCamera.MoveY(speed * deltaTime);
    }
    if (app.m_input.isButtonPressed(psyqo::AdvancedPad::Pad::Pad1a, psyqo::AdvancedPad::Button::R1)) {
        m_mainCamera.MoveY(-speed * deltaTime);
    }*/

    if (!m_freecam) {
        psyqo::Vec3 adjustedPosition =
            psxsplash::ComputeNavmeshPosition(m_mainCamera.GetPosition(), *app.m_loader.navmeshes[0], -pheight);
        m_mainCamera.SetPosition(adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);
    }

    if (!m_renderSelect) {
        psxsplash::Renderer::GetInstance().Render(app.m_loader.gameObjects);
    } else {
        psxsplash::Renderer::GetInstance().RenderNavmeshPreview(*app.m_loader.navmeshes[0], true);
    }
    
    app.m_font.chainprintf(gpu(), {{.x = 2, .y = 2}}, {{.r = 0xff, .g = 0xff, .b = 0xff}}, "FPS: %i",
                                 gpu().getRefreshRate() / deltaTime);
                                 
    //app.m_font.print(gpu(), "Hello", {{.x = 5, .y = 5}}, {{.r = 0xff, .g = 0x00, .b = 0x00}});

    gpu().pumpCallbacks();
    uint32_t endFrame = gpu().now();
    uint32_t spent = endFrame - beginFrame;
}

int main() { return app.run(); }