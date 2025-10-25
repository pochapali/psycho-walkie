
#pragma once

#include "psyqo/scene.hh"
#include "psyqo/font.hh"


class MainScreen final : public psyqo::Scene  {
    void start(Scene::StartReason reason) override;
    void frame() override;
    void teardown(Scene::TearDownReason reason) override;

    void showText();
}