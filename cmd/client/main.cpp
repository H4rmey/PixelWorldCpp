#include "networking/client.hpp"
#include "world/helpers/constants.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <enet/enet.h>
#include <iostream>

class Renderer
{
  private:
    sf::RenderWindow window;
    sf::View         camera;
    sf::VertexArray  worldPixels;

    void addCell(int x, int y, sf::Color c)
    {
        int   idx = (y * WORLD_SIZE_X + x) * 4;
        float fx  = x * CELL_SIZE;
        float fy  = y * CELL_SIZE;

        worldPixels[idx + 0].position = sf::Vector2f(fx, fy);
        worldPixels[idx + 1].position = sf::Vector2f(fx + CELL_SIZE, fy);
        worldPixels[idx + 2].position = sf::Vector2f(fx + CELL_SIZE, fy + CELL_SIZE);
        worldPixels[idx + 3].position = sf::Vector2f(fx, fy + CELL_SIZE);

        worldPixels[idx + 0].color = c;
        worldPixels[idx + 1].color = c;
        worldPixels[idx + 2].color = c;
        worldPixels[idx + 3].color = c;
    };

    sf::VertexArray createEmptyVertexArray()
    {
        for (int y = 0; y < WORLD_SIZE_Y; ++y)
        {
            for (int x = 0; x < WORLD_SIZE_X; ++x)
            {
                addCell(x, y, backgroundColor);
            }
        }
        return worldPixels;
    }

  public:
    Renderer() = default;

    sf::Color backgroundColor{0x828282FF};

    void init()
    {
        window.create(sf::VideoMode(WINDOW_SIZE_X, WINDOW_SIZE_Y), "World Simulation Client");
        window.setFramerateLimit(MAX_FRAMERATE);

        camera = window.getDefaultView();

        worldPixels = sf::VertexArray(sf::Quads, WORLD_SIZE_X * WORLD_SIZE_Y * 4);
        worldPixels = createEmptyVertexArray();
    };

    sf::RenderWindow &getWindow() { return window; };
    sf::View         &getCamera() { return camera; };
    sf::VertexArray  &getVertexArray() { return worldPixels; };
};

int main()
{
    // 1. Initialize Network Tool
    Client client;
    if (!client.init())
    {
        return EXIT_FAILURE;
    }
    if (!client.connect("127.0.0.1", 1234))
    {
        return EXIT_FAILURE;
    }

    // 2. Initialize SFML Window
    Renderer renderer;
    renderer.init();

    sf::RenderWindow &window      = renderer.getWindow();
    sf::VertexArray  &worldPixels = renderer.getVertexArray();
    sf::View         &camera      = renderer.getCamera();

    int     brush_size = BRUSH_SIZE;
    uint8_t brush_type = 1; // 1 = Sand

    // 3. Main Loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Num1)
                {
                    brush_type = 2;
                }
                if (event.key.code == sf::Keyboard::Num2)
                {
                    brush_type = 3;
                }

                if (event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal)
                {
                    brush_size = std::min(90, brush_size + 1);
                }
                if (event.key.code == sf::Keyboard::Subtract ||
                    event.key.code == sf::Keyboard::Dash)
                {
                    brush_size = std::max(0, brush_size - 1);
                }
            }
        }

        // Camera Panning
        float panSpeed = 900.0f * (1.0f / 60.0f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            camera.move(panSpeed, 0);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            camera.move(-panSpeed, 0);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        {
            camera.move(0, panSpeed);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        {
            camera.move(0, -panSpeed);
        }
        window.setView(camera);

        // --- NETWORK: Send Input ---
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) ||
            sf::Mouse::isButtonPressed(sf::Mouse::Right))
        {
            sf::Vector2i mouseScreen = sf::Mouse::getPosition(window);
            sf::Vector2f mouseWorld  = window.mapPixelToCoords(mouseScreen);

            int  gx    = static_cast<int>(mouseWorld.x) / CELL_SIZE;
            int  gy    = static_cast<int>(mouseWorld.y) / CELL_SIZE;
            bool erase = sf::Mouse::isButtonPressed(sf::Mouse::Right);

            // Delegate packet creation to the client tool
            client.send_brush_click(gx, gy, brush_size, brush_type, erase);
        }

        // --- NETWORK: Receive Updates ---
        // Client tool consumes packets and maps them onto worldPixels
        client.poll_events(worldPixels);

        // --- RENDER ---
        window.clear(renderer.backgroundColor);
        window.draw(worldPixels);

        // draw the brush
        sf::Vector2f    mWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::CircleShape brushPreview(brush_size * CELL_SIZE);
        brushPreview.setOrigin(brushPreview.getRadius(), brushPreview.getRadius());
        brushPreview.setPosition(mWorld);
        brushPreview.setFillColor(sf::Color::Transparent);
        brushPreview.setOutlineColor(sf::Color::Red);
        brushPreview.setOutlineThickness(1.0f);
        window.draw(brushPreview);

        window.display();
    }

    return 0; // atexit handles enet cleanup, destructor handles disconnect
}
