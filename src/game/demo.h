#ifndef DEMO_H
#define DEMO_H

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

class Demo
{
public:
    Demo();

    void run();

private:
    sf::RenderWindow window_;
};

#endif // DEMO_H
