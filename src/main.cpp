#include <chrono>
#include <cstdio>
#include <limits>
#include <vector>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

struct Vec2
{
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    float x;
    float y;

    bool operator==(Vec2 const & rhs) const { return x == rhs.x && y == rhs.y; }
    bool operator!=(Vec2 const & rhs) const { return !operator==(rhs); }

    Vec2 operator+(Vec2 const & rhs) { return Vec2(x + rhs.x, y + rhs.y); }
    Vec2 operator-(Vec2 const & rhs) { return Vec2(x - rhs.x, y - rhs.y); }

    Vec2& operator-=(Vec2 const & rhs) { x -= rhs.x; y -= rhs.y; return *this; }

    float dot(Vec2 const & rhs) const { return (x * rhs.x) + (y * rhs.y); }
};

template <typename Number>
Vec2 operator*(Number n, Vec2 const & v) { return Vec2(v.x * n, v.y * n); }

template <typename Number>
Vec2 operator*(Vec2 const & v, Number n) { return operator*(n, v);}

float dot(Vec2 const & lhs, Vec2 const & rhs)
{
    return lhs.dot(rhs);
}

struct Point2
{
    Point2() = default;
    Point2(float x, float y) : x(x), y(y) {}

    float x;
    float y;

    Point2& operator=(Point2 const & rhs) { x = rhs.x; y = rhs.y; return *this; }

    Vec2 operator-(Point2 const & rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
};

Point2 operator+(Vec2 const & v, Point2 const & p) {return Point2{v.x + p.x, v.y + p.y};}
Point2 operator+(Point2 const & p, Vec2 const & v) {return operator+(v, p);}

struct Rect
{
    Rect() = default;
    Rect(float width, float height) : width(width), height(height) {}

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

struct Block
{
    Rect boundingBox;
    Point2 position;
};

struct CollisionInfo
{
    Vec2  normal;               //< vector normal to collision surface
    float percentThroughMove;   //< How far through the move the collision occured at
    bool  collided = false;;
};

constexpr auto microsecondsInSecond = 1000000.0f;
auto gScreenHeight = 0.0f;

void drawMap(sf::RenderWindow& window, Map const & map);
void drawPlayer(sf::RenderWindow& window, Player const & player);
void applyGravity(std::chrono::microseconds deltaTime, Player & player);
void initBlocks(Map const & map, std::vector<Block>& blocks);
void drawBlocks(sf::RenderWindow& window, std::vector<Block>& blocks);
void updatePhysics(std::chrono::microseconds deltaTime, Map const & map, Player & player, std::vector<Block> const & blocks);
CollisionInfo detectCollisions(Point2 const & oldMoverPosition,
                               Point2 const & newMoverPosition,
                               Rect const & moverBoundingBox,
                               Point2 const & stationaryPosition,
                               Rect const & stationaryBoundingBox);

int main()
{
    auto const videoModes = sf::VideoMode::getFullscreenModes();
    assert(!videoModes.empty());

    auto videoMode = sf::VideoMode({800, 600}); // default mode that should never be used
    if (!videoModes.empty()) {
        videoMode = videoModes.front(); // these are sorted in order from best to worst
    }

    auto window = sf::RenderWindow(videoMode, "Cool game");
    window.setVerticalSyncEnabled(true);

    // Next we're going to create a matrix to define our world position.
    // uses pixels as world space (this is dumb). We don't want to work with pixels. So,
    // just as a proof of concept for now, I'm defining a our world space to be whatever
    // makes the view able to hold 100 units in the x direction
    auto const screenWindowWidth = window.getSize().x;
    auto const worldWindowWidth = 100.0f;
    auto const scale = screenWindowWidth / worldWindowWidth;
    auto const worldWindowHeight = window.getSize().y / scale;
    gScreenHeight = worldWindowHeight;

    // SFML also dumbly has the positive y axis pointed down the screen, we're going to
    // modify the view to fix that
    auto currentView = window.getView();

    //auto currentView = window.getView();
    currentView.reset({{0, 0}, {worldWindowWidth, worldWindowHeight}});
    //auto currentTransform = currentView.getTransform();;
    //currentView.setSize({worldWindowWidth, -worldWindowHeight});
    window.setView(currentView);

    auto event = sf::Event();

    Map map;
    map.width = 10 * worldWindowWidth;
    map.height = worldWindowHeight;
    map.floorHeight = (1.0f / 3.0f) * worldWindowHeight;
    map.floorColor = sf::Color(255, 165, 0); // orange-ish
    map.skyColor = sf::Color(0, 180, 255); // cyan-ish

    Player player;
    player.boundingBox  = {worldWindowWidth / 10, worldWindowHeight / 10};
    player.position     = {worldWindowWidth / 2, worldWindowHeight / 2 + 10};
    player.velocity     = {0, 0};
    player.acceleration = {0, 0};

    auto blocks = std::vector<Block>();
    initBlocks(map, blocks);

    std::chrono::microseconds prevFrameDuration = std::chrono::microseconds::zero();

    auto frameRateFont = sf::Font();
    auto const success = frameRateFont.loadFromFile("/Library/Fonts/Arial Unicode.ttf");
    assert(success);

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
                    if (event.key.code == sf::Keyboard::Key::W || event.key.code == sf::Keyboard::Key::Up) {
                        player.acceleration.y = 50.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::A || event.key.code == sf::Keyboard::Key::Left) {
                        player.acceleration.x = -20.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::S || event.key.code == sf::Keyboard::Key::Down) {
                        player.acceleration.y = -40.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::D || event.key.code == sf::Keyboard::Key::Right) {
                        player.acceleration.x = 20.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Key::Q) {
                        window.close();
                    }
                } break;
                case sf::Event::KeyReleased:
                {
                    if (event.key.code == sf::Keyboard::Key::W || event.key.code == sf::Keyboard::Key::Up) {
                        player.acceleration.y = 0;
                    }
                    else if (event.key.code == sf::Keyboard::Key::A || event.key.code == sf::Keyboard::Key::Left) {
                        player.acceleration.x = 0;
                    }
                    else if (event.key.code == sf::Keyboard::Key::S || event.key.code == sf::Keyboard::Key::Down) {
                        player.acceleration.y = 0;
                    }
                    else if (event.key.code == sf::Keyboard::Key::D || event.key.code == sf::Keyboard::Key::Right) {
                        player.acceleration.x = 0;
                    }

                } break;
                default: break; // ignore other events for now
            }
        }

        applyGravity(prevFrameDuration, player);

        updatePhysics(prevFrameDuration, map, player, blocks);

        static auto printCount = 0;

        auto currentView = window.getView();
        currentView.setCenter({player.position.x, gScreenHeight-player.position.y});
        window.setView(currentView);

        window.clear(); // begin frame
        drawMap(window, map);
        drawPlayer(window, player);
        drawBlocks(window, blocks);

        auto const frameRate = 1.0 / prevFrameDuration.count() * microsecondsInSecond;
        auto const txt = std::to_string(frameRate);
        auto frameRateText = sf::Text(txt, frameRateFont);
        frameRateText.setCharacterSize(12);
        window.draw(frameRateText);

        window.display(); // end frame
        auto const endFrameTime = std::chrono::system_clock::now();
        prevFrameDuration = std::chrono::duration_cast<decltype(prevFrameDuration)>(endFrameTime - beginFrameTime);
    }

    return 0;
}

