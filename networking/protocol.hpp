#pragma once
#include <cstdint>

// ENet Channel definitions
enum NetworkChannel : uint8_t
{
    CHANNEL_INPUT         = 0, // Client -> Server brush clicks
    CHANNEL_PIXEL_UPDATES = 1, // Server -> Client incremental pixel changes
    CHANNEL_WORLD_SYNC    = 2, // Server -> Client massive initial chunk data
    CHANNEL_COUNT         = 3
};

#pragma pack(push, 1)

// Packet IDs
enum PacketID : uint8_t
{
    PACKET_BRUSH_INPUT = 0,
    PACKET_PIXEL_BATCH = 1
};

// Client -> Server
struct ClientInputPacket
{
    uint8_t packet_id;
    int32_t mouse_world_x;
    int32_t mouse_world_y;
    uint8_t brush_size;
    uint8_t brush_element;
    uint8_t is_erasing;
};

// Server -> Client (A single pixel in a batch)
struct NetPixelUpdate
{
    int32_t  global_x;
    int32_t  global_y;
    uint8_t  element_type;
    uint32_t color;
};

#pragma pack(pop)
