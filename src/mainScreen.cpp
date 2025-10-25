#include "mainScreen.hh"


void MainScreen::start(Scene::StartReason reason){
    showText();
}

void MainScreen::frame(){


}

void MainScreen::teardown(Scene::TearDownReason reason){

}

void MainScreen::showText(){
    auto& font = m_font;
    
    font.print(gpu(), "Hello", {{.x = 17 * 8, .y = 5 * 16}}, RED);
}