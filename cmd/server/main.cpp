#include "network_protocol.h"
#include "world/movementSystems.hpp"
#include "world/world.hpp"
#include <chrono>
#include <enet/enet.h>
#include <iostream>
#include <vector>

int main()
{
    enet_initialize();
    ENetAddress address = {ENET_HOST_ANY, 1234};
    ENetHost   *server  = enet_host_create(&address, 32, 2, 0, 0);

    // Initialize World & Movement Systems
    World world;
    world.init();

    SandMovement        sand_movement;
    SolidMovement       solid_movement;
    GasMovement         gas_movement;
    LiquidMovement      liquid_movement;
    EmptyMovement       empty_movement;
    ScaffoldingMovement scaffolding_movement;

    // A helper to map ElementType back to the correct movement/physics/render pointers
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

                        IMovement  *move_ptr     = &empty_movement;
                        RenderLayer matter_state = RenderLayer::None;
                        Physics     physics_type = Physics::Empty;

                        if (!erase)
                        {
                            switch (type_to_set)
                            {
                            case ElementType::Sand:
                                move_ptr     = &sand_movement;
                                matter_state = RenderLayer::Solid;
                                physics_type = Physics::Sand;
                                break;
                            case ElementType::Water:
                                move_ptr     = &liquid_movement;
                                matter_state = RenderLayer::Liquid;
                                physics_type = Physics::Water;
                                break;
                            case ElementType::Gas:
                                move_ptr     = &gas_movement;
                                matter_state = RenderLayer::Gas;
                                physics_type = Physics::Gas;
                                break;
                            case ElementType::Stone:
                                move_ptr     = &solid_movement;
                                matter_state = RenderLayer::Solid;
                                physics_type = Physics::Stone;
                                break;
                            case ElementType::Scaffolding:
                                move_ptr     = &scaffolding_movement;
                                matter_state = RenderLayer::Solid;
                                physics_type = Physics::Scaffolding;
                                break;
                            case ElementType::Custom:
                            case ElementType::Bedrock:
                            default:
                                break;
                            }
                        }

                        Chunk &chunk = world.chunkManager.chunk_at(cx, cy);
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
                        chunk.set_element_local(
                            lx, ly, Element(matter_state, type_to_set, 0, move_ptr, physics_type));
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

    while (true)
    {
        auto  currentTime = std::chrono::high_resolution_clock::now();
        float frameTime   = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime          = currentTime;
        if (frameTime > 0.25f)
        {
            frameTime = 0.25f;
        }
        accumulator += frameTime;

        // 1. Process Network Input & Connections
        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_CONNECT)
            {
                std::cout << "Client connected! Sending full world state...\n";
                std::vector<NetPixelUpdate> sync_batch;
                sync_batch.reserve(MAX_UPDATES_PER_PACKET);

                // Iterate through the entire world and send non-empty pixels
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

                                    // Send batch strictly to the new connecting peer
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
                // Send remainder of the world sync
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

        // 2. Step Physics
        while (accumulator >= targetDt)
        {
            world.chunkManager.step(); // Automatically clears and repopulates m_changed_pixels
            accumulator -= targetDt;

            // 3. Broadcast Changes
            const std::vector<Point> &changed_points = world.chunkManager.get_changed_pixels();
            packet_batch.clear();
            for (const Point &p : changed_points)
            {
                int cx, cy, lx, ly;
                if (world.chunkManager.get_chunk_for_global_cell(p.x, p.y, cx, cy, lx, ly))
                {
                    const Element &e = world.chunkManager.chunk_at(cx, cy).at(lx, ly);

                    packet_batch.push_back({p.x, p.y, static_cast<uint8_t>(e.type), e.color});

                    if (packet_batch.size() >= MAX_UPDATES_PER_PACKET)
                    {
                        ENetPacket *packet = enet_packet_create(
                            packet_batch.data(), packet_batch.size() * sizeof(NetPixelUpdate),
                            ENET_PACKET_FLAG_RELIABLE);
                        enet_host_broadcast(server, 0, packet);
                        packet_batch.clear();
                    }
                }
            }

            // Send remainder
            if (!packet_batch.empty())
            {
                ENetPacket *packet = enet_packet_create(
                    packet_batch.data(), packet_batch.size() * sizeof(NetPixelUpdate),
                    ENET_PACKET_FLAG_RELIABLE);
                enet_host_broadcast(server, 0, packet);
            }
        }
    }
    enet_host_destroy(server);
    return 0;
}
