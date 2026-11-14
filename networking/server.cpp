#include "server.hpp"
#include "world/elementFactory.hpp"
#include <iostream>

Server::~Server() { stop(); }

bool Server::start(uint16_t port)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    // Max 32 clients, 3 channels (0, 1, 2)
    m_host = enet_host_create(&address, 32, CHANNEL_COUNT, 0, 0);
    if (!m_host)
    {
        std::cerr << "Failed to create ENet server host.\n";
        return false;
    }

    m_packet_batch.reserve(MAX_UPDATES_PER_PACKET);
    return true;
}

void Server::stop()
{
    if (m_host)
    {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
}

void Server::poll_events(World &world, std::vector<Point> &manual_brush_changes)
{
    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            on_client_connect(event, world);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            on_client_event(event, world, manual_brush_changes);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            on_client_disconnect(event);
            break;
        default:
            break;
        }
    }
}

void Server::on_client_connect(ENetEvent &event, World &world)
{
    std::cout << "Client connected from " << event.peer->address.host << "\n";
    send_chunk_data(event.peer, world);
}

void Server::on_client_disconnect(ENetEvent &event)
{
    std::cout << "Client disconnected.\n";
    event.peer->data = nullptr;
}

void Server::on_client_event(ENetEvent &event, World &world,
                             std::vector<Point> &manual_brush_changes)
{
    uint8_t packet_id = event.packet->data[0];

    if (packet_id == PACKET_BRUSH_INPUT)
    {
        ClientInputPacket *input = reinterpret_cast<ClientInputPacket *>(event.packet->data);
        apply_brush_to_world(input, world, manual_brush_changes);
    }

    enet_packet_destroy(event.packet);
}

void Server::apply_brush_to_world(ClientInputPacket *input, World &world,
                                  std::vector<Point> &manual_brush_changes)
{
    int         b_size = input->brush_size;
    ElementType b_type = static_cast<ElementType>(input->brush_element);
    bool        erase  = (input->is_erasing == 1);

    for (int dy = -b_size; dy <= b_size; dy++)
    {
        for (int dx = -b_size; dx <= b_size; dx++)
        {
            if (dx * dx + dy * dy <= b_size * b_size)
            {
                int px = input->mouse_world_x + dx;
                int py = input->mouse_world_y + dy;
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

                    chunk.set_element_local(lx, ly, ElementFactory::create(type_to_set));
                    manual_brush_changes.push_back({px, py});
                }
            }
        }
    }
}

void Server::send_chunk_data(ENetPeer *peer, World &world)
{
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

                        sync_batch.push_back({gx, gy, static_cast<uint8_t>(e.type), e.color});

                        if (sync_batch.size() >= MAX_UPDATES_PER_PACKET)
                        {
                            ENetPacket *packet = enet_packet_create(
                                sync_batch.data(), sync_batch.size() * sizeof(NetPixelUpdate),
                                ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(peer, CHANNEL_WORLD_SYNC, packet);
                            sync_batch.clear();
                        }
                    }
                }
            }
        }
    }

    if (!sync_batch.empty())
    {
        ENetPacket *packet =
            enet_packet_create(sync_batch.data(), sync_batch.size() * sizeof(NetPixelUpdate),
                               ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, CHANNEL_WORLD_SYNC, packet);
    }
}

void Server::send_changed_pixels(const std::vector<Point> &physics_changes,
                                 const std::vector<Point> &manual_brush_changes, World &world)
{
    m_packet_batch.clear();

    auto process_point = [&](const Point &p)
    {
        int cx, cy, lx, ly;
        if (world.chunkManager.get_chunk_for_global_cell(p.x, p.y, cx, cy, lx, ly))
        {
            const Element &e = world.chunkManager.chunk_at(cx, cy).at(lx, ly);

            m_packet_batch.push_back({p.x, p.y, static_cast<uint8_t>(e.type), e.color});

            if (m_packet_batch.size() >= MAX_UPDATES_PER_PACKET)
            {
                ENetPacket *packet = enet_packet_create(
                    m_packet_batch.data(), m_packet_batch.size() * sizeof(NetPixelUpdate),
                    ENET_PACKET_FLAG_RELIABLE);
                enet_host_broadcast(m_host, CHANNEL_PIXEL_UPDATES, packet);
                m_packet_batch.clear();
            }
        }
    };

    for (const Point &p : physics_changes)
    {
        process_point(p);
    }
    for (const Point &p : manual_brush_changes)
    {
        process_point(p);
    }

    if (!m_packet_batch.empty())
    {
        ENetPacket *packet = enet_packet_create(m_packet_batch.data(),
                                                m_packet_batch.size() * sizeof(NetPixelUpdate),
                                                ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(m_host, CHANNEL_PIXEL_UPDATES, packet);
    }
}
