#include "network_protocol.h"
#include "world/elementFactory.hpp"
#include "world/helpers/constants.h"
#include "world/movementSystems.hpp"
#include "world/world.hpp"
#include <SFML/Graphics.hpp>
#include <chrono>
#include <enet/enet.h>
#include <iostream>
#include <vector>

int main()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet.\n";
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address = {ENET_HOST_ANY, 1234};
    ENetHost   *server  = enet_host_create(&address, 32, 2, 0, 0);

    // Initialize World
    World world;
    world.init();

    // ---------------------------------------------------------
    // Server Debug Window Setup
    // ---------------------------------------------------------
    sf::RenderWindow serverWindow(sf::VideoMode(WINDOW_SIZE_X, WINDOW_SIZE_Y),
                                  "Server Authoritative View");
    serverWindow.setFramerateLimit(MAX_FRAMERATE);
    sf::View camera = serverWindow.getDefaultView();

    // Preallocate server's visual representation of the pixels
    sf::VertexArray worldPixels(sf::Quads, WORLD_SIZE_X * WORLD_SIZE_Y * 4);
    for (int y = 0; y < WORLD_SIZE_Y; ++y)
    {
        for (int x = 0; x < WORLD_SIZE_X; ++x)
        {
            int   idx = (y * WORLD_SIZE_X + x) * 4;
            float fx  = x * CELL_SIZE;
            float fy  = y * CELL_SIZE;

            worldPixels[idx + 0].position = sf::Vector2f(fx, fy);
            worldPixels[idx + 1].position = sf::Vector2f(fx + CELL_SIZE, fy);
            worldPixels[idx + 2].position = sf::Vector2f(fx + CELL_SIZE, fy + CELL_SIZE);
            worldPixels[idx + 3].position = sf::Vector2f(fx, fy + CELL_SIZE);

            // Default color matching Empty element (0x828282FF from the factory)
            sf::Color defaultColor(0x828282FF);
            worldPixels[idx + 0].color = defaultColor;
            worldPixels[idx + 1].color = defaultColor;
            worldPixels[idx + 2].color = defaultColor;
            worldPixels[idx + 3].color = defaultColor;
        }
    }

    // Buffer to hold pixels placed by the mouse so they can be broadcast alongside physics
    std::vector<Point> manual_brush_changes;

    auto apply_brush = [&](int gx, int gy, int b_size, ElementType b_type, bool erase)
    {
        for (int dy = -b_size; dy <= b_size; dy++)
        {
            for (int dx = -b_size; dx <= b_size; dx++)
            {
                if (dx * dx + dy * dy <= b_size * b_size)
                {
                    int px = gx + dx;
                    int py = gy + dy;
                    int cx, cy, lx, ly;

                    if (world.chunkManager.get_chunk_for_global_cell(px, py, cx, cy, lx, ly))
                    {
                        ElementType type_to_set = erase ? ElementType::Empty : b_type;
                        Chunk      &chunk       = world.chunkManager.chunk_at(cx, cy);

                        if (type_to_set == ElementType::Empty)
                        {
                            int offset_x[] = {0, 0, -1, 1};
                            int offset_y[] = {-1, 1, 0, 0};
                            for (int i = 0; i < 4; ++i)
                            {
                                Element &neighbor =
                                    chunk.get_element_global(px + offset_x[i], py + offset_y[i]);
                                neighbor.physics.canMove_x = true;
                                neighbor.physics.canMove_y = true;
                            }
                        }

                        // Use the factory to spawn the element so colors and physics match
                        // perfectly
                        chunk.set_element_local(lx, ly, ElementFactory::create(type_to_set));

                        // Add to our manual changes buffer so the network loop knows to send it
                        manual_brush_changes.push_back({px, py});
                    }
                }
            }
        }
    };

    const float targetDt    = 1.0f / 60.0f;
    float       accumulator = 0.0f;
    auto        lastTime    = std::chrono::high_resolution_clock::now();

    const size_t                MAX_UPDATES_PER_PACKET = 175;
    std::vector<NetPixelUpdate> packet_batch;
    packet_batch.reserve(MAX_UPDATES_PER_PACKET);

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

        // ---------------------------------------------------------
        // SFML Window Events & Camera Panning (Server Side)
        // ---------------------------------------------------------
        sf::Event sfEvent;
        while (serverWindow.pollEvent(sfEvent))
        {
            if (sfEvent.type == sf::Event::Closed)
            {
                serverWindow.close();
            }
        }

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

        // ---------------------------------------------------------
        // Network Input & Connections
        // ---------------------------------------------------------
        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_CONNECT)
            {
                std::cout << "Client connected! Sending full world state...\n";
                std::vector<NetPixelUpdate> sync_batch;
                sync_batch.reserve(MAX_UPDATES_PER_PACKET);

                for (int cy = 0; cy < CHUNK_COUNT_Y; ++cy)
                {
                    for (int cx = 0; cx < CHUNK_COUNT_X; ++cx)
                    {
                        const Chunk &chunk = world.chunkManager.chunk_at(cx, cy);
                        for (int ly = 0; ly < CHUNK_SIZE_Y; ++ly)
                        {
                            for (int lx = 0; lx < CHUNK_SIZE_X; ++lx)
                            {
                                const Element &e = chunk.at(lx, ly);
                                if (e.type != ElementType::Empty)
                                {
                                    int gx = cx * CHUNK_SIZE_X + lx;
                                    int gy = cy * CHUNK_SIZE_Y + ly;
                                    sync_batch.push_back(
                                        {gx, gy, static_cast<uint8_t>(e.type), e.color});

                                    if (sync_batch.size() >= MAX_UPDATES_PER_PACKET)
                                    {
                                        ENetPacket *packet = enet_packet_create(
                                            sync_batch.data(),
                                            sync_batch.size() * sizeof(NetPixelUpdate),
                                            ENET_PACKET_FLAG_RELIABLE);
                                        enet_peer_send(event.peer, 0, packet);
                                        sync_batch.clear();
                                    }
                                }
                            }
                        }
                    }
                }
                if (!sync_batch.empty())
                {
                    ENetPacket *packet = enet_packet_create(
                        sync_batch.data(), sync_batch.size() * sizeof(NetPixelUpdate),
                        ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 0, packet);
                }
            }
            else if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                if (event.packet->data[0] == 0) // Input packet
                {
                    ClientInputPacket *input =
                        reinterpret_cast<ClientInputPacket *>(event.packet->data);
                    apply_brush(input->mouse_world_x, input->mouse_world_y, input->brush_size,
                                static_cast<ElementType>(input->brush_element), input->is_erasing);
                }
                enet_packet_destroy(event.packet);
            }
        }

        // ---------------------------------------------------------
        // Step Physics & Update Server Visuals
        // ---------------------------------------------------------
        while (accumulator >= targetDt)
        {
            world.chunkManager.step();
            accumulator -= targetDt;

            const std::vector<Point> &physics_changes = world.chunkManager.get_changed_pixels();
            packet_batch.clear();

            // Helper to process both physics points and brush points
            auto process_point = [&](const Point &p)
            {
                int cx, cy, lx, ly;
                if (world.chunkManager.get_chunk_for_global_cell(p.x, p.y, cx, cy, lx, ly))
                {
                    const Element &e = world.chunkManager.chunk_at(cx, cy).at(lx, ly);
                    packet_batch.push_back({p.x, p.y, static_cast<uint8_t>(e.type), e.color});

                    // Update the server's local visual cache instantly
                    if (p.x >= 0 && p.x < WORLD_SIZE_X && p.y >= 0 && p.y < WORLD_SIZE_Y)
                    {
                        int       idx = (p.y * WORLD_SIZE_X + p.x) * 4;
                        sf::Color newColor(e.color);
                        worldPixels[idx + 0].color = newColor;
                        worldPixels[idx + 1].color = newColor;
                        worldPixels[idx + 2].color = newColor;
                        worldPixels[idx + 3].color = newColor;
                    }

                    if (packet_batch.size() >= MAX_UPDATES_PER_PACKET)
                    {
                        ENetPacket *packet = enet_packet_create(
                            packet_batch.data(), packet_batch.size() * sizeof(NetPixelUpdate),
                            ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(server, 0, packet);
                        packet_batch.clear();
                    }
                }
            };

            // 1. Process physics movements
            for (const Point &p : physics_changes)
            {
                process_point(p);
            }

            // 2. Process manual brush strokes
            for (const Point &p : manual_brush_changes)
            {
                process_point(p);
            }

            // Clear manual changes until the next mouse click
            manual_brush_changes.clear();

            if (!packet_batch.empty())
            {
                ENetPacket *packet = enet_packet_create(
                    packet_batch.data(), packet_batch.size() * sizeof(NetPixelUpdate),
                    ENET_PACKET_FLAG_RELIABLE);
                enet_host_broadcast(server, 0, packet);
            }
        }

        // ---------------------------------------------------------
        // Server Rendering
        // ---------------------------------------------------------
        serverWindow.clear(sf::Color::White);

        // 1. Draw the pixels
        serverWindow.draw(worldPixels);

        // 2. Draw Chunk Activity Boundaries
        for (int cy = 0; cy < CHUNK_COUNT_Y; ++cy)
        {
            for (int cx = 0; cx < CHUNK_COUNT_X; ++cx)
            {
                const Chunk &chunk = world.chunkManager.chunk_at(cx, cy);

                int posx   = cx * CHUNK_SIZE_X * CELL_SIZE + 1;
                int posy   = cy * CHUNK_SIZE_Y * CELL_SIZE + 1;
                int width  = CHUNK_SIZE_X * CELL_SIZE - 2;
                int height = CHUNK_SIZE_Y * CELL_SIZE - 2;

                sf::RectangleShape chunkBox(sf::Vector2f(width, height));
                chunkBox.setPosition(posx, posy);
                chunkBox.setFillColor(sf::Color::Transparent);
                chunkBox.setOutlineThickness(1.0f);

                // Red = Inactive/Sleeping (UINT32_MAX), Green = Active
                chunkBox.setOutlineColor(chunk.active_index == UINT32_MAX ? sf::Color::Red
                                                                          : sf::Color::Green);

                serverWindow.draw(chunkBox);
            }
        }

        serverWindow.display();
    }

    enet_host_destroy(server);
    return 0;
}