void drawMap(sf::RenderWindow& window, Map const & map)
{
    auto floor = sf::RectangleShape({map.width, map.floorHeight});
    floor.move({0.0f, map.height - map.floorHeight});
    floor.setFillColor(map.floorColor);

    auto sky = sf::RectangleShape({map.width, map.height - map.floorHeight});
    sky.setFillColor(map.skyColor);

    window.draw(floor);
    window.draw(sky);
}

void drawPlayer(sf::RenderWindow& window, Player const & player)
{
    auto rect = sf::RectangleShape({player.boundingBox.width, player.boundingBox.height});
    rect.move({player.position.x, gScreenHeight - player.position.y});
    rect.setFillColor(sf::Color::Green);

    window.draw(rect);
}

void applyGravity(std::chrono::microseconds deltaTime, Player & player)
{
    constexpr auto gravity = -9.81f; // m / s^2
    player.acceleration.y += (deltaTime.count() / microsecondsInSecond) * gravity;
}

void initBlocks(Map const & map, std::vector<Block>& blocks)
{
    auto const numBlocks = map.width / 20.0f;
    auto const blockSpacing = map.width / 20.0f;
    auto const blockWidth = map.width / 200.0f;
    auto const blockHeight = map.height / 9.0f;

    for (int i = 0; i < numBlocks; ++i) {
        auto newBlock = Block();
        newBlock.boundingBox = {blockWidth, blockHeight};
        newBlock.position = {i * blockSpacing, map.floorHeight + blockHeight};
        blocks.push_back(newBlock);
    }
}

void drawBlocks(sf::RenderWindow& window, std::vector<Block>& blocks)
{
    for (auto& block : blocks) {
        auto rect = sf::RectangleShape({block.boundingBox.width, block.boundingBox.height});
        rect.move({block.position.x, gScreenHeight - block.position.y});
        rect.setFillColor(sf::Color::Red);

        window.draw(rect);
    }
}

