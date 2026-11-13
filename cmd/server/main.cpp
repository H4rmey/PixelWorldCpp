#include "world/helpers/constants.h"
#include "world/helpers/element_type.h"
#include "world/helpers/physics.hpp"
#include "world/helpers/renderLayer.h"
#include "world/movement/IMovement.h"
#include "world/movementSystems.hpp"
#include "world/world.hpp"
#include <algorithm>
#include <cmath>
#include <enet/enet.h>
#include <iostream>
#include <raylib.h>

int main()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet.\n";
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    // create the world
    printf("Start: creating world\n");
    World world = World();
    world.init();
    printf("Done: creating world\n");

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 1234;

    // Create server: address, max connections (32), channels (2), incoming bandwidth (0 =
    // unlimited), outgoing bandwidth (0 = unlimited)
    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);
    if (server == nullptr)
    {
        std::cerr << "An error occurred while trying to create an ENet server host.\n";
        return EXIT_FAILURE;
    }

    std::cout << "Server started on port 1234. Waiting for clients...\n";

    ENetEvent event;
    // Main server loop
    while (true)
    {
        // Wait up to 1000 milliseconds for an event
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A new client connected from " << std::endl;
                // Store custom data about the client (e.g., player ID)
                event.peer->data = (void *)"Client Information";
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "A packet of length " << event.packet->dataLength << " containing "
                          << (char *)event.packet->data << " was received from "
                          << (char *)event.peer->data << " on channel "
                          << static_cast<int>(event.channelID) << "\n";
                // Clean up the packet now that we're done using it
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << (char *)event.peer->data << " disconnected.\n";
                // Reset the peer's client information
                event.peer->data = nullptr;
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }
    }

    enet_host_destroy(server);
    return EXIT_SUCCESS;
}
