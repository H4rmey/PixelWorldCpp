#include "network_protocol.h"
#include "world/helpers/constants.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <enet/enet.h>
#include <iostream>

sf::VertexArray createEmptyVertexArray()
{
    // Preallocate a quad (4 vertices) for every single pixel in the world.
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

            // Default color matching your Empty element (0x828282FF)
            sf::Color defaultColor(0x828282FF);
            worldPixels[idx + 0].color = defaultColor;
            worldPixels[idx + 1].color = defaultColor;
            worldPixels[idx + 2].color = defaultColor;
            worldPixels[idx + 3].color = defaultColor;
        }
    }
    return worldPixels;
}

int main()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet.\n";
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetHost   *client = enet_host_create(nullptr, 1, 2, 0, 0);
    ENetAddress address;
    enet_address_set_host(&address, "127.0.0.1");
    address.port   = 1234;
    ENetPeer *peer = enet_host_connect(client, &address, 2, 0);

    sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE_X, WINDOW_SIZE_Y), "World Simulation Client");
    window.setFramerateLimit(MAX_FRAMERATE);

    sf::VertexArray worldPixels = createEmptyVertexArray();

    sf::View camera = window.getDefaultView();

    int     brush_size = BRUSH_SIZE;
    uint8_t brush_type = 1; // 1 = Sand

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            // SFML Keybindings
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Num1)
                {
                    brush_type = 1; // Sand
                    printf("Set brush to Sand\n");
                }
                if (event.key.code == sf::Keyboard::Num2)
                {
                    brush_type = 2; // Water
                    printf("Set brush to Water\n");
                }
                // ... map 3 through 8 as needed

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
        float dt       = 1.0f / 60.0f;
        float panSpeed = 900.0f * dt;
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

        // Send Mouse Input to Server
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) ||
            sf::Mouse::isButtonPressed(sf::Mouse::Right))
        {
            sf::Vector2i mouseScreen = sf::Mouse::getPosition(window);
            sf::Vector2f mouseWorld  = window.mapPixelToCoords(mouseScreen);

            ClientInputPacket input;
            input.packet_id     = 0;
            input.mouse_world_x = static_cast<int32_t>(mouseWorld.x) / CELL_SIZE;
            input.mouse_world_y = static_cast<int32_t>(mouseWorld.y) / CELL_SIZE;
            input.brush_size    = brush_size;
            input.brush_element = brush_type;
            input.is_erasing    = sf::Mouse::isButtonPressed(sf::Mouse::Right) ? 1 : 0;

            ENetPacket *packet =
                enet_packet_create(&input, sizeof(ClientInputPacket), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);
        }

        // Process Network Updates from Server
        ENetEvent netEvent;
        while (enet_host_service(client, &netEvent, 0) > 0)
        {
            if (netEvent.type == ENET_EVENT_TYPE_RECEIVE)
            {
                size_t          update_count = netEvent.packet->dataLength / sizeof(NetPixelUpdate);
                NetPixelUpdate *updates = reinterpret_cast<NetPixelUpdate *>(netEvent.packet->data);

                for (size_t i = 0; i < update_count; ++i)
                {
                    NetPixelUpdate &u = updates[i];

                    if (u.global_x >= 0 && u.global_x < WORLD_SIZE_X && u.global_y >= 0 &&
                        u.global_y < WORLD_SIZE_Y)
                    {
                        int idx = (u.global_y * WORLD_SIZE_X + u.global_x) * 4;

                        sf::Color newColor(u.color);

                        worldPixels[idx + 0].color = newColor;
                        worldPixels[idx + 1].color = newColor;
                        worldPixels[idx + 2].color = newColor;
                        worldPixels[idx + 3].color = newColor;
                    }
                }
                enet_packet_destroy(netEvent.packet);
            }
        }

        // Rendering
        sf::Color defaultColor(0x828282FF);
        window.clear(defaultColor);

        // Draw the massive world
        window.draw(worldPixels);

        // Brush preview
        sf::Vector2i    mScreen = sf::Mouse::getPosition(window);
        sf::Vector2f    mWorld  = window.mapPixelToCoords(mScreen);
        sf::CircleShape brushPreview(brush_size * CELL_SIZE);
        brushPreview.setOrigin(brushPreview.getRadius(), brushPreview.getRadius());
        brushPreview.setPosition(mWorld);
        brushPreview.setFillColor(sf::Color::Transparent);
        brushPreview.setOutlineColor(sf::Color::Red);
        brushPreview.setOutlineThickness(1.0f);
        window.draw(brushPreview);

        window.display();
    }

    enet_host_destroy(client);
    return 0;
}
