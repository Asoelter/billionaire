#include "demo.h"

Demo::Demo()
    : window_(sf::VideoMode({800, 600}), "demo")
{
    window_.setVerticalSyncEnabled(true);
}

void Demo::run()
{
    sf::Event event;

    auto blueCircle = sf::CircleShape(50);
    blueCircle.setFillColor(sf::Color(10, 10, 250));
    blueCircle.setPosition(sf::Vector2f(400 - 50, 300 - 50)); // center the circle, otherwise position is based off of top left corner

    auto redCircle = sf::CircleShape(50);
    redCircle.setFillColor(sf::Color(250, 10, 10));
    redCircle.setPosition(sf::Vector2f(400 - 50, 300 - 50)); // center the circle, otherwise position is based off of top left corner

    auto const blueIncrement = 800 / 120; // 120 isn't important, just important that red and blue have
    auto const redIncrement  = 600 / 120; // equal denominators so they appear to move at the same rate (not sure why this doesn't work)
    auto blueDirection = sf::Vector2f(blueIncrement, 0);
    auto redDirection = sf::Vector2f(0, redIncrement);

    while (window_.isOpen()) {
        while (window_.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                {
                    window_.close();
                } break;

                default: break; // skip other events for now
            }
        }

        window_.clear(); // begin frame

        blueCircle.move(blueDirection);
        redCircle.move(redDirection);

        if (blueCircle.getPosition().x > 800 || blueCircle.getPosition().x < 0) {
            blueDirection.x *= -1;
        }

        if (redCircle.getPosition().y > 600 || redCircle.getPosition().y < 0) {
            redDirection.y *= -1;
        }

        window_.draw(blueCircle);
        window_.draw(redCircle);

        window_.display(); // end frame
    }
}
