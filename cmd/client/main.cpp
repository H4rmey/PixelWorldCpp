#include <cstring>
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

    // Create client: no address bounds, 1 outgoing connection, 2 channels, unlimited bandwidth
    ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (client == nullptr)
    {
        std::cerr << "An error occurred while trying to create an ENet client host.\n";
        return EXIT_FAILURE;
    }

    ENetAddress address;
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 1234;

    ENetPeer *peer = enet_host_connect(client, &address, 2, 0);
    if (peer == nullptr)
    {
        std::cerr << "No available peers for initiating an ENet connection.\n";
        return EXIT_FAILURE;
    }

    ENetEvent event;
    // Wait up to 5 seconds for the connection attempt to succeed
    if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connection to 127.0.0.1:1234 succeeded.\n";
    }
    else
    {
        // Connection failed or timed out
        enet_peer_reset(peer);
        std::cout << "Connection to 127.0.0.1:1234 failed.\n";
        return EXIT_FAILURE;
    }

    // Send a test packet
    const char *testData = "Hello from Client!";
    ENetPacket *packet =
        enet_packet_create(testData, strlen(testData) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);

    // Main client loop
    while (true)
    {
        while (enet_host_service(client, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "A packet of length " << event.packet->dataLength
                          << " was received on channel " << static_cast<int>(event.channelID)
                          << "\n";
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnected from server.\n";
                return EXIT_SUCCESS;

            default:
                break;
            }
        }
    }

    enet_host_destroy(client);
    return EXIT_SUCCESS;
}
