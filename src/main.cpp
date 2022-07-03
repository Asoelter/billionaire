#include <chrono>
#include <cstdio>
#include <vector>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

struct Vec2
{
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    float x;
    float y;

    Vec2 operator+(Vec2 const & rhs) { return Vec2(x + rhs.x, y + rhs.y); }
    Vec2 operator-(Vec2 const & rhs) { return Vec2(x + rhs.x, y + rhs.y); }

    Vec2& operator-=(Vec2 const & rhs) { x -= rhs.x; y -= rhs.y; return *this; }
};

template <typename Number>
Vec2 operator*(Number n, Vec2 const & v) { return Vec2(v.x * n, v.y * n); }

template <typename Number>
Vec2 operator*(Vec2 const & v, Number n) { return operator*(n, v);}

struct Point2
{
    Point2() = default;
    Point2(float x, float y) : x(x), y(y) {}

    float x;
    float y;

    Point2& operator=(Point2 const & rhs) { x = rhs.x; y = rhs.y; return *this; }

    Point2 operator+(Point2 const & rhs) {return Point2(x + rhs.x, y + rhs.y); }
    Point2 operator-(Point2 const & rhs) {return Point2(x + rhs.x, y + rhs.y); }
};

Point2 operator+(Vec2 const & v, Point2 const & p) {return Point2{v.x + p.x, v.y + p.y};}
Point2 operator+(Point2 const & p, Vec2 const & v) {return operator+(v, p);}

struct Rect
{
    float width;
    float height;
};

struct Map
{
    float floorHeight = 0;
    float width = 0;
    float height = 0;
    sf::Color floorColor;
    sf::Color skyColor;
};

struct Player
{
    Rect   boundingBox;
    Point2 position;
    Vec2   velocity;
    Vec2   acceleration;
};

void drawMap(sf::RenderWindow& window, Map const & map, sf::Transform const & worldTransform);
void drawPlayer(sf::RenderWindow& window, Player const & player, sf::Transform worldTransform);
void applyGravity(std::chrono::microseconds deltaTime, Player & player);
void updatePhysics(std::chrono::microseconds deltaTime, Map const & map, Player & player);

int main()
{
    auto const videoModes = sf::VideoMode::getFullscreenModes();
    assert(!videoModes.empty());

    auto videoMode = sf::VideoMode({800, 600}); // default mode that should never be used
    if (!videoModes.empty()) {
        videoMode = videoModes.front(); // these are sorted in order from best to worst
    }

    auto window = sf::RenderWindow(videoMode, "Cool game");

    // Next we're going to create a matrix to define our world position. By default, SFML
    // uses pixels as world space (this is dumb). We don't want to work with pixels. So,
    // just as a proof of concept for now, I'm defining a our world space to be whatever
    // makes the view able to hold 100 units in the x direction
    auto const screenWindowWidth = window.getSize().x;
    auto const worldWindowWidth = 100.0f;
    auto const scale = screenWindowWidth / worldWindowWidth;
    auto worldTransform = sf::Transform();
    worldTransform.scale({scale, scale});
    auto const worldWindowHeight = window.getSize().y / scale;

    auto event = sf::Event();

    Map map;
    map.width = 10 * worldWindowWidth;
    map.height = worldWindowHeight;
    map.floorHeight = (2.0f / 3.0f) * worldWindowHeight;
    map.floorColor = sf::Color(255, 165, 0); // orange-ish
    map.skyColor = sf::Color(0, 180, 255); // cyan-ish

    Player player;
    player.boundingBox  = {worldWindowWidth / 10, worldWindowHeight / 10};
    player.position     = {worldWindowWidth / 2, worldWindowHeight / 2};
    player.velocity     = {0, 0};
    player.acceleration = {0, 0};

    std::chrono::microseconds prevFrameDuration = std::chrono::microseconds::zero();

    while (window.isOpen()) {
        auto const beginFrameTime = std::chrono::system_clock::now();

        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                {
                    window.close();
                } break;
                case sf::Event::KeyPressed:
                {
                    constexpr auto microsecondsInSecond = 1000000.0f;

                    if (event.key.code == sf::Keyboard::Key::W || event.key.code == sf::Keyboard::Key::Up) {
                        //auto view = window.getView();
                        //view.move({0.0f, -10.0f}); // remember +y is down (for now)
                        //window.setView(view);
                        //player.acceleration.y -= (prevFrameDuration.count() / microsecondsInSecond) * 100.0f;
                        player.acceleration.y -= 10.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::A || event.key.code == sf::Keyboard::Key::Left) {
                        //auto view = window.getView();
                        //view.move({-10.0f, 0.0f});
                        //window.setView(view);
                        //player.acceleration.x -= (prevFrameDuration.count() / microsecondsInSecond) * 100.0f;
                        player.acceleration.x = -20.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::S || event.key.code == sf::Keyboard::Key::Down) {
                        //auto view = window.getView();
                        //view.move({0.0f, 10.0f}); // remember +y is down (for now)
                        //window.setView(view);
                        player.acceleration.y += 40.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::D || event.key.code == sf::Keyboard::Key::Right) {
                        //auto view = window.getView();
                        //view.move({10.0f, 0.0f});
                        //window.setView(view);
                        //player.acceleration.x += (prevFrameDuration.count() / microsecondsInSecond) * 100.0f;
                        player.acceleration.x = 20.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Key::Q) {
                        window.close();
                    }
                    printf("acceleration: (x:%f, y:%f)\n", player.acceleration.x, player.acceleration.y);
                    printf("velocity:     (x:%f, y:%f)\n", player.velocity.x, player.velocity.y);
                } break;
                default: break; // ignore other events for now
            }
        }

        applyGravity(prevFrameDuration, player);

        updatePhysics(prevFrameDuration, map, player);

        window.clear(); // begin frame
        drawMap(window, map, worldTransform);
        drawPlayer(window, player, worldTransform);
        window.display(); // end frame
        auto const endFrameTime = std::chrono::system_clock::now();
        prevFrameDuration = std::chrono::duration_cast<decltype(prevFrameDuration)>(endFrameTime - beginFrameTime);
    }

    return 0;
}

