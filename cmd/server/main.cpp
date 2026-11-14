#include "networking/server.hpp"
#include "world/helpers/constants.h"
#include "world/world.hpp"
#include <SFML/Graphics.hpp>
#include <chrono>
#include <enet/enet.h>
#include <iostream>

int main()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet.\n";
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    // 1. Initialize World
    World world;
    world.init();

    // 2. Initialize Network Tool
    Server server;
    if (!server.start(1234))
    {
        return EXIT_FAILURE;
    }

    // 3. SFML Server Debug Window Setup
    sf::RenderWindow serverWindow(sf::VideoMode(WINDOW_SIZE_X, WINDOW_SIZE_Y),
                                  "Server Authoritative View");

    serverWindow.setFramerateLimit(MAX_FRAMERATE);
    sf::View camera = serverWindow.getDefaultView();

    sf::VertexArray worldPixels(sf::Quads, WORLD_SIZE_X * WORLD_SIZE_Y * 4);
    for (int y = 0; y < WORLD_SIZE_Y; ++y)
    {
        for (int x = 0; x < WORLD_SIZE_X; ++x)
        {
            int   idx = (y * WORLD_SIZE_X + x) * 4;
            float fx = x * CELL_SIZE, fy = y * CELL_SIZE;

            worldPixels[idx + 0].position = sf::Vector2f(fx, fy);
            worldPixels[idx + 1].position = sf::Vector2f(fx + CELL_SIZE, fy);
            worldPixels[idx + 2].position = sf::Vector2f(fx + CELL_SIZE, fy + CELL_SIZE);
            worldPixels[idx + 3].position = sf::Vector2f(fx, fy + CELL_SIZE);

            sf::Color defaultColor(0x828282FF);
            worldPixels[idx + 0].color = defaultColor;
            worldPixels[idx + 1].color = defaultColor;
            worldPixels[idx + 2].color = defaultColor;
            worldPixels[idx + 3].color = defaultColor;
        }
    }

    std::vector<Point> manual_brush_changes;
    const float        targetDt    = 1.0f / 60.0f;
    float              accumulator = 0.0f;
    auto               lastTime    = std::chrono::high_resolution_clock::now();

    // 4. Main Server Loop
    while (serverWindow.isOpen())
    {
        auto  currentTime = std::chrono::high_resolution_clock::now();
        float frameTime   = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime          = currentTime;
        if (frameTime > 0.25f)
        {
            frameTime = 0.25f;
        }
        accumulator += frameTime;

        // Window Events
        sf::Event sfEvent;
        while (serverWindow.pollEvent(sfEvent))
        {
            if (sfEvent.type == sf::Event::Closed)
            {
                serverWindow.close();
            }
        }

        // Camera Panning
        float panSpeed = 900.0f * frameTime;
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
        serverWindow.setView(camera);

        // --- NETWORK: Poll Events ---
        // Reads incoming brush inputs and adds them to manual_brush_changes
        // Handles new connections by syncing chunk data internally
        server.poll_events(world, manual_brush_changes);

        // --- PHYSICS & BROADCAST ---
        while (accumulator >= targetDt)
        {
            world.chunkManager.step();
            accumulator -= targetDt;

            const std::vector<Point> &physics_changes = world.chunkManager.get_changed_pixels();

            // Delegate packet batching and sending to the networking layer
            server.send_changed_pixels(physics_changes, manual_brush_changes, world);

            // Update Server's local debug window
            auto update_local_cache = [&](const std::vector<Point> &points)
            {
                for (const Point &p : points)
                {
                    int cx, cy, lx, ly;
                    if (world.chunkManager.get_chunk_for_global_cell(p.x, p.y, cx, cy, lx, ly))
                    {
                        const Element &e = world.chunkManager.chunk_at(cx, cy).at(lx, ly);
                        if (p.x >= 0 && p.x < WORLD_SIZE_X && p.y >= 0 && p.y < WORLD_SIZE_Y)
                        {
                            int       idx = (p.y * WORLD_SIZE_X + p.x) * 4;
                            sf::Color c(e.color);
                            worldPixels[idx + 0].color = c;
                            worldPixels[idx + 1].color = c;
                            worldPixels[idx + 2].color = c;
                            worldPixels[idx + 3].color = c;
                        }
                    }
                }
            };

            update_local_cache(physics_changes);
            update_local_cache(manual_brush_changes);
            manual_brush_changes.clear();
        }

        // --- RENDER ---
        serverWindow.clear(sf::Color::White);
        serverWindow.draw(worldPixels);

        // Draw Chunk Boundaries
        for (int cy = 0; cy < CHUNK_COUNT_Y; ++cy)
        {
            for (int cx = 0; cx < CHUNK_COUNT_X; ++cx)
            {
                const Chunk       &chunk = world.chunkManager.chunk_at(cx, cy);
                sf::RectangleShape chunkBox(
                    sf::Vector2f(CHUNK_SIZE_X * CELL_SIZE - 2, CHUNK_SIZE_Y * CELL_SIZE - 2));
                chunkBox.setPosition(cx * CHUNK_SIZE_X * CELL_SIZE + 1,
                                     cy * CHUNK_SIZE_Y * CELL_SIZE + 1);
                chunkBox.setFillColor(sf::Color::Transparent);
                chunkBox.setOutlineThickness(1.0f);
                chunkBox.setOutlineColor(chunk.active_index == UINT32_MAX ? sf::Color::Red
                                                                          : sf::Color::Green);
                serverWindow.draw(chunkBox);
            }
        }

        serverWindow.display();
    }

    return 0;
}
