#pragma once
#include "protocol.hpp"
#include "world/helpers/constants.h"
#include "world/world.hpp"
#include <enet/enet.h>
#include <vector>

class Server
{
  private:
    ENetHost                   *m_host = nullptr;
    std::vector<NetPixelUpdate> m_packet_batch;
    const size_t                MAX_UPDATES_PER_PACKET = 175;
    std::vector<Chunk>          chunks_to_send;

    // Internal event handlers
    void on_client_connect(ENetEvent &event, World &world);
    void on_client_disconnect(ENetEvent &event);
    void on_client_event(ENetEvent &event, World &world, std::vector<Point> &manual_brush_changes);

    // Helpers
    void apply_brush_to_world(ClientInputPacket *input, World &world,
                              std::vector<Point> &manual_brush_changes);

  public:
    Server() = default;
    ~Server();

    bool start(uint16_t port);
    void stop();

    // Drains the network buffer, processes inputs/connections, and populates manual_brush_changes
    void poll_events(World &world, std::vector<Point> &manual_brush_changes);

    // Sends both physics and manual brush changes that happened this frame
    void send_changed_pixels(const std::vector<Point> &physics_changes,
                             const std::vector<Point> &manual_brush_changes, World &world);

    // Sends the full non-empty world state to a specific peer
    void send_chunk_data_all(ENetPeer *peer, World &world);
    void send_chunk_data_single(ENetPeer *peer, Chunk &chunk, World &world);
    void send_chunk_data_multiple(ENetPeer *peer, Chunk &chunk,
                                  std::vector<NetPixelUpdate> &sync_batch);
};
