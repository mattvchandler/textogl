#include <iostream>

#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System.hpp>


int main()
{
    sf::Window win(sf::VideoMode(1024, 786), "Textogl Demo", sf::Style::Default, sf::ContextSettings(32, 8, 0, 4, 0));
    win.setKeyRepeatEnabled(false);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    sf::Event ev;

    bool running = true;
    while(running)
    {
        while(win.pollEvent(ev))
        {
            switch(ev.type)
            {
                // close window
                case sf::Event::Closed:
                    running = false;
                    break;

                default:
                    break;
            }
        }
    }

    win.close();

    return EXIT_SUCCESS;
}
