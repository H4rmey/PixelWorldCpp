#include "client.hpp"
#include "../world/helpers/constants.h"
#include "networking/protocol.hpp"
#include <cstdio>
#include <enet/enet.h>
#include <iostream>

Client::~Client() { stop(); }

bool Client::init()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet.\n";
        return false;
    }
    atexit(enet_deinitialize);
    return true;
}

bool Client::connect(const std::string &host_ip, uint16_t port)
{
    m_host = enet_host_create(nullptr, 1, CHANNEL_COUNT, 0, 0);
    if (!m_host)
    {
        std::cerr << "Failed to create enet host for client\n";
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, host_ip.c_str());
    address.port = port;

    m_peer = enet_host_connect(m_host, &address, CHANNEL_COUNT, 0);
    if (!m_peer)
    {
        stop();
        std::cerr << "Failed to connect to server.\n";
        return false;
    }

    ENetEvent event;
    if (enet_host_service(m_host, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connected to server.\n";
        return true;
    }

    enet_peer_reset(m_peer);
    return false;
}

void Client::stop()
{
    if (m_peer)
    {
        enet_peer_disconnect(m_peer, 0);

        ENetEvent event;
        while (enet_host_service(m_host, &event, 100) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                enet_packet_destroy(event.packet);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
            {
                break;
            }
        }
        m_peer = nullptr;
    }
    if (m_host)
    {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
}

void Client::poll_events(sf::VertexArray &worldPixels)
{
    if (!m_host)
    {
        return;
    }

    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            on_packet_receive(event, worldPixels);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            std::cout << "Disconnected from server.\n";
            m_peer = nullptr;
            break;
        default:
            break;
        }
    }
}

void Client::on_packet_receive(ENetEvent &event, sf::VertexArray &worldPixels)
{
    // Check if this packet is on our pixel update or world sync channels
    if (event.channelID == CHANNEL_PIXEL_UPDATES || event.channelID == CHANNEL_WORLD_SYNC)
    {

        // Calculate the number of pixels in this batch
        size_t          update_count = event.packet->dataLength / sizeof(NetPixelUpdate);
        NetPixelUpdate *updates      = reinterpret_cast<NetPixelUpdate *>(event.packet->data);

        for (size_t i = 0; i < update_count; ++i)
        {
            NetPixelUpdate &u = updates[i];

            // Protect against out-of-bounds rendering
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
    }

    // Always clean up the packet when done!
    enet_packet_destroy(event.packet);
}

void Client::send_client_position(int gx, int gy)
{
    if (!m_peer)
    {
        return;
    }

    ClientPositionPacket input;
    input.packet_id     = PACKET_MOUSE_POSITION;
    input.mouse_world_x = static_cast<int32_t>(gx);
    input.mouse_world_y = static_cast<int32_t>(gy);

    // Send on our dedicated input channel
    ENetPacket *packet = enet_packet_create(&input, sizeof(ClientPositionPacket),
                                            ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(m_peer, CHANNEL_MOUSE_POSITION, packet);
}

void Client::send_brush_click(int gx, int gy, int brush_size, uint8_t element_type, bool erase)
{
    if (!m_peer)
    {
        return;
    }

    ClientInputPacket input;
    input.packet_id     = PACKET_BRUSH_INPUT;
    input.mouse_world_x = static_cast<int32_t>(gx);
    input.mouse_world_y = static_cast<int32_t>(gy);
    input.brush_size    = static_cast<uint8_t>(brush_size);
    input.brush_element = element_type;
    input.is_erasing    = erase ? 1 : 0;

    // Send on our dedicated input channel
    ENetPacket *packet =
        enet_packet_create(&input, sizeof(ClientInputPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(m_peer, CHANNEL_INPUT, packet);
}