void drawMap(sf::RenderWindow& window, Map const & map, sf::Transform const & worldTransform)
{
    auto floor = sf::RectangleShape({map.width, map.floorHeight});
    floor.move({0.0f, map.floorHeight});
    floor.setFillColor(map.floorColor);

    auto sky = sf::RectangleShape({map.width, map.floorHeight});
    sky.setFillColor(map.skyColor);

    window.draw(floor, worldTransform);
    window.draw(sky, worldTransform);
}

void drawPlayer(sf::RenderWindow& window, Player const & player, sf::Transform worldTransform)
{
    auto rect = sf::RectangleShape({player.boundingBox.width, player.boundingBox.height});
    rect.move({player.position.x, player.position.y});
    rect.setFillColor(sf::Color::Green);

    window.draw(rect, worldTransform);
}

void applyGravity(std::chrono::microseconds deltaTime, Player & player)
{
    constexpr auto microsecondsInSecond = 1000000.0f;
    constexpr auto gravity = 9.81f; // m / s^2
    player.acceleration.y += (deltaTime.count() / microsecondsInSecond) * gravity;
}

void updatePhysics(std::chrono::microseconds deltaTime, Map const & map, Player & player)
{
    constexpr auto microsecondsInSecond = 1000000.0f;
    auto const deltaTimeInSeconds = deltaTime.count() / microsecondsInSecond;

    auto const newPlayerPosition = ((1.0 / 2.0) * player.acceleration * (deltaTimeInSeconds * deltaTimeInSeconds))
                                 + (player.velocity * deltaTimeInSeconds)
                                 + player.position;

    auto newPlayerVelocity = (player.acceleration * deltaTimeInSeconds) + player.velocity;
    //newPlayerVelocity -= 0.01 * newPlayerVelocity;

    static auto printCount = 0;

    if (printCount++ < 0) {
        printf("\n\n");
        printf("newPlayerPosition.y: %f\tmap.floorHeight: %f\n", newPlayerPosition.y, map.floorHeight);
        printf("Player: Position: (%f, %f)\tvelocity: (%f, %f)\tacceleration: (%f, %f)\n",player.position.x, player.position.y, player.velocity.x, player.velocity.y, player.acceleration.x, player.acceleration.y);
        printf("newPlayerPosition: (x:%f, y:%f)\tnewPlayerVeclocity: (x:%f, y:%f)\tfloorHeight: %f\n", newPlayerPosition.x, newPlayerPosition.y, newPlayerVelocity.x, newPlayerVelocity.y, map.floorHeight);
    }

    player.position = newPlayerPosition;
    player.velocity = newPlayerVelocity;

    if (newPlayerPosition.y > (map.floorHeight - player.boundingBox.height)) {
        player.velocity.y = 0;
        player.acceleration.y = 0;
        player.position.y = map.floorHeight - player.boundingBox.height;
    }

    ++printCount;
}
