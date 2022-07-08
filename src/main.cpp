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

    float dot(Vec2 const & rhs) const { return (x * rhs.y) + (y * rhs.x); }
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

    Point2 operator+(Point2 const & rhs) { return Point2(x + rhs.x, y + rhs.y); }
    Point2 operator-(Point2 const & rhs) { return Point2(x + rhs.x, y + rhs.y); }
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

struct Block
{
    Rect boundingBox;
    Point2 position;
};

constexpr auto microsecondsInSecond = 1000000.0f;

void drawMap(sf::RenderWindow& window, Map const & map);
void drawPlayer(sf::RenderWindow& window, Player const & player);
void applyGravity(std::chrono::microseconds deltaTime, Player & player);
void initBlocks(Map const & map, std::vector<Block>& blocks);
void drawBlocks(sf::RenderWindow& window, std::vector<Block>& blocks);
void updatePhysics(std::chrono::microseconds deltaTime, Map const & map, Player & player, std::vector<Block> const & blocks);

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
    map.floorHeight = (2.0f / 3.0f) * worldWindowHeight;
    map.floorColor = sf::Color(255, 165, 0); // orange-ish
    map.skyColor = sf::Color(0, 180, 255); // cyan-ish

    Player player;
    player.boundingBox  = {worldWindowWidth / 10, worldWindowHeight / 10};
    player.position     = {worldWindowWidth / 2, worldWindowHeight / 2};
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
                        player.acceleration.y = -10.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::A || event.key.code == sf::Keyboard::Key::Left) {
                        player.acceleration.x = -20.0f;
                    }
                    else if (event.key.code == sf::Keyboard::Key::S || event.key.code == sf::Keyboard::Key::Down) {
                        player.acceleration.y = 40.0f;
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
        currentView.setCenter({player.position.x, player.position.y});
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
    floor.move({0.0f, map.floorHeight});
    floor.setFillColor(map.floorColor);

    auto sky = sf::RectangleShape({map.width, map.floorHeight});
    sky.setFillColor(map.skyColor);

    window.draw(floor);
    window.draw(sky);
}

void drawPlayer(sf::RenderWindow& window, Player const & player)
{
    auto rect = sf::RectangleShape({player.boundingBox.width, player.boundingBox.height});
    rect.move({player.position.x, player.position.y});
    rect.setFillColor(sf::Color::Green);

    window.draw(rect);
}

void applyGravity(std::chrono::microseconds deltaTime, Player & player)
{
    constexpr auto gravity = 9.81f; // m / s^2
    player.acceleration.y += (deltaTime.count() / microsecondsInSecond) * gravity;
}

void initBlocks(Map const & map, std::vector<Block>& blocks)
{
    auto const numBlocks = map.width / 20.0f;
    auto const blockSpacing = map.width / 20.0f;
    auto const blockWidth = map.width / 200.0f;
    auto const blockHeight = map.height / 15.0f;

    for (int i = 0; i < numBlocks; ++i) {
        auto newBlock = Block();
        newBlock.boundingBox = {blockWidth, blockHeight};
        newBlock.position = {i * blockSpacing, map.floorHeight - blockHeight};
        blocks.push_back(newBlock);
    }
}

void drawBlocks(sf::RenderWindow& window, std::vector<Block>& blocks)
{
    for (auto& block : blocks) {
        auto rect = sf::RectangleShape({block.boundingBox.width, block.boundingBox.height});
        rect.move({block.position.x, block.position.y});
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
    newPlayerVelocity -= 0.0004 * newPlayerVelocity;

#if 0
    for (auto& block : blocks) {
        if (newPlayerPosition.x < block.position.x  + block.boundingBox.width // my left is less than your right
                && newPlayerPosition.x + player.boundingBox.width > block.position.x // my right greater than your left
                && newPlayerPosition.y < block.position.y + block.boundingBox.height // my top is above your bottom
                && newPlayerPosition.y + player.boundingBox.height > block.position.y) { // bottom is above your top
            // collision with block, no need to update position
            player.velocity = {0, 0};
            player.acceleration = {0, 0};
            return;
        }
    }
#endif

    player.position = newPlayerPosition;
    player.velocity = newPlayerVelocity;

#if 0
    if (newPlayerPosition.y > (map.floorHeight - player.boundingBox.height)
            && newPlayerPosition.x >= 0 - player.boundingBox.width
            && newPlayerPosition.x <= map.width) {
        player.velocity.y = 0;
        player.acceleration.y = 0;
        player.position.y = map.floorHeight - player.boundingBox.height;
    }
#endif

    if (newPlayerPosition.y + player.boundingBox.height > map.floorHeight
            && newPlayerPosition.x >= 0 - player.boundingBox.width
            && newPlayerPosition.x <= map.width) {
        printf("collision!\n");
        printf("velocity: (x:%f, y:%f)\n", player.velocity.x, player.velocity.y);
        auto const reflection = Vec2(0, -1);
        auto const dotResult = dot(player.velocity, reflection);
        printf("dot: %f\n", dotResult);
        player.velocity = player.velocity - (2 * dot(player.velocity, reflection) * reflection);
        player.acceleration = {0, 0};
        printf("velocity: (x:%f, y:%f)\n\n\n", player.velocity.x, player.velocity.y);
    }
}