void updatePhysics(std::chrono::microseconds deltaTime, Map const & map, Player & player, std::vector<Block> const & blocks)
{
    auto const deltaTimeInSeconds = deltaTime.count() / microsecondsInSecond;

    auto const newPlayerPosition = ((1.0 / 2.0) * player.acceleration * (deltaTimeInSeconds * deltaTimeInSeconds))
                                 + (player.velocity * deltaTimeInSeconds)
                                 + player.position;

    auto newPlayerVelocity = (player.acceleration * deltaTimeInSeconds) + player.velocity;
    newPlayerVelocity -= 0.00004 * newPlayerVelocity;

    for (auto& block : blocks) {
        auto collisionResult = detectCollisions(player.position, newPlayerPosition, player.boundingBox, block.position, block.boundingBox);

        if (collisionResult.collided) {
            player.velocity = player.velocity - (dot(player.velocity, collisionResult.normal) * collisionResult.normal);

            if (collisionResult.normal == Vec2(0.0f, 1.0f)) {
                // landed on something, must give reaction to gravity
                player.acceleration.y = (deltaTime.count() / microsecondsInSecond) * 9.81f;
            }
            return;
        }
    }

    if (newPlayerPosition.y - player.boundingBox.height <= map.floorHeight
            && newPlayerPosition.x >= 0 - player.boundingBox.width
            && newPlayerPosition.x <= map.width) {
        auto const reflection = Vec2(0, 1);
        player.velocity = player.velocity - (1 * dot(player.velocity, reflection) * reflection);
        player.acceleration.y = (deltaTime.count() / microsecondsInSecond) * 9.81; // equal and oposite gravitational force
        return;
    }

    player.position = newPlayerPosition;
    player.velocity = newPlayerVelocity;
}

CollisionInfo detectCollisions(Point2 const & oldMoverPosition,
                               Point2 const & newMoverPosition,
                               Rect const & moverBoundingBox,
                               Point2 const & stationaryPosition,
                               Rect const & stationaryBoundingBox)
{
    // This algorithm works by finding the "time" it would take to collide with
    // each wall of the stationary object, and selects the normal based off of
    // which wall would be hit first

    // Minkowski adjustments
    auto const adjustedStationaryPosition = Point2(stationaryPosition.x - moverBoundingBox.width, stationaryPosition.y + moverBoundingBox.height);
    auto const adjustedStationaryBoundingBox = Rect(stationaryBoundingBox.width + moverBoundingBox.width, stationaryBoundingBox.height + moverBoundingBox.height);

    auto const moverDelta = newMoverPosition - oldMoverPosition;
    CollisionInfo result;
    auto tSoFar = 1;

    if (moverDelta.x != 0.0f) { // prevent divide by 0
        // left wall vs left wall (NOTE: left vs left is correct because of minkowski)
        auto const tll = (adjustedStationaryPosition.x - oldMoverPosition.x) / moverDelta.x;

        if (tll >= 0.0f && tll < tSoFar) {
            // check that y would be within collision bounds
            auto const yll = oldMoverPosition.y + (tll * moverDelta).y;

            if (yll < adjustedStationaryPosition.y && yll > adjustedStationaryPosition.y - adjustedStationaryBoundingBox.height) {
                tSoFar = tll;
                result.normal = Vec2(-1.0f, 0.0f);
                result.collided = true;
            }
        }

        // left wall vs right wall
        auto const tlr = (adjustedStationaryPosition.x + adjustedStationaryBoundingBox.width - oldMoverPosition.x) / moverDelta.x;

        if (tlr >= 0.0f && tlr < tSoFar) {
            // check that y would be within collision bounds
            auto const ylr = oldMoverPosition.y + (tlr * moverDelta).y;

            if (ylr < adjustedStationaryPosition.y && ylr > adjustedStationaryPosition.y - adjustedStationaryBoundingBox.height) {
                tSoFar = tlr;
                result.normal = Vec2(1.0f, 0.0f);
                result.collided = true;
            }
        }
    }

    if (moverDelta.y != 0.0f) {
        // top wall vs top wall
        auto const ttt = (adjustedStationaryPosition.y - oldMoverPosition.y) / moverDelta.y;

        if (ttt >= 0.0f && ttt < tSoFar) {
            auto xtt = oldMoverPosition.x + (ttt * moverDelta).x;

            if (xtt > adjustedStationaryPosition.x && xtt < adjustedStationaryPosition.x + adjustedStationaryBoundingBox.width) {
                tSoFar = ttt;
                result.normal = Vec2(0.0f, 1.0f);
                result.collided = true;
            }
        }
    }

    result.percentThroughMove = tSoFar;

    return result;
}
