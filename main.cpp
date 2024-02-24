#include <engine.hpp>
#include <iostream>

int main()
{
    using namespace engine::graphics;
    auto window = Window::make(1280, 720);

    if (!window->good())
    {
        std::cout << "Window error: " << *window->error() << "\n";
    }
    
    while (window->isOpen())
    {
        window->pollEvents();
        window->display();
    }

    Window::destroy(window);
}
